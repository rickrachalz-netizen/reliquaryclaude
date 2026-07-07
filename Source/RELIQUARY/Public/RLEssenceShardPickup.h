// RELIQUARY — an essence shard dropped by the first kill of an enemy type.
// Collecting it permanently unlocks that essence for the active hero.

#pragma once

#include "CoreMinimal.h"
#include "RLResourcePickup.h"
#include "RLEssenceShardPickup.generated.h"

/**
 * Spawned by ARLEnemyBase::GrantRewards when an enemy type dies whose essence
 * the hero hasn't learned yet. Collection routes straight to persistent hero
 * data (URLGameInstance::UnlockEssence — saved immediately), never the run
 * inventory, so an unlock can't be forfeited by dying later in the run. If
 * the shard is left behind, the next first-kill check simply drops it again.
 */
UCLASS()
class RELIQUARY_API ARLEssenceShardPickup : public ARLResourcePickup
{
	GENERATED_BODY()

public:
	ARLEssenceShardPickup();

	/** Essence row this shard teaches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Essence")
	FName EssenceId = NAME_None;

protected:
	virtual void GrantTo(AActor* Collector) override;
};
