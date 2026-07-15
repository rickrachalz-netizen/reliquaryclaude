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
	}
}

void ARLWolfPack::EnterRoam()
{
	State = EPackState::Roam;
	Lunger = nullptr;
	AnchorLocation = ProjectPoint(GetCentroid());
	RoamTarget = AnchorLocation;	// dwell in place, then pick somewhere new
	RoamDwellTimer = Rng.FRandRange(RoamDwellMin, RoamDwellMax) * 0.5f;
	UE_LOG(LogRELIQUARY, Verbose, TEXT("WolfPack %s: lost the hero, roaming"), *GetName());
}

void ARLWolfPack::EnterEncircle()
{
	State = EPackState::Encircle;
	Lunger = nullptr;
	LungeElapsed = 0.f;
	LoseHeroTimer = 0.f;
	LungeTimer = LungeIntervalMin;	// let the ring form before the first bite
	UE_LOG(LogRELIQUARY, Verbose, TEXT("WolfPack %s: hero spotted, surrounding"), *GetName());
}

void ARLWolfPack::TickRoam(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive)
{
	if (GetHero() && (GetMinHeroDistance() < AggroRange || WasDamagedWithin(1.f)))
	{
		EnterEncircle();
		return;
	}

	// The virtual leader trots toward the roam target at the slowest wolf's
	// roam pace, holding up whenever somebody falls too far behind.
	float RefSpeed = MAX_flt;
	bool bRegroup = false;
	for (ARLEnemyBase* Wolf : Alive)
	{
		RefSpeed = FMath::Min(RefSpeed, Wolf->GetBaseMoveSpeed());
		bRegroup |= FVector::Dist2D(Wolf->GetActorLocation(), AnchorLocation) > RoamRingRadius * 3.f;
	}
	if (RefSpeed == MAX_flt)
	{
		RefSpeed = 400.f;
	}

	if (FVector::Dist2D(AnchorLocation, RoamTarget) <= 200.f)
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

	// Members chase formation slots slightly faster than the leader moves, so
	// the pack visibly travels together instead of stringing out.
	OrderRing(Alive, AnchorLocation, RoamRingRadius, RoamSpeedScale * 1.3f, SlotTolerance);

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), AnchorLocation, RoamTarget, FColor::Yellow, false, -1.f, 0, 2.f);
	}
}

void ARLWolfPack::TickEncircle(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive)
{
	APawn* Hero = GetHero();
	if (!Hero)
	{
		EnterRoam();
		return;
	}

	// Losing the hero takes sustained distance — one dodge roll doesn't count.
	if (GetMinHeroDistance() > LoseHeroRange && !WasDamagedWithin(2.f))
	{
		LoseHeroTimer += DeltaSeconds;
		if (LoseHeroTimer >= LoseHeroSeconds)
		{
			EnterRoam();
			return;
		}
	}
	else
	{
		LoseHeroTimer = 0.f;
	}

	const FVector HeroLocation = Hero->GetActorLocation();

	ARLEnemyBase* CurrentLunger = Lunger.Get();
	if (CurrentLunger && CurrentLunger->IsDead())
	{
		CurrentLunger = nullptr;
		Lunger = nullptr;
	}

	// Everyone but the lunger strafes to an even slot on the ring; with the
	// lunger excluded, the remaining wolves close the gap it left behind.
	OrderRing(Alive, HeroLocation, CircleRadius, EncircleSpeedScale, SlotTolerance, CurrentLunger);

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
			LungeTimer = Rng.FRandRange(LungeIntervalMin, LungeIntervalMax);
		}
	}
	else
	{
		LungeTimer -= DeltaSeconds;
		if (LungeTimer <= 0.f)
		{
			NextLungerIndex = (NextLungerIndex + 1) % Alive.Num();
			Lunger = Alive[NextLungerIndex];
			LungeElapsed = 0.f;
		}
	}

	if (bDrawDebug)
	{
		DrawDebugCircle(GetWorld(), HeroLocation, CircleRadius, 32, FColor::Red, false, -1.f, 0, 2.f,
			FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
		if (CurrentLunger)
		{
			DrawDebugLine(GetWorld(), CurrentLunger->GetActorLocation(), HeroLocation, FColor::Red, false, -1.f, 0, 3.f);
		}
	}
}
