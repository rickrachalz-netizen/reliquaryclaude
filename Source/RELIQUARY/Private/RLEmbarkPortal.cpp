#include "RLEmbarkPortal.h"
#include "RLRunManagerSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ARLEmbarkPortal::ARLEmbarkPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Mesh);
}

bool ARLEmbarkPortal::CanInteract_Implementation(AActor* Interactor) const
{
	const URLRunManagerSubsystem* RunManager =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
	return RunManager && !RunManager->IsOnRun();
}

FText ARLEmbarkPortal::GetInteractionPrompt_Implementation() const
{
	return Destination == ERLPortalDestination::RealmPath
		? NSLOCTEXT("RL", "PortalEmbark", "Embark on the Realm's Path")
		: NSLOCTEXT("RL", "PortalWildGod", "Challenge the Wild God");
}

void ARLEmbarkPortal::Interact_Implementation(AActor* Interactor)
{
	URLRunManagerSubsystem* RunManager =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
	if (!RunManager || RunManager->IsOnRun())
	{
		return;
	}

	if (Destination == ERLPortalDestination::RealmPath)
	{
		RunManager->EmbarkOnRun();
	}
	else
	{
		UGameplayStatics::OpenLevel(this, WildGodArenaMap);
	}
}
