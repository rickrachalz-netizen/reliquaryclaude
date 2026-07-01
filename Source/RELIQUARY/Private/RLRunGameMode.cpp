#include "RLRunGameMode.h"
#include "RLEnemyDirector.h"
#include "RLRunManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"

void ARLRunGameMode::BeginPlay()
{
	// Establish run state BEFORE Super so scatter volumes and directors
	// (whose BeginPlay runs on world begin) see an active, seeded run.
	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>())
	{
		if (!RunManager->IsOnRun())
		{
			RunManager->EmbarkInPlace();
		}
	}

	Super::BeginPlay();

	if (!UGameplayStatics::GetActorOfClass(this, ARLEnemyDirector::StaticClass()))
	{
		UClass* SpawnClass = DirectorClass ? DirectorClass.Get() : ARLEnemyDirector::StaticClass();
		GetWorld()->SpawnActor<ARLEnemyDirector>(SpawnClass, FVector::ZeroVector, FRotator::ZeroRotator);
	}
}
