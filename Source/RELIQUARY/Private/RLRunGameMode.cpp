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

	// Risk of Rain 2 runs two combat directors: a fast one that drips small
	// waves and a slow one that saves up for big ones. Same here.
	if (!UGameplayStatics::GetActorOfClass(this, ARLEnemyDirector::StaticClass()))
	{
		UClass* SpawnClass = DirectorClass ? DirectorClass.Get() : ARLEnemyDirector::StaticClass();

		// Fast director: class defaults (7.5-10s waves).
		GetWorld()->SpawnActor<ARLEnemyDirector>(SpawnClass, FVector::ZeroVector, FRotator::ZeroRotator);

		// Slow director: long saving intervals, bigger and elite-happier waves.
		if (ARLEnemyDirector* SlowDirector = GetWorld()->SpawnActor<ARLEnemyDirector>(
			SpawnClass, FVector::ZeroVector, FRotator::ZeroRotator))
		{
			SlowDirector->MinWaveInterval = 22.5f;
			SlowDirector->MaxWaveInterval = 30.f;
			SlowDirector->MaxSpawnsPerWave = 10;
			SlowDirector->EliteWaveChance = 0.4f;
		}
	}
}
