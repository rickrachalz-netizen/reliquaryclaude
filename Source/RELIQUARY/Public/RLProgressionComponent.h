// RELIQUARY — assembles the hero's permanent build on the ability system:
// class base stats and per-level growth, talent ranks, Reliquary Shard
// essences, and the granted ability kit. Everything here persists between
// runs; roguelike run power lives in URLRunPowerComponent instead.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "RLProgressionComponent.generated.h"

class UAbilitySystemComponent;

UCLASS(ClassGroup = (RELIQUARY), meta = (BlueprintSpawnableComponent))
class RELIQUARY_API URLProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Full rebuild from the active hero's saved data. Called on spawn and
	 * after any base-camp change (level up, talent spend, essence swap).
	 * Also fills health/mana to their new maximums.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Progression")
	void RebuildHero(UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Progression")
	void ClearAll(UAbilitySystemComponent* ASC);

protected:
	TArray<FActiveGameplayEffectHandle> AppliedEffects;
	TArray<FGameplayAbilitySpecHandle> GrantedAbilities;

	void GrantAbility(UAbilitySystemComponent* ASC, const TSoftClassPtr<class URLGameplayAbility>& AbilityClass, int32 Level);
};
