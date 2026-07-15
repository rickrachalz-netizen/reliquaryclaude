#include "RLGoblinGang.h"
#include "RLEnemyBase.h"
#include "RELIQUARY.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

void ARLGoblinGang::TickGroup(float DeltaSeconds)
{
	TArray<ARLEnemyBase*> Alive;
	GetAliveMembers(Alive);
	if (Alive.Num() == 0)
	{
		return;
	}

	APawn* Hero = GetHero();

	// Camp-centric perception: a calm gang notices the hero near its ANCHOR,
	// not near its members — otherwise a gang walking home from a leashed
	// fight would re-aggro off the hero it just disengaged from and stutter
	// between the two states forever. Damage overrides at any range.
	const bool bThreatened = Hero &&
		(FVector::Dist2D(Hero->GetActorLocation(), AnchorLocation) < AlertRange
			|| WasDamagedWithin(DamageAlertSeconds));

	// Courage is a pure function of headcount, so deaths mid-fight re-branch
	// on the spot: a hunting mob whittled to 3 loses its nerve for the chase,
	// and the last goblin standing always breaks and runs.
	if (State == EGangState::Fight || State == EGangState::Hunt)
	{
		if (Alive.Num() == 1)
		{
			EnterFlee();
		}
		else if (Alive.Num() >= HuntThreshold && State != EGangState::Hunt)
		{
			EnterHunt();
		}
		else if (Alive.Num() < HuntThreshold && State != EGangState::Fight)
		{
			EnterFight();
		}
	}

	switch (State)
	{
	case EGangState::Circle:
	case EGangState::Roam:
		TickCalm(DeltaSeconds, Alive, bThreatened);
		break;
	case EGangState::Fight:
		TickFight(Alive);
		break;
	case EGangState::Hunt:
		TickHunt(Alive);
		break;
	case EGangState::Flee:
		TickFlee(Alive);
		break;
	}
}

void ARLGoblinGang::EnterCircle(bool bReanchor)
{
	State = EGangState::Circle;
	if (bReanchor)
	{
		AnchorLocation = ProjectPoint(GetCentroid());
	}
	// Lone goblins only pause briefly — they have friends to find.
	CircleDwellTimer = GetAliveCount() == 1
		? Rng.FRandRange(1.f, 3.f)
		: Rng.FRandRange(CircleDwellMin, CircleDwellMax);
}

void ARLGoblinGang::EnterRoam(int32 AliveCount)
{
	State = EGangState::Roam;

	// A lone goblin drifts toward the nearest gang it can sense; everyone
	// else just picks somewhere new to set up the circle.
	float GangDistance = 0.f;
	ARLGoblinGang* NearestGang = AliveCount == 1 ? FindNearestGang(SeekRange, GangDistance) : nullptr;
	if (NearestGang)
	{
		// Jitter kept well inside MergeRange so arriving means merging.
		const FVector Jitter(Rng.FRandRange(-150.f, 150.f), Rng.FRandRange(-150.f, 150.f), 0.f);
		RoamTarget = ProjectPoint(NearestGang->GetCentroid() + Jitter);
		UE_LOG(LogRELIQUARY, Verbose, TEXT("GoblinGang %s: lone goblin heading for %s"),
			*GetName(), *NearestGang->GetName());
	}
	else
	{
		RoamTarget = PickWanderPoint(AnchorLocation, RoamTargetRangeMin, RoamTargetRangeMax);
	}
}

void ARLGoblinGang::EnterFight()
{
	State = EGangState::Fight;
	UE_LOG(LogRELIQUARY, Verbose, TEXT("GoblinGang %s: standing and fighting (%d strong)"),
		*GetName(), GetAliveCount());
}

void ARLGoblinGang::EnterHunt()
{
	State = EGangState::Hunt;
	UE_LOG(LogRELIQUARY, Verbose, TEXT("GoblinGang %s: mob hunting the hero (%d strong)"),
		*GetName(), GetAliveCount());
}

void ARLGoblinGang::EnterFlee()
{
	State = EGangState::Flee;
	FleeStartSeconds = GetWorld()->GetTimeSeconds();
	UE_LOG(LogRELIQUARY, Verbose, TEXT("GoblinGang %s: last goblin fleeing"), *GetName());
}

void ARLGoblinGang::ReceiveHeroIntel(const FVector& HeroLocation)
{
	if (!IsCalm())
	{
		return;	// a gang already in a fight has fresher intel of its own
	}
	State = EGangState::Roam;
	RoamTarget = ProjectPoint(HeroLocation);
	UE_LOG(LogRELIQUARY, Verbose, TEXT("GoblinGang %s: posse marching to the hero's last known position (%d strong)"),
		*GetName(), GetAliveCount());
}

