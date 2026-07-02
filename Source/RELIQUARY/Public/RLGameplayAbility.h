// RELIQUARY — base class for all hero and enemy abilities.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ScalableFloat.h"
#include "RLGameplayAbility.generated.h"

/** Which half of the Classic WoW damage pipeline a hit uses. */
UENUM(BlueprintType)
enum class ERLDamageSchool : uint8
{
	/** Weapon damage + AP/14 x WeaponSpeed; mitigated by armor. */
	Physical,
	/** Base + SpellPower x (CastTime / 3.5); ignores armor. */
	Spell
};

/**
 * Every ability declares an ActionTag (usually one of Ability.Primary/
 * Secondary/Utility/Special). On activation the tag is reported to the
 * owning URLAbilitySystemComponent so Adaptability stacks can build.
 *
 * Damage follows Classic WoW: BaseDamage is the weapon/spell base roll,
 * then attack power (or the spell coefficient), crit, Adaptability, and
 * the victim's armor shape the final number in URLDamageExecution.
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

	/** Weapon/spell base damage before the WoW pipeline. Scales per level. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY|Damage")
	FScalableFloat BaseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY|Damage")
	ERLDamageSchool DamageSchool = ERLDamageSchool::Physical;

	/** Physical only: swing speed for the AP/14 formula (Classic 2H ~3.5). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY|Damage",
		meta = (EditCondition = "DamageSchool == ERLDamageSchool::Physical"))
	float WeaponSpeed = 2.4f;

	/** Spell only: cast time for the CastTime/3.5 coefficient (1.5 = instant). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY|Damage",
		meta = (EditCondition = "DamageSchool == ERLDamageSchool::Spell"))
	float CastTime = 1.5f;

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
	 * Builds a damage spec carrying the Classic-WoW pipeline inputs
	 * (SetByCaller Damage + WeaponSpeed or CastTime, per DamageSchool).
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
