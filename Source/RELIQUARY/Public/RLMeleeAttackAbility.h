// RELIQUARY — a crunchy, instant melee swing usable by heroes and enemies.

#pragma once

#include "CoreMinimal.h"
#include "RLGameplayAbility.h"
#include "RLMeleeAttackAbility.generated.h"

/**
 * Sweeps a sphere in front of the avatar and damages everything hostile it
 * touches. Blueprint subclasses add montage, VFX, and camera shake; the
 * gameplay result is identical with or without them so the kit is playable
 * from day one.
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

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/** Hook for BP subclasses: fired per victim hit. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Melee")
	void OnMeleeHit(AActor* Victim);
};
