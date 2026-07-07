// RELIQUARY — warrior Special: Reckless Abandon, a hold-to-charge launch.

#pragma once

#include "CoreMinimal.h"
#include "RLGameplayAbility.h"
#include "RLRecklessAbandonAbility.generated.h"

class UCameraShakeBase;
class USoundBase;

/**
 * Hold the Special input to dig in and charge, release to launch along the
 * full 3D camera aim (Loader's gauntlet feel: aim up and it flings you into
 * the air). Held time scales distance and damage; releasing early is a
 * shorter, weaker charge. Usable midair at reduced range.
 *
 * Walkable ground never stops the punch — the dash skims along floors and
 * rides up ramps. Objects do:
 *  - Enemy: heavy damage and hitstun, Crushed Armor on the victim (takes
 *    increased damage, long duration), then AoE damage around the impact and
 *    Demoralizing Shout (deals reduced damage) on every enemy caught.
 *  - Wall/tree/rock: the direct-hit damage lands on destructibles.
 *  - Either collision recoils the warrior: a cut of current HP, a brief
 *    self-slow, and a small bounce up-and-backward off the impact.
 * A clean whiff costs nothing and keeps its momentum — the warrior sails on
 * with dash velocity and arcs down ballistically.
 *
 * Works fully without content; subclass in BP for montages/VFX on the hooks.
 */
UCLASS()
class RELIQUARY_API URLRecklessAbandonAbility : public URLGameplayAbility
{
	GENERATED_BODY()

public:
	URLRecklessAbandonAbility();

	// --- Charge ---

	/** Held seconds for a full-power charge; releasing sooner scales down. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Charge")
	float MaxChargeSeconds = 1.f;

	/** The dig-in always plays at least this long, even on a tap. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Charge")
	float MinWindupSeconds = 0.25f;

	/** Damage multiplier at zero charge (full charge is 1). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Charge")
	float MinDamageMultiplier = 0.4f;

	// --- Dash ---

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Dash")
	float MinDashDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Dash")
	float MaxDashDistance = 2400.f;

	/** Distance scale when the charge starts midair. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Dash")
	float AirDistanceMultiplier = 0.6f;

	/** Dash speed, cm/s. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Dash")
	float DashSpeed = 3000.f;

	/** Velocity kept when the dash ends without a collision (the fling). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Dash")
	float MomentumCarryFraction = 1.f;

	/** Deflect along walkable ground instead of treating it as a wall. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Dash")
	bool bSkimWalkableGround = true;

	/** Surfaces with an impact normal Z at or above this count as ground. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Dash")
	float SkimFloorNormalZ = 0.7f;

	// --- Impact ---

	/** Hard CC on the enemy the dash collides with. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Impact")
	float HitStunSeconds = 1.5f;

	/** Crushed Armor duration on the collision victim. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Impact")
	float CrushedArmorSeconds = 45.f;

	/** AoE radius around the collision point. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Impact")
	float AoERadius = 600.f;

	/** AoE damage as a fraction of the direct hit. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Impact")
	float AoEDamageMultiplier = 0.25f;

	/** Demoralizing Shout duration on enemies caught in the AoE. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Impact")
	float DemoralizeSeconds = 10.f;

	// --- Recoil (any collision: enemy or wall) ---

	/** Fraction of CURRENT health taken as recoil damage. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Recoil")
	float RecoilCurrentHealthFraction = 0.05f;

	/** Walk-speed multiplier while the recoil slow lasts. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Recoil")
	float RecoilSlowMultiplier = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Recoil")
	float RecoilSlowSeconds = 1.5f;

	/** Bounce off an impact: backward speed, opposite the dash. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Recoil")
	float ImpactBounceBackSpeed = 400.f;

	/** Bounce off an impact: upward pop. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Recoil")
	float ImpactBounceUpSpeed = 350.f;

	// --- Feel ---

	/** Seconds attacker and victim freeze on enemy impact (0 disables). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Feel")
	float HitStopSeconds = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Feel")
	TSubclassOf<UCameraShakeBase> ImpactCameraShake;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Feel")
	TObjectPtr<USoundBase> ImpactSound;

	/** Lockout after the ability ends (shortened by haste/trance). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Cooldown")
	float CooldownSeconds = 12.f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual float GetCooldownRemaining() const override;
	virtual float GetCooldownDuration() const override;

	/** 0..1 while charging (drive a HUD meter or glow from this). */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|RecklessAbandon")
	float GetChargeAlpha() const;

	// --- BP hooks for montages/VFX/SFX ---

	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|RecklessAbandon")
	void OnChargeStarted();

	/** Fired ~20x/s while the button is held. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|RecklessAbandon")
	void OnChargeTick(float Alpha);

	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|RecklessAbandon")
	void OnDashStarted(float Alpha);

	/** Victim is the collided enemy, or null for a wall/destructible hit. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|RecklessAbandon")
	void OnChargeImpact(AActor* Victim, FVector ImpactLocation);

protected:
	// Charge state
	bool bCharging = false;
	double ChargeStartTimeSeconds = 0.0;
	bool bMovementLocked = false;

	// Dash state
	bool bDashing = false;
	FVector DashDirection = FVector::ForwardVector;
	FVector LastDashLocation = FVector::ZeroVector;
	double DashStartTimeSeconds = 0.0;
	float DashDistanceTraveled = 0.f;
	float DashDistanceBudget = 0.f;
	float DashDamageMultiplier = 1.f;

	FTimerHandle ChargeTickTimerHandle;
	FTimerHandle WindupTimerHandle;
	FTimerHandle DashTickTimerHandle;
	double LastEndTimeSeconds = -1000.0;

	void DoChargeTick();

	/** Release path: waits out any remaining windup, then dashes. */
	void LaunchDash(float Alpha);

	/** ~120Hz while dashing: keeps velocity, sweeps for the first collision. */
	void DoDashTick();

	/** Full package: damage, hitstun, Crushed Armor, AoE + Shout, recoil. */
	void ResolveEnemyImpact(AActor* Victim);

	/** Wall/destructible: direct damage to destructibles, recoil, stop. */
	void ResolveObstacleImpact(AActor* HitActor, const FVector& ImpactLocation);

	/** 5% current HP + the brief self-slow. */
	void ApplyRecoil();

	/** Hit-stop + camera shake + impact sound. */
	void ApplyImpactFeedback(AActor* Victim);

	/** Halts the dash; an impact bounce pops the warrior up and backward. */
	void StopDashMovement(bool bImpactBounce);

	/** Whiff end: hand the dash velocity over to gravity (the fling). */
	void EndDashWithMomentum();

	void EndSelf(bool bWasCancelled);
};
