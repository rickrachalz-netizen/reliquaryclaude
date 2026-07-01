// RELIQUARY — interface for anything the hero can use: altars, crates,
// crafting stations, pickups.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RLInteractable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class URLInteractable : public UInterface
{
	GENERATED_BODY()
};

class RELIQUARY_API IRLInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RELIQUARY|Interaction")
	void Interact(AActor* Interactor);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RELIQUARY|Interaction")
	FText GetInteractionPrompt() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RELIQUARY|Interaction")
	bool CanInteract(AActor* Interactor) const;
};
