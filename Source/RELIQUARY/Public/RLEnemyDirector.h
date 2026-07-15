// RELIQUARY — the enemy director, modeled on Risk of Rain 2's combat
// director. It earns credits over time (faster as difficulty climbs) and
// spends them on spawn cards, occasionally promoting spawns to elites.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLXoshiro.h"
#include "RLEnemyDirector.generated.h"

class ARLEnemyBase;

UCLASS()
class RELIQUARY_API ARLEnemyDirector : public AActor
{
	GENERATED_BODY()

public:
	ARLEnemyDirector();

	/** Seconds between spend attempts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float SpawnInterval = 8.f;

	/** Global income scale — 0.75 means 25% fewer enemies over a run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float CreditIncomeScale = 0.75f;

	/** Hard cap on simultaneously alive director spawns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	int32 MaxAliveEnemies = 18;

	/**
	 * Spawn ring around the hero, in cm. Far enough out that packs are
	 * stumbled upon mid-roam or glimpsed in the distance, not dropped on
	 * the hero's head.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float MinSpawnDistance = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float MaxSpawnDistance = 5200.f;

	/** Elites cost this multiple of the card price. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float EliteCostMultiplier = 3.f;

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Director")
	float GetCredits() const { return Credits; }

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Director")
	int32 GetAliveCount() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	float Credits = 0.f;
	float TimeSinceSpend = 0.f;
	FRLXoshiro256 Rng;

	UPROPERTY()
	TArray<TObjectPtr<ARLEnemyBase>> AliveEnemies;

	void TrySpend();
	bool FindSpawnPoint(FVector& OutLocation) const;
};
