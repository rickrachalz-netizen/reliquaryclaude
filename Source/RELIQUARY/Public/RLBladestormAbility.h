// RELIQUARY — Bladestorm keystone: replaces the warrior primary.

#pragma once

#include "CoreMinimal.h"
#include "RLGameplayAbility.h"
#include "RLBladestormAbility.generated.h"

class UAnimMontage;

/**
 * Spin for TickCount seconds, damaging everything around the avatar once
 * per tick. The final tick grants State.BladestormEmpowered, which the
 * next Mortal Strike consumes for bonus damage.
 *
 * Parent GA_Bladestorm to this, set SpinMontage (AM_Bladestorm), and the
 * War_K2 keystone grants it over the primary slot.
 */
UCLASS()
class RELIQUARY_API URLBladestormAbility : public URLGameplayAbility
{
	GENERATED_BODY()

public:
	URLBladestormAbility();

	/** Spin montage; the ability still ticks without one. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Bladestorm")
	TObjectPtr<UAnimMontage> SpinMontage;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Bladestorm")
	float MontagePlayRate = 1.f;

	/** Damage ticks (BaseDamage each), one per TickInterval. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Bladestorm")
	int32 TickCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Bladestorm")
	float TickInterval = 1.f;

	/** Damage radius around the avatar. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Bladestorm")
	float Radius = 300.f;

	/** Lockout after the spin ends (shortened by haste/trance). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Bladestorm")
	float CooldownSeconds = 6.f;

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

	/** BP hook: per-tick VFX/SFX. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Bladestorm")
	void OnSpinTick(int32 TickIndex);

protected:
	int32 TicksDone = 0;
	FTimerHandle TickTimerHandle;
	double LastEndTimeSeconds = -1000.0;

	void DoSpinTick();

	UFUNCTION()
	void HandleMontageCancelled();
};
