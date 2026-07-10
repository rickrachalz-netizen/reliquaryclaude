// RELIQUARY — a physical excess-mana orb. Nodes and enemies burst into these
// (V Rising style); they magnet to the hero and add to the run's mana wallet.

#pragma once

#include "CoreMinimal.h"
#include "RLResourcePickup.h"
#include "RLTypes.h"
#include "RLManaOrbPickup.generated.h"

/**
 * Reuses ARLResourcePickup's magnet + overlap plumbing, but collecting one
 * adds ManaAmount to URLRunManagerSubsystem's excess mana instead of the run
 * inventory. Spawn a reward's worth with SpawnBurst, which chunks the total
 * into orbs and scatters them like a shattered node.
 */
UCLASS()
class RELIQUARY_API ARLManaOrbPickup : public ARLResourcePickup
{
	GENERATED_BODY()

public:
	ARLManaOrbPickup();

	/** Excess mana granted when this orb is collected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Mana")
	int32 ManaAmount = RLBalance::ManaOrbUnitValue;

	/**
	 * Spawns TotalMana worth of orbs scattered around Origin, chunked into
	 * pieces of RLBalance::ManaOrbUnitValue (the remainder rides the last orb).
	 * No-op for non-positive totals or a null world.
	 *
	 * OrbClass picks the orb to spawn. When null (every existing call site),
	 * the burst looks for a BP_ManaOrb at /Game/Resources/BP_ManaOrb — author
	 * one there with a mesh/VFX and every burst in the game uses it, no wiring
	 * needed. Missing Blueprint falls back to this (invisible) native class.
	 */
	static void SpawnBurst(UWorld* World, const FVector& Origin, int32 TotalMana,
		TSubclassOf<ARLManaOrbPickup> OrbClass = nullptr);

protected:
	virtual void GrantTo(AActor* Collector) override;
};
