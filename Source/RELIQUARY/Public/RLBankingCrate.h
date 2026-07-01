// RELIQUARY — a banking crate. Every three cleared zones, one appears near
// the challenge altar: ship everything gathered so far back to base camp and
// keep pushing, Helldivers-style insurance against a greedy death.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLInteractable.h"
#include "RLBankingCrate.generated.h"

class UStaticMeshComponent;

UCLASS()
class RELIQUARY_API ARLBankingCrate : public AActor, public IRLInteractable
{
	GENERATED_BODY()

public:
	ARLBankingCrate();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// --- IRLInteractable ---
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** BP hook: launch VFX as the crate rockets home. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Crate")
	void OnResourcesBanked(int32 TotalBanked);

protected:
	bool bUsed = false;
};
