// RELIQUARY — classic-WoW floating damage numbers. Spawned per hit above the
// struck enemy; rises, fades, and removes itself. Crits pop larger and gold.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLDamageNumberWidget.generated.h"

class UTextBlock;

/**
 * A single floating damage number. Anchored to a world point at spawn; each
 * tick it projects that point to screen, drifts upward, and fades out over
 * Lifetime, then removes itself from the viewport.
 *
 * Works with zero content. To restyle in UMG, reparent a WBP to this class and
 * include a TextBlock named "DamageText" — the native tree then steps aside.
 */
UCLASS()
class RELIQUARY_API URLDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set the value/style and begin the float from WorldAnchor. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Combat")
	void InitDamageNumber(float Amount, bool bCritical, const FVector& WorldAnchor);

	/** Seconds the number lives before removing itself. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combat")
	float Lifetime = 1.0f;

	/** Upward drift in screen pixels per second. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combat")
	float RiseSpeed = 90.f;

	/** Horizontal spread so stacked hits don't perfectly overlap (± pixels). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combat")
	float HorizontalSpread = 35.f;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combat")
	float NormalFontSize = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combat")
	float CritFontSize = 32.f;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combat")
	FLinearColor NormalColor = FLinearColor::White;

	/** Classic WoW crit gold. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Combat")
	FLinearColor CritColor = FLinearColor(1.f, 0.84f, 0.f);

protected:
	virtual bool Initialize() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Bound from a WBP subclass, or built natively when absent. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Combat")
	TObjectPtr<UTextBlock> DamageText;

	FVector WorldAnchor = FVector::ZeroVector;
	float Elapsed = 0.f;
	float JitterX = 0.f;
	bool bActive = false;
};
