#include "RLBaseCampGameMode.h"
#include "RLGameInstance.h"
#include "RLRunManagerSubsystem.h"
#include "RLTypes.h"

void ARLBaseCampGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>())
	{
		// Banks any still-live haul (direct-travel shortcut) before the save
		// below, so nothing gathered is lost on the way home.
		RunManager->NotifyArrivedAtBaseCamp();
	}

	// Autosave whenever the player enters base camp.
	if (URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance()))
	{
		GI->SaveToDisk();
	}
}
