#include "RLEnemyDirector.h"
#include "RLEnemyBase.h"
#include "RLEnemyGroup.h"
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

	// Spend in a burst until the wallet or the cap runs out. Cards are drawn
	// without an affordability filter: when the draw lands on a pack the
	// wallet can't cover yet, the director banks toward it (RoR2-style
	// saving) instead of trickling out cheap spawns forever.
	int32 SafetyCounter = 8;
	while (SafetyCounter-- > 0 && AliveEnemies.Num() < MaxAliveEnemies)
	{
		const FName CardId = Data->DrawSpawnCard(
			RunManager->GetCurrentZoneIndex(), Difficulty, MAX_flt, Rng);
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

		// Pack cards roll a headcount; cost is per member.
		const int32 PackSizeMin = FMath::Max(1, Card->PackSizeMin);
		const int32 PackSize = Rng.RandRange(PackSizeMin, FMath::Max(PackSizeMin, Card->PackSizeMax));
		if (AliveEnemies.Num() + PackSize > MaxAliveEnemies)
		{
			break;	// wait for room rather than spawning a crippled pack
		}

		// Elite promotion grows more likely as the run drags on; a promoted
		// pack goes elite as a whole, so it must be affordable as a whole.
		const float EliteChance = FMath::Clamp((Difficulty - 1.5f) * 0.08f, 0.f, 0.3f);
		float TotalCost = Card->Cost * PackSize;
		const bool bElite = Rng.FRand() < EliteChance && Credits >= TotalCost * EliteCostMultiplier;
		if (bElite)
		{
			TotalCost *= EliteCostMultiplier;
		}
		if (Credits < TotalCost)
		{
			break;	// bank toward this card
		}

		FVector SpawnLocation;
		if (!FindSpawnPoint(SpawnLocation))
		{
			break;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		// Pack cards get a coordinator that owns the group behavior
		// (wolf packs surround and lunge, goblin gangs circle up and roam).
		ARLEnemyGroup* Group = nullptr;
		if (UClass* GroupClass = Card->GroupClass.IsNull() ? nullptr : Card->GroupClass.LoadSynchronous())
		{
			Group = GetWorld()->SpawnActor<ARLEnemyGroup>(GroupClass, SpawnLocation, FRotator::ZeroRotator, Params);
			if (Group)
			{
				Group->InitSeed(Rng.NextUInt64());
			}
		}

		int32 Spawned = 0;
		for (int32 i = 0; i < PackSize; ++i)
		{
			FVector MemberLocation = SpawnLocation;
			if (PackSize > 1)
			{
				// Fan members around the spawn point so packs land as a clump.
				const float Angle = (2.f * PI * i) / PackSize + Rng.FRandRange(-0.3f, 0.3f);
				const float Ring = Rng.FRandRange(140.f, 260.f);
				MemberLocation += FVector(FMath::Cos(Angle) * Ring, FMath::Sin(Angle) * Ring, 0.f);
			}

			ARLEnemyBase* Enemy = GetWorld()->SpawnActor<ARLEnemyBase>(
				EnemyClass, MemberLocation, FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), Params);
			if (!Enemy)
			{
				continue;
			}

			Enemy->bElite = bElite;
			Enemy->EnemyTypeId = CardId;	// drives first-kill essence shard drops
			if (Group)
			{
				Group->RegisterMember(Enemy);
			}
			AliveEnemies.Add(Enemy);
			++Spawned;
		}

		if (Spawned == 0)
		{
			break;	// a memberless group destroys itself on its first tick
		}

		// Charge only for what actually spawned.
		Credits -= Card->Cost * Spawned * (bElite ? EliteCostMultiplier : 1.f);
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
