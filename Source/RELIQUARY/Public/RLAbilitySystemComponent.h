// RELIQUARY — ability system component with the live combat state behind
// the proc suite: Adaptability, Hatred, Synergy, and Frenzy stacks all
// live here and are read/updated by URLDamageExecution on every hit.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "RLAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRLDamageTakenSignature, float, Damage, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRLDeathSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRLFrenzySignature);

UCLASS()
class RELIQUARY_API URLAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxAdaptabilityStacks = 5;

	// --- Adaptability (varied actions) ---

	/** Called by URLGameplayAbility on activation. */
	void NotifyActionUsed(const FGameplayTag& ActionTag);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Combat")
	int32 GetAdaptabilityStacks() const { return AdaptabilityStacks; }

	// --- Hatred / Synergy / Frenzy (updated per damage instance) ---

	/**
	 * Called by the damage execution after each hit resolves. Updates
	 * Hatred (same-victim chains), Synergy (crit chains), and the Frenzy
	 * instance window. Instances > 1 when a multistrike echoed.
	 */
	void NotifyDamageDealt(AActor* Victim, bool bCrit, int32 Instances);

	/** Consecutive hits on the current victim (haste bonus = Hatred x this). */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Combat")
	int32 GetHatredStacks() const { return HatredStacks; }

	/** Extra haste from Hatred right now. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Combat")
	float GetHatredHasteBonus() const;

	/** Consecutive crits (buffs Multistrike/Hatred/Adaptability by Synergy x this). */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Combat")
	int32 GetSynergyStacks() const { return SynergyStacks; }

	/** True while the 3-hits-in-1-second payoff window is running. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Combat")
	bool IsFrenzied() const;

	// --- Ability activation ---

	/**
	 * Activates the granted ability whose ActionTag matches (Ability.Primary,
	 * Ability.Secondary, ...). This is how input reaches the kit.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Abilities")
	bool TryActivateByActionTag(FGameplayTag ActionTag);

	// --- Damage taken / death ---

	/** Called from the attribute set when IncomingDamage lands. */
	void HandleDamageTaken(float Damage, const FGameplayEffectContextHandle& Context, bool bLethal);

	/** Fired every time this actor takes post-mitigation damage. */
	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Combat")
	FRLDamageTakenSignature OnDamageTaken;

	/** Fired once when damage reduces health to zero. */
	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Combat")
	FRLDeathSignature OnDeath;

	/** Fired when Frenzy procs — hook VFX/audio here. */
	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Combat")
	FRLFrenzySignature OnFrenzyTriggered;

protected:
	// Adaptability
	FGameplayTag LastActionTag;
	int32 AdaptabilityStacks = 0;

	// Hatred
	TWeakObjectPtr<AActor> HatredVictim;
	int32 HatredStacks = 0;

	// Synergy
	int32 SynergyStacks = 0;

	// Frenzy
	TArray<double> RecentInstanceTimes;
	double FrenzyActiveUntil = -1.0;

	bool bDeathHandled = false;

	double NowSeconds() const;
};
