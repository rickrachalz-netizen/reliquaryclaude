#include "RLWolfPack.h"
#include "RLEnemyBase.h"
#include "RELIQUARY.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"

void ARLWolfPack::NotifyMemberDamaged(ARLEnemyBase* Member, AActor* InstigatorActor)
{
	Super::NotifyMemberDamaged(Member, InstigatorActor);

	// Wounding one wolf brings the whole pack.
	if (State == EPackState::Roam)
	{
		EnterEncircle();
	}
}

void ARLWolfPack::TickGroup(float DeltaSeconds)
{
	TArray<ARLEnemyBase*> Alive;
	GetAliveMembers(Alive);
	if (Alive.Num() == 0)
	{
		return;
	}

	switch (State)
	{
	case EPackState::Roam:
		TickRoam(DeltaSeconds, Alive);
		break;
	case EPackState::Encircle:
		TickEncircle(DeltaSeconds, Alive);
		break;
	case EPackState::Pursue:
		TickPursue(DeltaSeconds, Alive);
		break;
	}
}

void ARLWolfPack::EnterRoam()
{
	State = EPackState::Roam;
	Lunger = nullptr;
	Interceptor = nullptr;
	AnchorLocation = ProjectPoint(GetCentroid());
	RoamTarget = AnchorLocation;	// mill briefly, then pick somewhere far
	RoamDwellTimer = Rng.FRandRange(RoamDwellMin, RoamDwellMax) * 0.5f;
	UE_LOG(LogRELIQUARY, Verbose, TEXT("WolfPack %s: lost the hero, roaming"), *GetName());
}

void ARLWolfPack::EnterEncircle()
{
	State = EPackState::Encircle;
	Lunger = nullptr;
	Interceptor = nullptr;
	InvalidateRing();	// everyone converges from arbitrary spots — assign fresh
	LungeElapsed = 0.f;
	LoseHeroTimer = 0.f;
	PursueScoreSeconds = 0.f;
	bRingFormed = false;
	LungeTimer = Rng.FRandRange(0.4f, 0.9f);	// first dart comes while closing in
	OrbitSign = Rng.FRand() < 0.5f ? -1.f : 1.f;
	OrbitFlipTimer = Rng.FRandRange(OrbitFlipSecondsMin, OrbitFlipSecondsMax);
	UE_LOG(LogRELIQUARY, Verbose, TEXT("WolfPack %s: hero spotted, surrounding"), *GetName());
}

void ARLWolfPack::EnterPursue()
{
	State = EPackState::Pursue;
	Lunger = nullptr;
	Interceptor = nullptr;
	LoseHeroTimer = 0.f;
	CalmScoreSeconds = 0.f;
	LungeTimer = Rng.FRandRange(LungeIntervalMin, LungeIntervalMax) * 0.5f;
	InterceptTimer = Rng.FRandRange(2.f, 4.f);	// first cutoff comes fairly soon
	UE_LOG(LogRELIQUARY, Verbose, TEXT("WolfPack %s: hero running, pack giving chase"), *GetName());
}

bool ARLWolfPack::TickLoseHero(float DeltaSeconds, APawn* Hero)
{
	if (!Hero)
	{
		EnterRoam();
		return true;
	}

	// Losing the hero takes sustained distance — one dodge roll doesn't count.
	if (GetMinHeroDistance() > LoseHeroRange && !WasDamagedWithin(2.f))
	{
		LoseHeroTimer += DeltaSeconds;
		if (LoseHeroTimer >= LoseHeroSeconds)
		{
			EnterRoam();
			return true;
		}
	}
	else
	{
		LoseHeroTimer = 0.f;
	}
	return false;
}

void ARLWolfPack::OrderLoosePack(const TArray<ARLEnemyBase*>& Alive, const FVector& Center,
	float SpreadScale, float BaseSpeedScale, ARLEnemyBase* ExcludeA, ARLEnemyBase* ExcludeB)
{
	const double Now = GetWorld()->GetTimeSeconds();
	for (ARLEnemyBase* Wolf : Alive)
	{
		if (Wolf == ExcludeA || Wolf == ExcludeB)
		{
			continue;
		}

		// Each wolf weaves on its own line: a drifting offset in the spread
		// and a gait of its own, rerolled every few seconds.
		FWolfStyle& Style = Styles.FindOrAdd(Wolf);
		if (Now >= Style.NextRerollSeconds)
		{
			const float Angle = Rng.FRandRange(0.f, 2.f * PI);
			const float Range = RoamSpreadRadius * FMath::Sqrt(Rng.FRand());	// uniform over the disc
			Style.Offset = FVector(FMath::Cos(Angle) * Range, FMath::Sin(Angle) * Range, 0.f);
			Style.SpeedJitter = Rng.FRandRange(0.85f, 1.2f);
			Style.NextRerollSeconds = Now + Rng.FRandRange(2.5f, 6.f);
		}

		Wolf->SetGroupOrder(ERLGroupOrder::MoveTo, Center + Style.Offset * SpreadScale,
			BaseSpeedScale * Style.SpeedJitter, 40.f);
	}
}

