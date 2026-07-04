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

	/**
	 * Presses buffer the next stage during the whole swing; this adds extra
	 * seconds after a swing ends where a late press still chains instead of
	 * restarting the combo.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combo")
	float ComboGraceSeconds = 0.2f;

	/** Seconds after the chain ends before the first swing can start again. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combo")
	float ComboCooldownSeconds = 0.f;

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

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Hook for BP subclasses: fired per victim hit. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Melee")
	void OnMeleeHit(AActor* Victim);

protected:
	int32 ComboIndex = 0;
	bool bComboQueued = false;
	bool bHitFiredThisSwing = false;
	bool bAwaitingGrace = false;
	FTimerHandle GraceTimerHandle;
	double ComboEndTimeSeconds = -1000.0;

	/** Starts the montage for the current ComboIndex. */
	void PlayComboStage();

	/** Sphere sweep + damage + feel feedback for the current swing. */
	void DoSweep();

	/** Plays the next stage if one exists, otherwise ends the combo. */
	void AdvanceOrEnd();

	void FinishGraceWindow();
	void EndCombo();

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
