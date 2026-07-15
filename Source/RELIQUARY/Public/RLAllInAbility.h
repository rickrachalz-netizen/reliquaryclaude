// RELIQUARY — "All in!", the warrior's Utility: a defiant stance that parries
// every incoming attack and floods health regen for a few seconds.

#pragma once

#include "CoreMinimal.h"
#include "RLGameplayAbility.h"
#include "RLAllInAbility.generated.h"

/**
 * Instant self-buff (Die by the Sword × Frenzied Regeneration): for
 * BuffDurationSeconds the hero gains State.Parry — the damage execution
 * negates every incoming hit — and health regen is multiplied by
 * RegenMultiplier (folded into the character's Tick regen, stacking
 * multiplicatively with the Battle Trance healing bonus).
 *
 * Works with zero content; layer a montage/VFX on a BP subclass via OnAllIn.
 */
UCLASS()
class RELIQUARY_API URLAllInAbility : public URLGameplayAbility
{
	GENERATED_BODY()

public:
	URLAllInAbility();

	/** How long the parry + regen surge lasts. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|AllIn")
	float BuffDurationSeconds = 5.f;

	/** Health regen is multiplied by this while the buff is up (3 = +200%). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|AllIn")
	float RegenMultiplier = 3.f;

	/** Lockout after firing (shortened by haste/trance). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Cooldown")
	float CooldownSeconds = 25.f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual float GetCooldownRemaining() const override;
	virtual float GetCooldownDuration() const override;

	/** BP hook: play the stance montage/VFX/SFX. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|AllIn")
	void OnAllIn();

protected:
	double LastEndTimeSeconds = -1000.0;
};
