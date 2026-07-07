// RELIQUARY — the RoR2-style ability bar: five icon slots with radial
// cooldown dials. Drop one into WBP_RunHUD; it builds its slots itself.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "RLAbilityBarWidget.generated.h"

class UHorizontalBox;
class URLAbilitySlotWidget;

/**
 * Primary / Secondary / Utility / Special / Essence icons in a row, each an
 * URLAbilitySlotWidget. Needs zero wiring: place it in a HUD widget (or add
 * it to the viewport) and it populates from the owning pawn's ability system.
 * The essence slot stays collapsed until a major essence grants an active.
 *
 * Set SlotWidgetClass to a WBP reparented to URLAbilitySlotWidget to reskin
 * every icon at once.
 */
UCLASS()
class RELIQUARY_API URLAbilityBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Slot widget to spawn; defaults to the native tile when unset. */
	UPROPERTY(EditAnywhere, Category = "RELIQUARY|Bar")
	TSubclassOf<URLAbilitySlotWidget> SlotWidgetClass;

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;

	/** Bound from a WBP subclass, or built natively when absent. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Bar")
	TObjectPtr<UHorizontalBox> SlotRow;

	void AddSlotWidget(const FGameplayTag& ActionTag, const FText& KeybindLabel, bool bCollapseWhenEmpty);
};
