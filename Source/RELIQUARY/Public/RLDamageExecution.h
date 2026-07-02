// RELIQUARY — the one damage formula for everything in the game,
// modeled on Classic World of Warcraft.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "RLDamageExecution.generated.h"

/**
 * Classic WoW damage pipeline (see RLCombatFormulas.h for the math):
 *
 * PHYSICAL (SetByCaller.WeaponSpeed present):
 *   AP         = Str x2 (Warrior/default) or Str + Agi (Rogue)
 *   Swing      = WeaponDamage + AP/14 x WeaponSpeed
 *   Crit       = 5% base (class CSV) + gear + 1% per 20 Agility -> x2.0
 *   Mitigation = Armor / (Armor + 400 + 85 x AttackerLevel), cap 75%
 *
 * SPELL (SetByCaller.CastTime present):
 *   SpellPower = Intellect (RELIQUARY adaptation) + gear
 *   Hit        = Base + SpellPower x (CastTime / 3.5)
 *   Crit       = 5% base + gear + 1% per 60 Intellect -> x1.5
 *   Ignores armor entirely.
 *
 * Adaptability (our signature stat) multiplies either school afterward:
 * x(1 + Adaptability x stacks), up to five stacks of varied play.
 *
 * Result lands on the target's IncomingDamage meta attribute, which
 * URLAttributeSet converts into Health loss and death handling.
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
