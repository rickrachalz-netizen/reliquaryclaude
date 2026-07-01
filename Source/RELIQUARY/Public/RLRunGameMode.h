// RELIQUARY — game mode for zone maps on the realm's path.

#pragma once

#include "CoreMinimal.h"
#include "RELIQUARYGameMode.h"
#include "RLRunGameMode.generated.h"

class ARLEnemyDirector;

/**
 * Ensures a run is active (PIE straight into a run map embarks in place)
 * and spawns the enemy director. Zone content itself is scattered by the
 * ARLZoneScatterVolume placed in the map.
 */
UCLASS()
class RELIQUARY_API ARLRunGameMode : public ARELIQUARYGameMode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY")
	TSubclassOf<ARLEnemyDirector> DirectorClass;

protected:
	virtual void BeginPlay() override;
};
