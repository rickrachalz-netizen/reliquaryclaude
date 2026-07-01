// RELIQUARY — save file. Only persistent hero data is ever written; active
// runs cannot be saved or reloaded by design.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RLTypes.h"
#include "RLSaveGame.generated.h"

UCLASS()
class RELIQUARY_API URLSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static const FString SlotName;
	static constexpr int32 UserIndex = 0;

	UPROPERTY(SaveGame)
	TArray<FRLHeroData> Heroes;

	UPROPERTY(SaveGame)
	int32 ActiveHeroIndex = 0;
};
