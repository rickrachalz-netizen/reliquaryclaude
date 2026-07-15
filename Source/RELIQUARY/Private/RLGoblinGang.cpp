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

	// A runner that gained company (another runner joined it mid-flight) is a
	// gang again — camp up and let the courage table take over next tick.
	if ((State == EGangState::Flee || State == EGangState::Seek) && Alive.Num() > 1)
	{
		EnterCircle(/*bReanchor=*/true);
	}

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
		TickFlee(DeltaSeconds, Alive);
		break;
	case EGangState::Seek:
		TickSeek(DeltaSeconds, Alive);
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

	// A calm lone goblin ambles toward the nearest gang it can sense;
	// everyone else just picks somewhere new to set up the circle.
	APawn* Hero = GetHero();
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
	else if (AliveCount == 1 && Hero)
	{
		// No friends left anywhere: drift, favoring the leg that grows the
		// gap — pure random picks walked the lone survivor straight back
		// into the hero half the time (the old ping-pong).
		FVector Best = PickWanderPoint(AnchorLocation, RoamTargetRangeMin, RoamTargetRangeMax);
		float BestDistance = FVector::Dist2D(Best, Hero->GetActorLocation());
		for (int32 i = 0; i < 2; ++i)
		{
			const FVector Candidate = PickWanderPoint(AnchorLocation, RoamTargetRangeMin, RoamTargetRangeMax);
			const float Distance = FVector::Dist2D(Candidate, Hero->GetActorLocation());
			if (Distance > BestDistance)
			{
				Best = Candidate;
				BestDistance = Distance;
			}
		}
		RoamTarget = Best;
	}
	else
	{
		RoamTarget = PickWanderPoint(AnchorLocation, RoamTargetRangeMin, RoamTargetRangeMax);
	}
}

void ARLGoblinGang::EnterFight()
{
	State = EGangState::Fight;
	InvalidateRing();	// coming home afterwards re-forms the circle fresh
	UE_LOG(LogRELIQUARY, Verbose, TEXT("GoblinGang %s: standing and fighting (%d strong)"),
		*GetName(), GetAliveCount());
}

void ARLGoblinGang::EnterHunt()
{
	State = EGangState::Hunt;
	InvalidateRing();
	UE_LOG(LogRELIQUARY, Verbose, TEXT("GoblinGang %s: mob hunting the hero (%d strong)"),
		*GetName(), GetAliveCount());
}

void ARLGoblinGang::EnterFlee()
{
	State = EGangState::Flee;
	FleeStartSeconds = GetWorld()->GetTimeSeconds();
	FleeRepathTimer = 0.f;
	FleeEscapePoint = FVector::ZeroVector;
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

	// Traveling ring: tighter arrival slack keeps everyone flowing with the
	// moving anchor instead of stop-starting behind it, and nobody bothers
	// facing the center until the gang actually camps.
	const float TravelRadius = Alive.Num() == 1 ? 0.f : CircleRadius * 0.8f;
	OrderRing(Alive, AnchorLocation, TravelRadius, RoamSpeedScale * 1.35f, SlotTolerance * 0.6f,
		nullptr, 0.f, /*bFaceCenterAtSlot=*/false);

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

void ARLGoblinGang::TickFlee(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive)
{
	APawn* Hero = GetHero();
	ARLEnemyBase* Runner = Alive[0];
	const FVector RunnerLocation = Runner->GetActorLocation();

	if (Hero)
	{
		LastKnownHeroLocation = Hero->GetActorLocation();
	}

	// Stumbling into a gang during the panic sprint means joining on the spot.
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

	// The flee is ONE decision: a short panic burst dead away, then commit to
	// the nearest gang and run there without reconsidering (Seek). Only a
	// goblin with no friends left anywhere keeps running until it's safe.
	const bool bBurst = GetWorld()->GetTimeSeconds() - FleeStartSeconds <= FleeBurstSeconds;
	if (!bBurst)
	{
		if (Refuge)
		{
			EnterSeek(Refuge);
			return;
		}
		if (!Hero || FVector::Dist(RunnerLocation, Hero->GetActorLocation()) > FleeSafeRange)
		{
			AnchorLocation = ProjectPoint(GetCentroid());
			EnterRoam(Alive.Num());
			return;
		}
	}

	// Escape decisions are rate-limited: pick a line, commit to it for a
	// beat, then reassess. Re-deciding every frame made the runner wobble.
	FleeRepathTimer -= DeltaSeconds;
	if ((FleeRepathTimer <= 0.f || FleeEscapePoint.IsNearlyZero()) && Hero)
	{
		FleeRepathTimer = 0.4f;

		// Candidate escape points straight away from the hero, refusing any
		// the navmesh projection snaps back toward them (at nav edges the
		// nearest on-mesh point can sit BEHIND the runner — chasing it made
		// goblins boomerang at the player).
		const FVector HeroLocation = Hero->GetActorLocation();
		const FVector Away = (RunnerLocation - HeroLocation).GetSafeNormal2D();
		const FVector Perpendicular(-Away.Y, Away.X, 0.f);
		const FVector Candidates[3] = {
			Away,
			(Away + Perpendicular).GetSafeNormal2D(),
			(Away - Perpendicular).GetSafeNormal2D()
		};
		const float HeroDistanceNow = FVector::Dist2D(RunnerLocation, HeroLocation);
		FleeEscapePoint = RunnerLocation + Away * FleeStepDistance;	// beeline fallback
		for (const FVector& Candidate : Candidates)
		{
			const FVector Projected = ProjectPoint(RunnerLocation + Candidate * FleeStepDistance);
			if (FVector::Dist2D(Projected, HeroLocation) > HeroDistanceNow + 100.f)
			{
				FleeEscapePoint = Projected;
				break;
			}
		}
	}

	if (!FleeEscapePoint.IsNearlyZero())
	{
		Runner->SetGroupOrder(ERLGroupOrder::MoveTo, FleeEscapePoint,
			bBurst ? FleeBurstSpeedScale : FleeSpeedScale, 100.f);
	}

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), RunnerLocation, FleeEscapePoint, FColor::Orange, false, -1.f, 0, 2.f);
	}
}

