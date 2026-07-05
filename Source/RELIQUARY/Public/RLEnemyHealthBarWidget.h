// RELIQUARY — floating enemy health bar, shown only while the enemy is hurt.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLEnemyHealthBarWidget.generated.h"

class UProgressBar;

/**
 * RoR2-style combat health bar. ARLEnemyBase pops it above an enemy when it
 * takes damage and hides it again after a few untouched seconds.
 *
 * Builds its own widget tree (one styled progress bar) so it works with zero
 * content. To restyle in UMG, reparent a WBP to this class and include a
 * ProgressBar named "Bar" — the native tree then steps aside.
 */
UCLASS()
class RELIQUARY_API URLEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|HealthBar")
	float BarWidth = 110.f;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|HealthBar")
	float BarHeight = 10.f;

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|HealthBar")
	void SetHealthFraction(float Fraction);

	/** Elites and bosses tint their bars. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|HealthBar")
	void SetBarColor(FLinearColor Color);

protected:
	virtual bool Initialize() override;

	/** Bound from a WBP subclass, or built natively when absent. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|HealthBar")
	TObjectPtr<UProgressBar> Bar;
};
