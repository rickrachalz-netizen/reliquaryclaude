// RELIQUARY — the enemy director, running Risk of Rain 2's combat director
// algorithm:
//
//   - Credits accrue continuously, scaling with the difficulty coefficient:
//     credits/sec = CreditMultiplier x (1 + 0.4 x coeff)
//   - Spending happens in WAVES on a randomized interval. Each wave the
//     director picks ONE affordable spawn card (weighted) and commits to
//     it, spawning copies until the wallet or the alive-cap runs out.
//   - If it can afford 6x the card's cost, it may promote the wave to
//     tier-1 elites: 6x cost for 4x HP / 2x damage.
//
// Two director profiles ship in RoR2 (fast, small waves / slow, big
// waves); spawn one of each from the game mode or tune the intervals on
// a single instance.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "RLEnemyDirector.generated.h"

class ARLEnemyBase;

UCLASS()
class RELIQUARY_API ARLEnemyDirector : public AActor
{
	GENERATED_BODY()

public:
	ARLEnemyDirector();

	/** Scales credit income; the zone's DirectorCreditRate multiplies this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float CreditMultiplier = 0.75f;

	/** Wave timing (RoR2 fast director: 7.5-10s; slow director: 22.5-30s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float MinWaveInterval = 7.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float MaxWaveInterval = 10.f;

	/** Chance an affordable wave gets promoted to elites. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float EliteWaveChance = 0.25f;

	/** Hard cap on simultaneously alive director spawns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	int32 MaxAliveEnemies = 18;

	/** Max spawns in a single wave burst. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	int32 MaxSpawnsPerWave = 6;

	/** Spawn ring around the hero, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float MinSpawnDistance = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float MaxSpawnDistance = 3200.f;

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Director")
	float GetCredits() const { return Credits; }

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Director")
	int32 GetAliveCount() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	float Credits = 0.f;
	float NextWaveIn = 0.f;
	float ZoneCreditScale = 1.f;
	FRandomStream Rng;

	UPROPERTY()
	TArray<TObjectPtr<ARLEnemyBase>> AliveEnemies;

	void SpendWave();
	void ScheduleNextWave();
	bool FindSpawnPoint(FVector& OutLocation) const;
};
