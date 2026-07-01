// RELIQUARY — temporary roguelike power. Boons bought at upgrade altars are
// applied here and exist only for the current run; the run manager holds the
// authoritative stack counts so power survives zone-to-zone travel, and all
// of it evaporates at base camp.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "RLRunPowerComponent.generated.h"

class UAbilitySystemComponent;

UCLASS(ClassGroup = (RELIQUARY), meta = (BlueprintSpawnableComponent))
class RELIQUARY_API URLRunPowerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Applies one new stack of a boon and records it with the run manager. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Boons")
	bool ApplyBoon(UAbilitySystemComponent* ASC, FName BoonId);

	/**
	 * Re-applies every recorded boon stack after the hero respawns on a new
	 * zone map. Call once during character initialization while on a run.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Boons")
	void RestoreFromRunManager(UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Boons")
	void ClearAll(UAbilitySystemComponent* ASC);

protected:
	TArray<FActiveGameplayEffectHandle> AppliedHandles;

	bool ApplyBoonInternal(UAbilitySystemComponent* ASC, FName BoonId, bool bRecord);
};
