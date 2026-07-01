#include "RLCraftingStation.h"
#include "Components/StaticMeshComponent.h"

ARLCraftingStation::ARLCraftingStation()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Mesh);
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
	OnCraftingOpened(Interactor);
}
