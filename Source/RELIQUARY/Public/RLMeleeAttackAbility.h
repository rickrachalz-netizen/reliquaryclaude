// RELIQUARY — a crunchy melee swing usable by heroes and enemies.

#pragma once

#include "CoreMinimal.h"
#include "RLGameplayAbility.h"
#include "RLMeleeAttackAbility.generated.h"

class UAnimMontage;
class UCameraShakeBase;
class USoundBase;

/**
 * With no montages assigned this is the day-one instant swing: sweep a
 * sphere in front of the avatar and damage everything hostile it touches.
 *
 * Assign ComboMontages on a Blueprint subclass and it becomes a montage-
 * driven combo chain: damage fires on the "RL Melee Hit" anim notify,
 * re-pressing the input mid-swing buffers the next stage, and contact
 * triggers hit-stop, knockback, camera shake, and sound.
 */
UCLASS()
class RELIQUARY_API URLMeleeAttackAbility : public URLGameplayAbility
{
	GENERATED_BODY()

public:
	URLMeleeAttackAbility();

	/** Reach of the swing from the avatar's location, in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Melee")
	float Range = 220.f;

	/** Radius of the hit sphere, in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Melee")
	float Radius = 120.f;

	/**
	 * Combo chain, played first to last while the input keeps coming.
	 * Each montage should carry one "RL Melee Hit" notify at its impact
	 * frame. Leave empty for the animation-less instant swing.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combo")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

	/** Damage multiplier on the chain's final swing. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combo")
	float FinisherMultiplier = 1.5f;

	/** Montage play rate; raise for snappier swings. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combo")
	float PlayRate = 1.f;

	/** Seconds attacker and victims freeze on contact (0 disables). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Feel")
	float HitStopSeconds = 0.05f;

	/** Shove applied to victims on contact (0 disables). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Feel")
	float KnockbackStrength = 300.f;

	/** Camera shake for the owning player on contact (optional). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Feel")
	TSubclassOf<UCameraShakeBase> HitCameraShake;

	/** Impact sound played at the first victim (optional). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Feel")
	TObjectPtr<USoundBase> HitSound;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/** Hook for BP subclasses: fired per victim hit. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Melee")
	void OnMeleeHit(AActor* Victim);

protected:
	int32 ComboIndex = 0;
	bool bComboQueued = false;
	bool bHitFiredThisSwing = false;

	/** Starts the montage for the current ComboIndex. */
	void PlayComboStage();

	/** Sphere sweep + damage + feel feedback for the current swing. */
	void DoSweep();

	void ApplyHitFeedback(AActor* Avatar, const TArray<AActor*>& Victims);

	UFUNCTION()
	void HandleHitEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleComboEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleStageBlendOut();

	UFUNCTION()
	void HandleStageCancelled();
};
