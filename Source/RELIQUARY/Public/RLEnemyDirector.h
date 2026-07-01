// RELIQUARY — the enemy director, modeled on Risk of Rain 2's combat
// director. It earns credits over time (faster as difficulty climbs) and
// spends them on spawn cards, occasionally promoting spawns to elites.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
	float SpawnInterval = 6.f;

	/** Hard cap on simultaneously alive director spawns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	int32 MaxAliveEnemies = 18;

	/** Spawn ring around the hero, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float MinSpawnDistance = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Director")
	float MaxSpawnDistance = 3200.f;

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
	FRandomStream Rng;

	UPROPERTY()
	TArray<TObjectPtr<ARLEnemyBase>> AliveEnemies;

	void TrySpend();
	bool FindSpawnPoint(FVector& OutLocation) const;
};
