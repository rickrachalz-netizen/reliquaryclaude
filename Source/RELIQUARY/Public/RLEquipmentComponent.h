// RELIQUARY — applies the hero's equipped gear to their ability system:
// item stats, cantrip effects, and set bonuses.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "RLTypes.h"
#include "RLEquipmentComponent.generated.h"

class UAbilitySystemComponent;

UCLASS(ClassGroup = (RELIQUARY), meta = (BlueprintSpawnableComponent))
class RELIQUARY_API URLEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Reads the active hero's Equipped map from the game instance and applies
	 * everything: per-item stats, cantrip GameplayEffects, and any set bonus
	 * thresholds met by worn pieces. Safe to call repeatedly (re-equip,
	 * arrival on a new map) — previous applications are removed first.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Equipment")
	void RefreshEquipment(UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Equipment")
	void ClearEquipment(UAbilitySystemComponent* ASC);

protected:
	TArray<FActiveGameplayEffectHandle> AppliedHandles;
};
