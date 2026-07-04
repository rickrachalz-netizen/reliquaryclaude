#include "RLEnemyDirector.h"
#include "RLEnemyBase.h"
#include "RLRunManagerSubsystem.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

ARLEnemyDirector::ARLEnemyDirector()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARLEnemyDirector::BeginPlay()
{
	Super::BeginPlay();

	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>() : nullptr)
	{
		Rng = RunManager->MakeZoneRng();
		Rng.SeedFrom(FPlatformTime::Cycles64());	// spawn timing may vary; layouts may not
	}
}

int32 ARLEnemyDirector::GetAliveCount() const
{
	int32 Alive = 0;
	for (const ARLEnemyBase* Enemy : AliveEnemies)
	{
		if (IsValid(Enemy) && !Enemy->IsDead())
		{
			++Alive;
		}
	}
	return Alive;
}

void ARLEnemyDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UGameInstance* GI = GetGameInstance();
	URLRunManagerSubsystem* RunManager = GI ? GI->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;
	if (!RunManager || !Data || !RunManager->IsOnRun())
	{
		return;
	}

	// Income: base rate from the zone, multiplied by current difficulty.
	float CreditRate = 3.f;
	if (const FRLZoneRow* Zone = Data->FindZone(RunManager->GetCurrentZoneIndex()))
	{
		CreditRate = Zone->DirectorCreditRate;
	}
	Credits += CreditRate * RunManager->GetDifficultyCoefficient() * DeltaSeconds;

	TimeSinceSpend += DeltaSeconds;
	if (TimeSinceSpend >= SpawnInterval)
	{
		TimeSinceSpend = 0.f;
		TrySpend();
	}
}

void ARLEnemyDirector::TrySpend()
{
	UGameInstance* GI = GetGameInstance();
	URLRunManagerSubsystem* RunManager = GI->GetSubsystem<URLRunManagerSubsystem>();
	URLDataSubsystem* Data = GI->GetSubsystem<URLDataSubsystem>();

	AliveEnemies.RemoveAll([](const ARLEnemyBase* Enemy) { return !IsValid(Enemy) || Enemy->IsDead(); });
	if (AliveEnemies.Num() >= MaxAliveEnemies)
	{
		return;
	}

	const float Difficulty = RunManager->GetDifficultyCoefficient();

	// Spend in a burst until the wallet or the cap runs out.
	int32 SafetyCounter = 8;
	while (SafetyCounter-- > 0 && AliveEnemies.Num() < MaxAliveEnemies)
	{
		const FName CardId = Data->DrawSpawnCard(
			RunManager->GetCurrentZoneIndex(), Difficulty, Credits, Rng);
		if (CardId == NAME_None)
		{
			break;
		}

		const FRLSpawnCardRow* Card = Data->FindSpawnCard(CardId);
		UClass* EnemyClass = Card ? Card->EnemyClass.LoadSynchronous() : nullptr;
		if (!Card || !EnemyClass)
		{
			break;
		}

		// Elite promotion grows more likely as the run drags on.
		const float EliteChance = FMath::Clamp((Difficulty - 1.5f) * 0.08f, 0.f, 0.3f);
		const bool bElite = Rng.FRand() < EliteChance && Credits >= Card->Cost * EliteCostMultiplier;
		const float Cost = bElite ? Card->Cost * EliteCostMultiplier : Card->Cost;
		if (Credits < Cost)
		{
			break;
		}

		FVector SpawnLocation;
		if (!FindSpawnPoint(SpawnLocation))
		{
			break;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		ARLEnemyBase* Enemy = GetWorld()->SpawnActor<ARLEnemyBase>(
			EnemyClass, SpawnLocation, FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), Params);
		if (!Enemy)
		{
			break;
		}

		Enemy->bElite = bElite;
		Credits -= Cost;
		AliveEnemies.Add(Enemy);
	}
}

bool ARLEnemyDirector::FindSpawnPoint(FVector& OutLocation) const
{
	APawn* Hero = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Hero)
	{
		return false;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	for (int32 Try = 0; Try < 10; ++Try)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Distance = FMath::FRandRange(MinSpawnDistance, MaxSpawnDistance);
		const FVector Candidate = Hero->GetActorLocation() +
			FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 0.f);

		if (NavSystem)
		{
			FNavLocation Projected;
			if (NavSystem->ProjectPointToNavigation(Candidate, Projected, FVector(500.f, 500.f, 1000.f)))
			{
				OutLocation = Projected.Location + FVector(0.f, 0.f, 100.f);
				return true;
			}
		}
		else
		{
			// No navmesh: trace to ground instead.
			FHitResult Hit;
			const FVector Start = Candidate + FVector(0.f, 0.f, 2000.f);
			const FVector End = Candidate - FVector(0.f, 0.f, 2000.f);
			if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic))
			{
				OutLocation = Hit.ImpactPoint + FVector(0.f, 0.f, 100.f);
				return true;
			}
		}
	}
	return false;
}
