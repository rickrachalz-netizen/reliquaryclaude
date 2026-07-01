// RELIQUARY — turns FRLStatBlock data into live GAS modifiers. This is the
// single bridge between authored CSV numbers and the attribute set, used by
// gear, set bonuses, talents, essences, and run boons alike.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffectTypes.h"
#include "RLDataTypes.h"
#include "RLStatLibrary.generated.h"

class UAbilitySystemComponent;

UCLASS()
class RELIQUARY_API URLStatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Applies every non-zero field of the block as an additive, infinite
	 * modifier via a transient GameplayEffect. Returns the active handle so
	 * the caller can remove it later (unequip, respec, run reset).
	 */
	static FActiveGameplayEffectHandle ApplyStatBlock(UAbilitySystemComponent* ASC,
		const FRLStatBlock& Block, const FString& DebugName);

	/**
	 * Writes the block into attribute base values (class base + level growth).
	 * Unlike ApplyStatBlock this is absolute, not additive-on-top.
	 */
	static void SetBaseStats(UAbilitySystemComponent* ASC, const FRLStatBlock& Block);

	/** Fills Health/Mana to their maximums (fresh spawn, level up). */
	static void FillVitals(UAbilitySystemComponent* ASC);
};
