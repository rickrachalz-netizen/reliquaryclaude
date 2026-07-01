// RELIQUARY — fires an ARLProjectile carrying the standard damage spec.

#pragma once

#include "CoreMinimal.h"
#include "RLGameplayAbility.h"
#include "RLProjectileAbility.generated.h"

class ARLProjectile;

/** The Mage's default bolt; any ranged kit can subclass or retag it. */
UCLASS()
class RELIQUARY_API URLProjectileAbility : public URLGameplayAbility
{
	GENERATED_BODY()

public:
	URLProjectileAbility();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RELIQUARY|Projectile")
	TSubclassOf<ARLProjectile> ProjectileClass;

	/** Spawn offset forward from the avatar, in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Projectile")
	float MuzzleDistance = 80.f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
