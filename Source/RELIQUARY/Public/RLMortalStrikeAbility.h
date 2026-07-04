// RELIQUARY — warrior Secondary: one slow, heavy strike on a real cooldown.

#pragma once

#include "CoreMinimal.h"
#include "RLMeleeAttackAbility.h"
#include "RLMortalStrikeAbility.generated.h"

/**
 * Mortal Strike. Consumes Exposed stacks left by the primary finisher
 * (+50% damage each), executes low-health targets, applies Mortal Wounds
 * (healing cut), and stuns when the War_T3A talent is owned. Consumes the
 * Bladestorm empowerment for a big bonus.
 *
 * Add a single montage to ComboMontages on a BP subclass for the swing
 * animation; works instantly without one.
 */
UCLASS()
class RELIQUARY_API URLMortalStrikeAbility : public URLMeleeAttackAbility
{
	GENERATED_BODY()

public:
	URLMortalStrikeAbility();

	/** Below this health fraction the strike executes. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|MortalStrike")
	float ExecuteThresholdFraction = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|MortalStrike")
	float ExecuteMultiplier = 1.5f;

	/** Bonus damage per consumed Exposed stack. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|MortalStrike")
	float ExposedBonusPerStack = 0.5f;

	/** Multiplier when consuming Bladestorm's empowerment (+150%). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|MortalStrike")
	float EmpoweredMultiplier = 2.5f;

	/** Mortal Wounds duration on victims. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|MortalStrike")
	float MortalWoundsSeconds = 10.f;

	/** Stun applied when the hero owns War_T3A (Second Wind). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|MortalStrike")
	float StunSeconds = 2.f;

	virtual float GetVictimDamageMultiplier(AActor* Victim, float BaseMultiplier) override;
	virtual void NotifyVictimHit(AActor* Victim) override;
};
