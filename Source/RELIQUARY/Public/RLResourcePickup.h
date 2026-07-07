// RELIQUARY — a dropped material that homes to the hero and joins the run
// inventory on touch.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLResourcePickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class RELIQUARY_API ARLResourcePickup : public AActor
{
	GENERATED_BODY()

public:
	ARLResourcePickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Pickup")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Pickup")
	int32 Count = 1;

	/** Pickups drift toward heroes inside this radius (RoR2-style magnet). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Pickup")
	float MagnetRadius = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Pickup")
	float MagnetSpeed = 900.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** BP hook for collect VFX/SFX. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Pickup")
	void OnCollected(AActor* Collector);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * What collecting actually grants. Base: ItemId x Count into the run
	 * inventory. Subclasses override for other payloads (mana orbs, essence
	 * shards) and inherit the magnet + overlap plumbing for free.
	 */
	virtual void GrantTo(AActor* Collector);

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
