// RELIQUARY — a destructible piece of the world: a tree, a boulder, a vein.
// Attacks shatter it into distinct materials and a spray of excess mana.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLResourceNode.generated.h"

class UStaticMeshComponent;
class ARLResourcePickup;

UCLASS()
class RELIQUARY_API ARLResourceNode : public AActor
{
	GENERATED_BODY()

public:
	ARLResourceNode();

	/** DT_Items row this node yields (Oakheart Timber, Duskiron Ore, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Resource")
	FName MaterialItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Resource")
	int32 MinYield = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Resource")
	int32 MaxYield = 4;

	/** Excess mana showered on the breaker when the node shatters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Resource")
	int32 ExcessManaYield = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Resource")
	float MaxNodeHealth = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Resource")
	TSubclassOf<ARLResourcePickup> PickupClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	/** BP hook: shatter VFX/SFX, foliage fall, camera shake. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Resource")
	void OnShattered(AActor* Breaker);

	/** BP hook: chip/wobble feedback per hit. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Resource")
	void OnChipped(float RemainingFraction);

protected:
	virtual void BeginPlay() override;

	float NodeHealth = 0.f;
	bool bShattered = false;

	void Shatter(AActor* Breaker);
};
