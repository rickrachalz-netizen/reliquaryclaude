// RELIQUARY — the one damage formula for everything in the game.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "RLDamageExecution.generated.h"

/**
 * Action-RPG pipeline (math and tuning in RLCombatFormulas.h), in order:
 *
 *   Power       Physical: AP by class (Warrior Str x2, Rogue Str + Agi);
 *               Spell: SpellPower = Intellect
 *   Hit         Base + PowerCoefficient x Power (RoR2-style coefficients)
 *   Sanguinate  x(1 + Sanguination); attacker pays a slice of current
 *               health for the privilege
 *   Adapt       x(1 + Adaptability x stacks), 5 stacks of varied play;
 *               Synergy raises the per-stack value while crits chain
 *   Crit        chance = CritChance + Agi/Int-derived + Frenzy bonus;
 *               multiplier = CritDamage + Force (+ Frenzy Force bonus)
 *   Armor       Classic DR = Armor/(Armor + 400 + 85 x AttackerLevel),
 *               cap 75% — physical only, spells ignore armor
 *   Multistrike chance (+ Synergy) to echo the hit at 40% effectiveness
 *
 * After the hit resolves, the source ASC is notified so Hatred, Synergy,
 * and Frenzy state advance for the NEXT hit — "consecutive" effects never
 * retroactively buff the hit that built them.
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
