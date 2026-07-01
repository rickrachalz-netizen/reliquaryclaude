// RELIQUARY — ability system component with Adaptability tracking and
// combat event broadcasting.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "RLAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRLDamageTakenSignature, float, Damage, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRLDeathSignature);

/**
 * Adaptability: abilities report their action tag here when they activate.
 * Using a different action than your last one builds a stack (up to 5);
 * repeating your last action resets to zero. The damage execution reads the
 * stack count and multiplies damage by (1 + Adaptability * Stacks).
 */
UCLASS()
class RELIQUARY_API URLAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxAdaptabilityStacks = 5;

	/** Called by URLGameplayAbility on activation. */
	void NotifyActionUsed(const FGameplayTag& ActionTag);

	/**
	 * Activates the granted ability whose ActionTag matches (Ability.Primary,
	 * Ability.Secondary, ...). This is how input reaches the kit.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Abilities")
	bool TryActivateByActionTag(FGameplayTag ActionTag);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Combat")
	int32 GetAdaptabilityStacks() const { return AdaptabilityStacks; }

	/** Called from the attribute set when IncomingDamage lands. */
	void HandleDamageTaken(float Damage, const FGameplayEffectContextHandle& Context, bool bLethal);

	/** Fired every time this actor takes post-mitigation damage. */
	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Combat")
	FRLDamageTakenSignature OnDamageTaken;

	/** Fired once when damage reduces health to zero. */
	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Combat")
	FRLDeathSignature OnDeath;

protected:
	FGameplayTag LastActionTag;
	int32 AdaptabilityStacks = 0;
	bool bDeathHandled = false;
};
