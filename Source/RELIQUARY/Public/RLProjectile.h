// RELIQUARY — simple homing-free projectile fired by abilities.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "RLProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

/**
 * Carries a prepared damage spec from the firing ability and applies it to
 * the first hostile GAS actor it touches. Non-GAS blockers (trees, rocks)
 * take plain point damage so projectiles also shatter the world.
 */
UCLASS()
class RELIQUARY_API ARLProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARLProjectile();

	/** Damage spec prepared by the ability that fired us. */
	FGameplayEffectSpecHandle DamageSpec;

	/** Fallback damage for non-GAS destructibles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	float WorldDamage = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float LifeSeconds = 5.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> Movement;

	/** BP hook for impact VFX/SFX. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile")
	void OnImpact(AActor* HitActor, const FVector& HitLocation);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
