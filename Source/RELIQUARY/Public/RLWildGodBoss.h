// RELIQUARY — the game win condition. A single, extraordinarily punishing
// enemy bars the way out of the realm. It can be challenged at any time,
// but it is tuned around a well-geared, level-30 hero — and even then it is
// meant to be a serious test.

#pragma once

#include "CoreMinimal.h"
#include "RLEnemyBase.h"
#include "RLWildGodBoss.generated.h"

UCLASS()
class RELIQUARY_API ARLWildGodBoss : public ARLEnemyBase
{
	GENERATED_BODY()

public:
	ARLWildGodBoss();

	/** BP hook: play the ending — the way home is open. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|WildGod")
	void OnWildGodDefeated(AActor* Killer);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleWildGodKilled(ARLEnemyBase* Enemy, AActor* Killer);
};
