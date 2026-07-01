// RELIQUARY — the forge at base camp. Interacting opens the crafting UI;
// URLCraftingLibrary does the actual work against the stash.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLInteractable.h"
#include "RLCraftingStation.generated.h"

class UStaticMeshComponent;

UCLASS()
class RELIQUARY_API ARLCraftingStation : public AActor, public IRLInteractable
{
	GENERATED_BODY()

public:
	ARLCraftingStation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// --- IRLInteractable ---
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** BP hook: open the crafting widget. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Crafting")
	void OnCraftingOpened(AActor* Interactor);
};
