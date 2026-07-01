// RELIQUARY — game mode for base camp, the corpse of the fallen Wild God.

#pragma once

#include "CoreMinimal.h"
#include "RELIQUARYGameMode.h"
#include "RLBaseCampGameMode.generated.h"

/**
 * Safe ground. Arriving here ends any run bookkeeping and autosaves —
 * the only place the game ever saves, by design.
 */
UCLASS()
class RELIQUARY_API ARLBaseCampGameMode : public ARELIQUARYGameMode
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
