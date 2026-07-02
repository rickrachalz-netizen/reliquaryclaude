#include "RLEnemyDirector.h"
#include "RLEnemyBase.h"
#include "RLCombatFormulas.h"
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

	UGameInstance* GI = GetGameInstance();
	URLRunManagerSubsystem* RunManager = GI ? GI->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;

	if (RunManager)
	{
		Rng = RunManager->MakeZoneRandomStream();
		Rng.GenerateNewSeed();	// spawn timing may vary; layouts may not

		// The zone's DirectorCreditRate scales this director's income.
		if (Data)
		{
			if (const FRLZoneRow* Zone = Data->FindZone(RunManager->GetCurrentZoneIndex()))
			{
				ZoneCreditScale = Zone->DirectorCreditRate;
			}
		}
	}

	ScheduleNextWave();
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
	if (!RunManager || !RunManager->IsOnRun())
	{
		return;
	}

	// RoR2 income: credits/sec = multiplier x (1 + 0.4 x coeff). The realm
	// pays its director better the longer you stay.
	const float Coefficient = RunManager->GetDifficultyCoefficient();
	Credits += RLCombat::DirectorCreditsPerSecond(CreditMultiplier * ZoneCreditScale, Coefficient) * DeltaSeconds;

	NextWaveIn -= DeltaSeconds;
	if (NextWaveIn <= 0.f)
	{
		SpendWave();
		ScheduleNextWave();
	}
}

void ARLEnemyDirector::ScheduleNextWave()
{
	NextWaveIn = Rng.FRandRange(MinWaveInterval, MaxWaveInterval);
}

void ARLEnemyDirector::SpendWave()
{
	UGameInstance* GI = GetGameInstance();
	URLRunManagerSubsystem* RunManager = GI->GetSubsystem<URLRunManagerSubsystem>();
	URLDataSubsystem* Data = GI->GetSubsystem<URLDataSubsystem>();
	if (!RunManager || !Data)
	{
		return;
	}

	AliveEnemies.RemoveAll([](const ARLEnemyBase* Enemy) { return !IsValid(Enemy) || Enemy->IsDead(); });
	if (AliveEnemies.Num() >= MaxAliveEnemies)
	{
		return;
	}

	const float Difficulty = RunManager->GetDifficultyCoefficient();

	// RoR2 wave commitment: pick ONE affordable card and ride it.
	const FName CardId = Data->DrawSpawnCard(
		RunManager->GetCurrentZoneIndex(), Difficulty, Credits, Rng);
	if (CardId == NAME_None)
	{
		return;	// can't afford anything yet; keep saving
	}

	const FRLSpawnCardRow* Card = Data->FindSpawnCard(CardId);
	UClass* EnemyClass = Card ? Card->EnemyClass.LoadSynchronous() : nullptr;
	if (!Card || !EnemyClass)
	{
		return;
	}

	// Elite promotion: the whole wave upgrades if the wallet supports 6x.
	float UnitCost = Card->Cost;
	bool bEliteWave = false;
	if (Credits >= Card->Cost * RLCombat::EliteCostMultiplier && Rng.FRand() < EliteWaveChance)
	{
		bEliteWave = true;
		UnitCost = Card->Cost * RLCombat::EliteCostMultiplier;
	}

	// Spend until broke, capped per wave and by the alive limit.
	int32 SpawnedThisWave = 0;
	while (Credits >= UnitCost
		&& SpawnedThisWave < MaxSpawnsPerWave
		&& AliveEnemies.Num() < MaxAliveEnemies)
	{
		FVector SpawnLocation;
		if (!FindSpawnPoint(SpawnLocation))
		{
			break;
		}

		const FTransform SpawnTransform(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), SpawnLocation);
		ARLEnemyBase* Enemy = GetWorld()->SpawnActorDeferred<ARLEnemyBase>(
			EnemyClass, SpawnTransform, /*Owner=*/nullptr, /*Instigator=*/nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Enemy)
		{
			break;
		}

		// Deferred spawn so bElite is set before BeginPlay scales stats.
		Enemy->bElite = bEliteWave;
		Enemy->FinishSpawning(SpawnTransform);

		Credits -= UnitCost;
		AliveEnemies.Add(Enemy);
		++SpawnedThisWave;
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
