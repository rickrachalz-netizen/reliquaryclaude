// RELIQUARY — base class for all hero and enemy abilities.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "RLGameplayAbility.generated.h"

/**
 * Every ability declares an ActionTag (usually one of Ability.Primary/
 * Secondary/Utility/Special). On activation the tag is reported to the
 * owning URLAbilitySystemComponent so Adaptability stacks can build.
 *
 * BaseDamage feeds the damage execution via SetByCaller.Damage; final damage
 * is shaped by the attacker's primary stat, crit, Adaptability, and the
 * victim's armor.
 */
UCLASS(Abstract)
class RELIQUARY_API URLGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	URLGameplayAbility();

	/** Which action this counts as for Adaptability and input binding. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY")
	FGameplayTag ActionTag;

	/** Base damage before stats/crit/adaptability. Scales per hero level. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY|Damage")
	FScalableFloat BaseDamage;

	/** Damage GameplayEffect (should use URLDamageExecution). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Flat mana cost paid on commit (0 = free). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY|Cost")
	float ManaCost = 0.f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/**
	 * Builds a damage spec (SetByCaller.Damage filled from BaseDamage at the
	 * ability level) and applies it to the target actor. Call from C++ or
	 * Blueprint ability graphs when a hit lands.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Damage")
	void ApplyDamageToTarget(AActor* Target, float DamageMultiplier = 1.f);

	/** Cooldown duration shortened by the owner's Haste. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Cooldown")
	float GetHastedDuration(float BaseSeconds) const;

protected:
	/** RoR2-style: player abilities snap to the camera's yaw, not run direction. */
	void FaceCameraDirection();
};
