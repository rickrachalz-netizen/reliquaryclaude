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
class URLRunManagerSubsystem;
enum class ERLEquipSlot : uint8;

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
 * One row in the gear list: an item name plus an Equip (stash gear) or Unequip
 * (worn gear) button. Carries the item id / slot so its button knows what to
 * act on. Built natively, mirroring URLEssencePanelEntry.
 */
UCLASS()
class RELIQUARY_API URLGearPanelEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Build the row; bEquipped chooses the Unequip vs Equip action. */
	void Setup(URLCharacterPanelWidget* InOwner, FName InItemId, bool bInEquipped, ERLEquipSlot InSlot);

protected:
	UPROPERTY()
	TWeakObjectPtr<URLCharacterPanelWidget> Owner;

	FName ItemId = NAME_None;
	bool bEquipped = false;
	ERLEquipSlot EquipSlot{};

	UFUNCTION() void HandleAction();
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

	// --- Gear (called by the gear rows; also BP-callable) ---

	/** Equip a stash gear item, then rebuild the hero and refresh. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Panel")
	void EquipItemById(FName ItemId);

	/** Unequip a worn slot back to the stash, then rebuild and refresh. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Panel")
	void UnequipSlot(ERLEquipSlot InSlot);

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UAbilitySystemComponent* GetHeroASC() const;
	URLGameInstance* GetRLGameInstance() const;
	URLRunManagerSubsystem* GetRunManager() const;

	/** Re-applies the hero build after a loadout edit (safe mid-run). */
	void ApplyLoadoutChange();

	/** Refreshes the per-frame stat readouts from the ASC. */
	void RefreshStats();

	/** Rebuild the materials list (run inventory on a run, else the stash). */
	void RefreshMaterials();

	/** Rebuild the equipped-slots + equippable-stash gear list. */
	void RefreshGear();

	/** Bound to the run manager so the materials list tracks gathering. */
	UFUNCTION() void HandleRunInventoryChanged();

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

	/** Materials the hero currently holds (placeholder inventory readout). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Panel")
	TObjectPtr<UVerticalBox> MaterialList;

	/** Equipped slots + equippable stash gear. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Panel")
	TObjectPtr<UVerticalBox> GearList;

	/** Value cells, in the fixed order the labels were created (see cpp). */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> StatValueTexts;

	/** Entry class to spawn for each unlocked essence (defaults to native). */
	UPROPERTY(EditAnywhere, Category = "RELIQUARY|Panel")
	TSubclassOf<URLEssencePanelEntry> EntryWidgetClass;

	/** Entry class to spawn for each gear row (defaults to native). */
	UPROPERTY(EditAnywhere, Category = "RELIQUARY|Panel")
	TSubclassOf<URLGearPanelEntry> GearEntryWidgetClass;

	/** Guards against re-binding the run-inventory delegate. */
	bool bBoundRunInventory = false;
};
