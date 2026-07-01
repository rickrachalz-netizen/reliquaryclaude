#include "RLBankingCrate.h"
#include "RLRunManagerSubsystem.h"
#include "Components/StaticMeshComponent.h"

ARLBankingCrate::ARLBankingCrate()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Mesh);
}

bool ARLBankingCrate::CanInteract_Implementation(AActor* Interactor) const
{
	if (bUsed)
	{
		return false;
	}
	const URLRunManagerSubsystem* RunManager =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
	return RunManager && RunManager->GetRunInventoryTotal() > 0;
}

FText ARLBankingCrate::GetInteractionPrompt_Implementation() const
{
	return bUsed
		? NSLOCTEXT("RL", "CrateGone", "Already sent home.")
		: NSLOCTEXT("RL", "CrateSend", "Send Resources to Base Camp");
}

void ARLBankingCrate::Interact_Implementation(AActor* Interactor)
{
	if (bUsed)
	{
		return;
	}

	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>())
	{
		const int32 Total = RunManager->GetRunInventoryTotal();
		if (Total > 0)
		{
			RunManager->BankRunInventory();
			bUsed = true;
			OnResourcesBanked(Total);
		}
	}
}