void ARLGoblinGang::TickCalm(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive, bool bThreatened)
{
	if (bThreatened)
	{
		if (Alive.Num() == 1)
		{
			EnterFlee();
		}
		else if (Alive.Num() >= HuntThreshold)
		{
			EnterHunt();
		}
		else
		{
			EnterFight();
		}
		return;
	}

	// Socializing: merge with a gang that's close. When this gang was the one
	// consumed, it is already being destroyed — hands off everything.
	if (TryMerge(Alive))
	{
		return;
	}

	if (State == EGangState::Circle)
	{
		// A circle of one is just a goblin standing around; use radius zero so
		// it pauses in place instead of orbiting an invisible point.
		const float Radius = Alive.Num() == 1 ? 0.f : CircleRadius;
		OrderRing(Alive, AnchorLocation, Radius, RoamSpeedScale, SlotTolerance);

		CircleDwellTimer -= DeltaSeconds;
		if (CircleDwellTimer <= 0.f)
		{
			EnterRoam(Alive.Num());
		}
		return;
	}

	// Roam: the camp anchor ambles toward the target with the gang loosely
	// ringed around it; arrival re-forms the circle.
	float RefSpeed = MAX_flt;
	bool bRegroup = false;
	for (ARLEnemyBase* Goblin : Alive)
	{
		RefSpeed = FMath::Min(RefSpeed, Goblin->GetBaseMoveSpeed());
		bRegroup |= FVector::Dist2D(Goblin->GetActorLocation(), AnchorLocation) > CircleRadius * 3.f + 300.f;
	}
	if (RefSpeed == MAX_flt)
	{
		RefSpeed = 300.f;
	}

	if (FVector::Dist2D(AnchorLocation, RoamTarget) <= 150.f)
	{
		EnterCircle(/*bReanchor=*/false);
	}
	else if (!bRegroup)
	{
		const FVector Direction = (RoamTarget - AnchorLocation).GetSafeNormal2D();
		AnchorLocation += Direction * RefSpeed * RoamSpeedScale * DeltaSeconds;
		AnchorLocation.Z = GetCentroid().Z;
	}

	const float TravelRadius = Alive.Num() == 1 ? 0.f : CircleRadius * 0.8f;
	OrderRing(Alive, AnchorLocation, TravelRadius, RoamSpeedScale * 1.35f, SlotTolerance);

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), AnchorLocation, RoamTarget, FColor::Green, false, -1.f, 0, 2.f);
	}
}

void ARLGoblinGang::TickFight(const TArray<ARLEnemyBase*>& Alive)
{
	APawn* Hero = GetHero();

	// Leashed to the camp: chase the hero around it, but a hero who leaves is
	// let go — the gang drifts back to its circle.
	if (!Hero ||
		(FVector::Dist2D(Hero->GetActorLocation(), AnchorLocation) > FightLeashRange
			&& !WasDamagedWithin(DamageAlertSeconds)))
	{
		EnterCircle(/*bReanchor=*/false);
		return;
	}

	for (ARLEnemyBase* Goblin : Alive)
	{
		Goblin->SetGroupOrder(ERLGroupOrder::EngageHero, Hero->GetActorLocation(), 1.f);
	}
}

void ARLGoblinGang::TickHunt(const TArray<ARLEnemyBase*>& Alive)
{
	APawn* Hero = GetHero();

	// The mob presses far beyond the camp; it camps wherever the chase ends.
	if (!Hero || (GetMinHeroDistance() > HuntGiveUpRange && !WasDamagedWithin(DamageAlertSeconds)))
	{
		EnterCircle(/*bReanchor=*/true);
		return;
	}

	for (ARLEnemyBase* Goblin : Alive)
	{
		Goblin->SetGroupOrder(ERLGroupOrder::EngageHero, Hero->GetActorLocation(), 1.05f);
	}
}

