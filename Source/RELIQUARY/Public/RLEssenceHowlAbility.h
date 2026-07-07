// RELIQUARY — Essence of the Alpha's active: a wolf's howl that demoralizes
// every nearby enemy. The first native essence active, proving the Q slot.

#pragma once

#include "CoreMinimal.h"
#include "RLGameplayAbility.h"
#include "RLEssenceHowlAbility.generated.h"

/**
 * A short-cooldown AoE shout. Enemies within Radius take a light hit and gain
 * State.Demoralized (they deal reduced damage) for DemoralizeSeconds — the
 * same debuff Reckless Abandon's shockwave applies, read by the damage
 * execution for free.
 *
 * Granted only from the major essence slot; set ActionTag = Ability.Essence so
 * the Q button fires it. Works with zero content; layer a montage/VFX on a BP
 * subclass via OnHowl.
 */
UCLASS()
class RELIQUARY_API URLEssenceHowlAbility : public URLGameplayAbility
{
	GENERATED_BODY()

public:
	URLEssenceHowlAbility();

	/** Enemies inside this radius are caught by the howl. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Howl")
	float Radius = 700.f;

	/** Demoralized duration on caught enemies. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Howl")
	float DemoralizeSeconds = 8.f;

	/** Lockout after firing (shortened by haste/trance). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Cooldown")
	float CooldownSeconds = 20.f;

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

	/** BP hook: play the howl montage/VFX/SFX. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Howl")
	void OnHowl();

protected:
	double LastEndTimeSeconds = -1000.0;
};
