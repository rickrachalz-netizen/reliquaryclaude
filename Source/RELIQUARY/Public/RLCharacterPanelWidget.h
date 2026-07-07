// RELIQUARY — the character sheet (toggled with C). Shows every live stat
// value and hosts the essence loadout: socket, unsocket, and upgrade the
// essences unlocked by killing enemy types.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLCharacterPanelWidget.generated.h"

class UVerticalBox;
class UScrollBox;
class UTextBlock;
class UButton;
class URLCharacterPanelWidget;
class UAbilitySystemComponent;
class URLGameInstance;

/**
 * One row in the unlocked-essence list. Carries the essence id so its buttons
 * know which essence to socket/upgrade (UButton::OnClicked has no payload).
 * Built natively; part of the panel's zero-content fallback tree.
 */
UCLASS()
class RELIQUARY_API URLEssencePanelEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Builds the row and wires its buttons back to the owning panel. */
	void Setup(URLCharacterPanelWidget* InOwner, FName InEssenceId);

protected:
	UPROPERTY()
	TWeakObjectPtr<URLCharacterPanelWidget> Owner;

	FName EssenceId = NAME_None;

	UFUNCTION() void HandleSocketMajor();
	UFUNCTION() void HandleSocketMinor();
	UFUNCTION() void HandleUpgrade();
};

/**
 * The character panel. Reads attributes through the owning pawn's ASC (same
 * pattern as URLRunHUDWidget) and refreshes them every tick; rebuilds the
 * essence section on open and after any loadout change. Socketing is allowed
 * anywhere — each change calls ARELIQUARYCharacter::RefreshHeroBuild.
 *
 * Works with zero content. To restyle, reparent a WBP to this class; the
 * native tree only builds when the WBP supplies none.
 */
UCLASS()
class RELIQUARY_API URLCharacterPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Rebuild the essence slots + unlocked list. Called on open and edits. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Panel")
	void RefreshEssences();

	// --- Loadout edits (called by the entry rows; also BP-callable) ---

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Panel")
	void SocketEssenceIntoSlot(FName EssenceId, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Panel")
	void SocketEssenceIntoFreeMinor(FName EssenceId);

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Panel")
	void UnsocketSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Panel")
	void UpgradeEssenceById(FName EssenceId);

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UAbilitySystemComponent* GetHeroASC() const;
	URLGameInstance* GetRLGameInstance() const;

	/** Re-applies the hero build after a loadout edit (safe mid-run). */
	void ApplyLoadoutChange();

	/** Refreshes the per-frame stat readouts from the ASC. */
	void RefreshStats();

	// One thin handler per fixed slot (UButton::OnClicked carries no payload).
	UFUNCTION() void HandleUnsocketMajor();
	UFUNCTION() void HandleUnsocketMinor1();
	UFUNCTION() void HandleUnsocketMinor2();
	UFUNCTION() void HandleUnsocketMinor3();

	/** Bound from a WBP subclass, or built natively when absent. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Panel")
	TObjectPtr<UTextBlock> HeroTitleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Panel")
	TObjectPtr<UVerticalBox> StatList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Panel")
	TObjectPtr<UVerticalBox> EssenceSlotList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Panel")
	TObjectPtr<UScrollBox> UnlockedEssenceList;

	/** Value cells, in the fixed order the labels were created (see cpp). */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> StatValueTexts;

	/** Entry class to spawn for each unlocked essence (defaults to native). */
	UPROPERTY(EditAnywhere, Category = "RELIQUARY|Panel")
	TSubclassOf<URLEssencePanelEntry> EntryWidgetClass;
};
