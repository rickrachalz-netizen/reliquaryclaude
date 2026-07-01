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
		RunManager->SetRunState(ERLRunState::AtBaseCamp);
	}

	// Autosave whenever the player enters base camp.
	if (URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance()))
	{
		GI->SaveToDisk();
	}
}