void ARLWolfPack::TickLungeToken(float DeltaSeconds, APawn* Hero, const TArray<ARLEnemyBase*>& Alive,
	float IntervalMin, float IntervalMax)
{
	const FVector HeroLocation = Hero->GetActorLocation();

	ARLEnemyBase* CurrentLunger = Lunger.Get();
	if (CurrentLunger && CurrentLunger->IsDead())
	{
		CurrentLunger = nullptr;
		Lunger = nullptr;
	}

	if (CurrentLunger)
	{
		LungeElapsed += DeltaSeconds;
		CurrentLunger->SetGroupOrder(ERLGroupOrder::MoveTo, HeroLocation, LungeSpeedScale, 10.f);

		const bool bInReach =
			FVector::Dist(CurrentLunger->GetActorLocation(), HeroLocation) <= CurrentLunger->GetStrikeReach();
		if (bInReach || LungeElapsed >= LungeMaxSeconds)
		{
			if (bInReach)	// the bite lands only when the dash connected
			{
				CurrentLunger->PerformTouchStrike(Hero);
			}
			Lunger = nullptr;
			LungeTimer = Rng.FRandRange(IntervalMin, IntervalMax);
		}

		if (bDrawDebug)
		{
			DrawDebugLine(GetWorld(), CurrentLunger->GetActorLocation(), HeroLocation, FColor::Red, false, -1.f, 0, 3.f);
		}
		return;
	}

	LungeTimer -= DeltaSeconds;
	if (LungeTimer <= 0.f)
	{
		// Round-robin, skipping the wolf that's busy cutting the hero off.
		for (int32 Try = 0; Try < Alive.Num(); ++Try)
		{
			NextLungerIndex = (NextLungerIndex + 1) % Alive.Num();
			if (Alive[NextLungerIndex] != Interceptor.Get())
			{
				Lunger = Alive[NextLungerIndex];
				LungeElapsed = 0.f;
				break;
			}
		}
	}
}

void ARLWolfPack::TickRoam(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive)
{
	if (GetHero() && (GetMinHeroDistance() < AggroRange || WasDamagedWithin(1.f)))
	{
		EnterEncircle();
		return;
	}

	// The virtual leader trots toward the roam target at the slowest wolf's
	// pace, holding up whenever somebody strays too far behind.
	float RefSpeed = MAX_flt;
	bool bRegroup = false;
	for (ARLEnemyBase* Wolf : Alive)
	{
		RefSpeed = FMath::Min(RefSpeed, Wolf->GetBaseMoveSpeed());
		bRegroup |= FVector::Dist2D(Wolf->GetActorLocation(), AnchorLocation) > RoamSpreadRadius * 2.2f;
	}
	if (RefSpeed == MAX_flt)
	{
		RefSpeed = 400.f;
	}

	if (FVector::Dist2D(AnchorLocation, RoamTarget) <= 250.f)
	{
		RoamDwellTimer -= DeltaSeconds;
		if (RoamDwellTimer <= 0.f)
		{
			RoamTarget = PickWanderPoint(AnchorLocation, RoamTargetRangeMin, RoamTargetRangeMax);
			RoamDwellTimer = Rng.FRandRange(RoamDwellMin, RoamDwellMax);
		}
	}
	else if (!bRegroup)
	{
		const FVector Direction = (RoamTarget - AnchorLocation).GetSafeNormal2D();
		AnchorLocation += Direction * RefSpeed * RoamSpeedScale * DeltaSeconds;
		AnchorLocation.Z = GetCentroid().Z;	// ride the terrain via the wolves themselves
	}

	// The pack drifts as a loose spread around the moving anchor, everyone
	// weaving their own line — wild horses, not a marching column.
	OrderLoosePack(Alive, AnchorLocation, 1.f, RoamSpeedScale * 1.15f);

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), AnchorLocation, RoamTarget, FColor::Yellow, false, -1.f, 0, 2.f);
	}
}

