// RELIQUARY — base class for all hero and enemy abilities.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ScalableFloat.h"
#include "RLGameplayAbility.generated.h"

/** Which power stat a hit scales from, and whether armor applies. */
UENUM(BlueprintType)
enum class ERLDamageSchool : uint8
{
	/** Scales from attack power; mitigated by armor. */
	Physical,
	/** Scales from spell power; ignores armor. */
	Spell
};

/**
 * Every ability declares an ActionTag (usually one of Ability.Primary/
 * Secondary/Utility/Special). On activation the tag is reported to the
 * owning URLAbilitySystemComponent so Adaptability stacks can build.
 *
 * Damage is coefficient-based, Risk of Rain 2 style: a hit deals
 * BaseDamage + PowerCoefficient x Power, then the proc suite (crit,
 * Adaptability, Sanguination, Multistrike...) and the victim's armor
 * shape the final number in URLDamageExecution.
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

	/** Flat base damage so level-1 hits still land. Scales per level. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY|Damage")
	FScalableFloat BaseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY|Damage")
	ERLDamageSchool DamageSchool = ERLDamageSchool::Physical;

	/**
	 * How hard this ability scales off your power stat, RoR2 style:
	 * bread-and-butter ~0.8, heavy hit ~2.0, big payoff 3.0+.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY|Damage")
	float PowerCoefficient = 1.f;

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
	 * Builds a damage spec carrying the pipeline inputs (SetByCaller
	 * Damage + the school's PowerCoefficient).
	 */
	FGameplayEffectSpecHandle MakeDamageSpec(float DamageMultiplier = 1.f) const;

	/**
	 * Builds a damage spec and applies it to the target actor. Call from
	 * C++ or Blueprint ability graphs when a hit lands.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Damage")
	void ApplyDamageToTarget(AActor* Target, float DamageMultiplier = 1.f);

	/** Cooldown duration shortened by the owner's Haste. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Cooldown")
	float GetHastedDuration(float BaseSeconds) const;
};
