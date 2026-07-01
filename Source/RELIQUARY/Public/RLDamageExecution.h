// RELIQUARY — the one damage formula for everything in the game.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "RLDamageExecution.generated.h"

/**
 * Damage pipeline (kept deliberately simple and legible):
 *
 *   Base       = SetByCaller.Damage (from the ability)
 *   Stat scale = 1 + HighestPrimaryStat / 100
 *   Adapt      = 1 + Adaptability * AdaptabilityStacks   (max 5 stacks)
 *   Crit       = CritDamage multiplier on a CritChance roll
 *   Mitigation = 1 - Armor / (Armor + 300)
 *
 * Result is written to the target's IncomingDamage meta attribute, which
 * URLAttributeSet converts into a Health loss and death handling.
 */
UCLASS()
class RELIQUARY_API URLDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	URLDamageExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