void ARLWolfPack::TickEncircle(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive)
{
	APawn* Hero = GetHero();
	if (TickLoseHero(DeltaSeconds, Hero))
	{
		return;
	}

	const FVector HeroLocation = Hero->GetActorLocation();

	// A hero who keeps running breaks the ring into an open chase.
	if (Hero->GetVelocity().Size2D() > PursueTriggerSpeed && GetMinHeroDistance() > CircleRadius * 1.15f)
	{
		PursueScoreSeconds += DeltaSeconds;
		if (PursueScoreSeconds >= PursueTriggerSeconds)
		{
			EnterPursue();
			return;
		}
	}
	else
	{
		PursueScoreSeconds = FMath::Max(0.f, PursueScoreSeconds - DeltaSeconds * 2.f);
	}

	// The orbit drifts around the hero and flips direction now and then.
	OrbitFlipTimer -= DeltaSeconds;
	if (OrbitFlipTimer <= 0.f)
	{
		OrbitSign = -OrbitSign;
		OrbitFlipTimer = Rng.FRandRange(OrbitFlipSecondsMin, OrbitFlipSecondsMax);
	}

	// Everyone but the lunger pads sideways after slots that themselves orbit
	// the hero, so circling is continuous; the lunger's gap closes itself.
	ARLEnemyBase* CurrentLunger = Lunger.Get();
	OrderRing(Alive, HeroLocation, CircleRadius, EncircleSpeedScale, SlotTolerance, CurrentLunger,
		OrbitSign * OrbitSpeed * DeltaSeconds, /*bFaceCenterAtSlot=*/true);

	// The pack darts more boldly once the ring has actually closed.
	int32 RingCount = 0;
	int32 AtRadius = 0;
	for (ARLEnemyBase* Wolf : Alive)
	{
		if (Wolf == CurrentLunger)
		{
			continue;
		}
		++RingCount;
		const float R = FVector::Dist2D(Wolf->GetActorLocation(), HeroLocation);
		if (FMath::Abs(R - CircleRadius) < CircleRadius * 0.35f)
		{
			++AtRadius;
		}
	}
	bRingFormed = RingCount > 0 && AtRadius * 5 >= RingCount * 3;	// >= 60%

	TickLungeToken(DeltaSeconds, Hero, Alive,
		bRingFormed ? LungeIntervalFormedMin : LungeIntervalMin,
		bRingFormed ? LungeIntervalFormedMax : LungeIntervalMax);

	if (bDrawDebug)
	{
		DrawDebugCircle(GetWorld(), HeroLocation, CircleRadius, 32,
			bRingFormed ? FColor::Red : FColor::Orange, false, -1.f, 0, 2.f,
			FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}
}

void ARLWolfPack::TickPursue(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive)
{
	APawn* Hero = GetHero();
	if (TickLoseHero(DeltaSeconds, Hero))
	{
		return;
	}

	const FVector HeroLocation = Hero->GetActorLocation();

	// The hero slowing down — or the pack catching up — re-forms the ring.
	if (Hero->GetVelocity().Size2D() < PursueCalmSpeed || GetMinHeroDistance() < CircleRadius * 1.1f)
	{
		CalmScoreSeconds += DeltaSeconds;
		if (CalmScoreSeconds >= 0.7f)
		{
			EnterEncircle();
			return;
		}
	}
	else
	{
		CalmScoreSeconds = 0.f;
	}

	ARLEnemyBase* CurrentInterceptor = Interceptor.Get();
	if (CurrentInterceptor && CurrentInterceptor->IsDead())
	{
		CurrentInterceptor = nullptr;
		Interceptor = nullptr;
	}

	// The chase runs in the same loose spread the pack roams in.
	OrderLoosePack(Alive, HeroLocation, 0.55f, PursueSpeedScale, Lunger.Get(), CurrentInterceptor);

	// Take-turns bites keep landing from behind while the hero runs...
	TickLungeToken(DeltaSeconds, Hero, Alive, LungeIntervalMin, LungeIntervalMax);

	// ...and every so often one wolf bursts ahead to cut the hero off.
	if (CurrentInterceptor)
	{
		InterceptElapsed += DeltaSeconds;

		FVector HeroVelocity = Hero->GetVelocity();
		HeroVelocity.Z = 0.f;
		const FVector CutoffPoint = HeroLocation + HeroVelocity * InterceptLeadSeconds;
		CurrentInterceptor->SetGroupOrder(ERLGroupOrder::MoveTo, CutoffPoint, InterceptSpeedScale, 40.f);

		const bool bInReach =
			FVector::Dist(CurrentInterceptor->GetActorLocation(), HeroLocation) <= CurrentInterceptor->GetStrikeReach();
		if (bInReach || InterceptElapsed >= InterceptMaxSeconds)
		{
			if (bInReach)
			{
				CurrentInterceptor->PerformTouchStrike(Hero);
			}
			Interceptor = nullptr;
			InterceptTimer = Rng.FRandRange(InterceptIntervalMin, InterceptIntervalMax);
		}

		if (bDrawDebug)
		{
			DrawDebugLine(GetWorld(), CurrentInterceptor->GetActorLocation(), CutoffPoint,
				FColor::Magenta, false, -1.f, 0, 3.f);
		}
	}
	else
	{
		InterceptTimer -= DeltaSeconds;
		if (InterceptTimer <= 0.f)
		{
			// The wolf best placed to head the hero off takes the burst.
			FVector HeroVelocity = Hero->GetVelocity();
			HeroVelocity.Z = 0.f;
			const FVector CutoffPoint = HeroLocation + HeroVelocity * InterceptLeadSeconds;

			ARLEnemyBase* Best = nullptr;
			float BestDistance = MAX_flt;
			for (ARLEnemyBase* Wolf : Alive)
			{
				if (Wolf == Lunger.Get())
				{
					continue;
				}
				const float Distance = FVector::Dist2D(Wolf->GetActorLocation(), CutoffPoint);
				if (Distance < BestDistance)
				{
					BestDistance = Distance;
					Best = Wolf;
				}
			}
			if (Best)
			{
				Interceptor = Best;
				InterceptElapsed = 0.f;
			}
		}
	}
}