void ARLGoblinGang::TickFlee(const TArray<ARLEnemyBase*>& Alive)
{
	APawn* Hero = GetHero();
	ARLEnemyBase* Runner = Alive[0];
	const FVector RunnerLocation = Runner->GetActorLocation();

	if (Hero)
	{
		LastKnownHeroLocation = Hero->GetActorLocation();
	}

	// The sprint has a destination: the nearest gang. Reaching it mid-flee
	// means joining on the spot — and the reinforced gang learns where the
	// hero chased the runner from, forming a posse that marches there.
	float GangDistance = 0.f;
	ARLGoblinGang* Refuge = FindNearestGang(SeekRange, GangDistance);
	if (Refuge && FVector::Dist2D(GetCentroid(), Refuge->GetCentroid()) <= MergeRange * 1.2f)
	{
		const FVector Intel = LastKnownHeroLocation;
		Refuge->AbsorbGang(this);	// destroys this gang — only locals below
		if (!Intel.IsNearlyZero())
		{
			Refuge->ReceiveHeroIntel(Intel);
		}
		return;
	}

	if (!Hero || FVector::Dist(RunnerLocation, Hero->GetActorLocation()) > FleeSafeRange)
	{
		// Safe: back to wandering (and looking for a new gang to join).
		AnchorLocation = ProjectPoint(GetCentroid());
		EnterRoam(Alive.Num());
		return;
	}

	// Escape direction: away from the hero, bent toward the refuge gang when
	// one exists — the panic sprint runs somewhere, not just anywhere.
	const FVector Away = (RunnerLocation - Hero->GetActorLocation()).GetSafeNormal2D();
	FVector Desired = Away;
	if (Refuge)
	{
		const FVector TowardRefuge = (Refuge->GetCentroid() - RunnerLocation).GetSafeNormal2D();
		const FVector Blend = (Away + TowardRefuge * 1.25f).GetSafeNormal2D();
		if (!Blend.IsNearlyZero())
		{
			Desired = Blend;
		}
	}

	// Candidate escape points, refusing any the navmesh projection snaps back
	// toward the hero (at nav edges the nearest on-mesh point can sit BEHIND
	// the runner — chasing it made fleeing goblins boomerang at the player).
	const FVector Perpendicular(-Away.Y, Away.X, 0.f);
	const FVector Candidates[4] = {
		Desired,
		Away,
		(Away + Perpendicular).GetSafeNormal2D(),
		(Away - Perpendicular).GetSafeNormal2D()
	};
	const float HeroDistanceNow = FVector::Dist2D(RunnerLocation, Hero->GetActorLocation());
	FVector Escape = RunnerLocation + Desired * FleeStepDistance;	// beeline fallback
	for (const FVector& Candidate : Candidates)
	{
		const FVector Projected = ProjectPoint(RunnerLocation + Candidate * FleeStepDistance);
		if (FVector::Dist2D(Projected, Hero->GetActorLocation()) > HeroDistanceNow + 100.f)
		{
			Escape = Projected;
			break;
		}
	}

	// The first moments of a flee are a panicked burst of speed.
	const bool bBurst = GetWorld()->GetTimeSeconds() - FleeStartSeconds <= FleeBurstSeconds;
	Runner->SetGroupOrder(ERLGroupOrder::MoveTo, Escape,
		bBurst ? FleeBurstSpeedScale : FleeSpeedScale, 100.f);

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), RunnerLocation, Escape, FColor::Orange, false, -1.f, 0, 2.f);
	}
}

ARLGoblinGang* ARLGoblinGang::FindNearestGang(float MaxRange, float& OutDistance) const
{
	ARLGoblinGang* Nearest = nullptr;
	OutDistance = MaxRange;

	const FVector MyCentroid = GetCentroid();
	for (TActorIterator<ARLGoblinGang> It(GetWorld()); It; ++It)
	{
		ARLGoblinGang* Other = *It;
		if (Other == this || !IsValid(Other) || Other->GetAliveCount() == 0)
		{
			continue;
		}
		const float Distance = FVector::Dist2D(MyCentroid, Other->GetCentroid());
		if (Distance <= OutDistance)
		{
			OutDistance = Distance;
			Nearest = Other;
		}
	}
	return Nearest;
}

void ARLGoblinGang::AbsorbGang(ARLGoblinGang* Consumed)
{
	TArray<ARLEnemyBase*> Joining;
	Consumed->GetAliveMembers(Joining);
	for (ARLEnemyBase* Goblin : Joining)
	{
		RegisterMember(Goblin);	// re-points the member's group at us
	}
	Consumed->Members.Empty();
	Consumed->Destroy();

	UE_LOG(LogRELIQUARY, Verbose, TEXT("GoblinGang %s: absorbed %d goblins (now %d strong)"),
		*GetName(), Joining.Num(), GetAliveCount());

	// Fold the newcomers in: a calm gang reforms its circle where everyone
	// now is; a fighting gang just gained reinforcements and stays on task.
	if (IsCalm())
	{
		EnterCircle(/*bReanchor=*/true);
	}
}

bool ARLGoblinGang::TryMerge(const TArray<ARLEnemyBase*>& Alive)
{
	float Distance = 0.f;
	ARLGoblinGang* Other = FindNearestGang(MergeRange, Distance);
	if (!Other || !Other->IsCalm())
	{
		return false;
	}

	// The bigger gang absorbs; ties break on UniqueID so exactly one side acts.
	const int32 OtherCount = Other->GetAliveCount();
	const bool bIAbsorb = Alive.Num() > OtherCount
		|| (Alive.Num() == OtherCount && GetUniqueID() < Other->GetUniqueID());
	if (bIAbsorb)
	{
		AbsorbGang(Other);
		return false;
	}

	Other->AbsorbGang(this);
	return true;
}