void ARLGoblinGang::EnterSeek(ARLGoblinGang* Refuge)
{
	State = EGangState::Seek;
	SeekRefuge = Refuge;
	SeekRefreshTimer = 0.f;
	SeekTarget = FVector::ZeroVector;
	UE_LOG(LogRELIQUARY, Verbose, TEXT("GoblinGang %s: last goblin committed to joining %s"),
		*GetName(), *Refuge->GetName());
}

void ARLGoblinGang::TickSeek(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive)
{
	// Committed: no threat checks, no dwells, no new decisions — the runner
	// goes to its friends no matter where the hero stands, and the navmesh
	// routes it around bodies and terrain on the way.
	APawn* Hero = GetHero();
	if (Hero)
	{
		LastKnownHeroLocation = Hero->GetActorLocation();
	}

	ARLGoblinGang* Refuge = SeekRefuge.Get();
	if (!Refuge || !IsValid(Refuge) || Refuge->GetAliveCount() == 0)
	{
		// That gang is gone; commit to another, or wander if none remain.
		float GangDistance = 0.f;
		Refuge = FindNearestGang(SeekRange, GangDistance);
		if (!Refuge)
		{
			AnchorLocation = ProjectPoint(GetCentroid());
			EnterRoam(Alive.Num());
			return;
		}
		SeekRefuge = Refuge;
		SeekRefreshTimer = 0.f;
	}

	if (FVector::Dist2D(GetCentroid(), Refuge->GetCentroid()) <= MergeRange * 1.2f)
	{
		// Made it: join, and hand over where the hero chased the runner from —
		// a calm gang marches there as a posse.
		const FVector Intel = LastKnownHeroLocation;
		Refuge->AbsorbGang(this);	// destroys this gang — only locals below
		if (!Intel.IsNearlyZero())
		{
			Refuge->ReceiveHeroIntel(Intel);
		}
		return;
	}

	// The refuge gang wanders too; refresh the destination on a slow cadence.
	SeekRefreshTimer -= DeltaSeconds;
	if (SeekRefreshTimer <= 0.f || SeekTarget.IsNearlyZero())
	{
		SeekRefreshTimer = 0.75f;
		SeekTarget = ProjectPoint(Refuge->GetCentroid());
	}

	Alive[0]->SetGroupOrder(ERLGroupOrder::MoveTo, SeekTarget, FleeSpeedScale, 120.f);

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), Alive[0]->GetActorLocation(), SeekTarget, FColor::Green, false, -1.f, 0, 2.f);
	}
}

ARLGoblinGang* ARLGoblinGang::FindNearestGang(float MaxRange, float& OutDistance,
	const FVector* AvoidLocation, float AvoidRadius) const
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
		if (AvoidLocation && FVector::Dist2D(Other->GetCentroid(), *AvoidLocation) < AvoidRadius)
		{
			continue;	// that refuge sits in the danger zone
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
