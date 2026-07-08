#include "RLCraftingStation.h"
#include "RLCraftingWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ARLCraftingStation::ARLCraftingStation()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Mesh);

	CraftingWidgetClass = URLCraftingWidget::StaticClass();
}

bool ARLCraftingStation::CanInteract_Implementation(AActor* Interactor) const
{
	return true;
}

FText ARLCraftingStation::GetInteractionPrompt_Implementation() const
{
	return NSLOCTEXT("RL", "CraftOpen", "Work the Forge");
}

void ARLCraftingStation::Interact_Implementation(AActor* Interactor)
{
	// Open the native forge panel for the interacting player.
	APawn* Pawn = Cast<APawn>(Interactor);
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (PC && CraftingWidgetClass)
	{
		if (URLCraftingWidget* Widget = CreateWidget<URLCraftingWidget>(PC, CraftingWidgetClass))
		{
			Widget->AddToViewport(80);
		}
	}

	// Fire the BP hook too, for optional bespoke forge presentation.
	OnCraftingOpened(Interactor);
}
