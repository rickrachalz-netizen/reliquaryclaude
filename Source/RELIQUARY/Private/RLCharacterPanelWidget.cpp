#include "RLCharacterPanelWidget.h"
#include "RELIQUARYCharacter.h"
#include "RLAbilitySystemComponent.h"
#include "RLAttributeSet.h"
#include "RLGameInstance.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "RLTypes.h"
#include "RLRunManagerSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace
{
	// Labels for the stat readout, in the exact order RefreshStats fills them.
	const TCHAR* GStatLabels[] = {
		TEXT("Health"), TEXT("Health Regen"), TEXT("Mana"), TEXT("Mana Regen"),
		TEXT("Strength"), TEXT("Agility"), TEXT("Intellect"),
		TEXT("Crit Chance"), TEXT("Crit Damage"), TEXT("Haste"),
		TEXT("Armor"), TEXT("Move Speed"), TEXT("Adaptability"), TEXT("Battle Trance")
	};
	constexpr int32 GStatCount = UE_ARRAY_COUNT(GStatLabels);

	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, int32 FontSize = 12, bool bBold = false)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(FText::FromString(Text));
		Block->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FontSize));
		return Block;
	}

	FText ItemDisplayName(const URLDataSubsystem* Data, FName ItemId)
	{
		if (Data)
		{
			if (const FRLItemRow* Row = Data->FindItem(ItemId))
			{
				if (!Row->DisplayName.IsEmpty())
				{
					return Row->DisplayName;
				}
			}
		}
		return FText::FromName(ItemId);
	}
}

// ---------------------------------------------------------------------------
// URLEssencePanelEntry
// ---------------------------------------------------------------------------

void URLEssencePanelEntry::Setup(URLCharacterPanelWidget* InOwner, FName InEssenceId)
{
	Owner = InOwner;
	EssenceId = InEssenceId;

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	FText DisplayName = FText::FromName(EssenceId);
	FText UpgradeLabel = NSLOCTEXT("RL", "EssenceUpgrade", "Upgrade");
	if (const URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance()))
	{
		if (const URLDataSubsystem* Data = GI->GetSubsystem<URLDataSubsystem>())
		{
			if (const FRLEssenceRow* Row = Data->FindEssence(EssenceId))
			{
				DisplayName = Row->DisplayName.IsEmpty() ? FText::FromName(EssenceId) : Row->DisplayName;
			}
		}
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Name->SetText(DisplayName);
	Name->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
	if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Name))
	{
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetPadding(FMargin(2.f, 2.f));
	}

	auto AddButton = [&](const FText& Label, FName FuncName)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(Label);
		Text->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
		Button->AddChild(Text);

		FScriptDelegate Del;
		Del.BindUFunction(this, FuncName);
		Button->OnClicked.Add(Del);

		if (UHorizontalBoxSlot* BtnSlot = Row->AddChildToHorizontalBox(Button))
		{
			BtnSlot->SetPadding(FMargin(2.f, 2.f));
		}
	};

	AddButton(NSLOCTEXT("RL", "SocketMajor", "Major"), GET_FUNCTION_NAME_CHECKED(URLEssencePanelEntry, HandleSocketMajor));
	AddButton(NSLOCTEXT("RL", "SocketMinor", "Minor"), GET_FUNCTION_NAME_CHECKED(URLEssencePanelEntry, HandleSocketMinor));
	AddButton(UpgradeLabel, GET_FUNCTION_NAME_CHECKED(URLEssencePanelEntry, HandleUpgrade));

	WidgetTree->RootWidget = Row;
}

void URLEssencePanelEntry::HandleSocketMajor()
{
	if (Owner.IsValid())
	{
		Owner->SocketEssenceIntoSlot(EssenceId, 0);
	}
}

void URLEssencePanelEntry::HandleSocketMinor()
{
	if (Owner.IsValid())
	{
		Owner->SocketEssenceIntoFreeMinor(EssenceId);
	}
}

void URLEssencePanelEntry::HandleUpgrade()
{
	if (Owner.IsValid())
	{
		Owner->UpgradeEssenceById(EssenceId);
	}
}

// ---------------------------------------------------------------------------
// URLGearPanelEntry
// ---------------------------------------------------------------------------

void URLGearPanelEntry::Setup(URLCharacterPanelWidget* InOwner, FName InItemId, bool bInEquipped, ERLEquipSlot InSlot)
{
	Owner = InOwner;
	ItemId = InItemId;
	bEquipped = bInEquipped;
	EquipSlot = InSlot;

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance());
	const URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	WidgetTree->RootWidget = Row;

	UTextBlock* Label = MakeText(WidgetTree, ItemDisplayName(Data, ItemId).ToString(), 12, false);
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label))
	{
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->AddChild(MakeText(WidgetTree, bEquipped ? TEXT("Unequip") : TEXT("Equip"), 10, false));
	FScriptDelegate Del;
	Del.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(URLGearPanelEntry, HandleAction));
	Button->OnClicked.Add(Del);
	if (UHorizontalBoxSlot* BtnSlot = Row->AddChildToHorizontalBox(Button))
	{
		BtnSlot->SetPadding(FMargin(2.f, 2.f));
	}
}

void URLGearPanelEntry::HandleAction()
{
	if (!Owner.IsValid())
	{
		return;
	}
	if (bEquipped)
	{
		Owner->UnequipSlot(EquipSlot);
	}
	else
	{
		Owner->EquipItemById(ItemId);
	}
}

// ---------------------------------------------------------------------------
// URLCharacterPanelWidget
// ---------------------------------------------------------------------------

bool URLCharacterPanelWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	// Pure native use (no WBP tree): build the two-column sheet.
	if (!StatList && WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Backing = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backing"));
		Backing->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
		Backing->SetPadding(FMargin(16.f));
		Backing->SetHorizontalAlignment(HAlign_Center);
		Backing->SetVerticalAlignment(VAlign_Center);

		UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Backing->AddChild(Outer);

		HeroTitleText = MakeText(WidgetTree, TEXT("Character"), 18, true);
		Outer->AddChildToVerticalBox(HeroTitleText);

		UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Outer->AddChildToVerticalBox(Columns);

		// Left column: stats.
		{
			USizeBox* LeftBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			LeftBox->SetWidthOverride(280.f);
			StatList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			LeftBox->AddChild(StatList);
			if (UHorizontalBoxSlot* ColSlot = Columns->AddChildToHorizontalBox(LeftBox))
			{
				ColSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
			}

			StatList->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Stats"), 14, true));
			StatValueTexts.Reset();
			for (int32 i = 0; i < GStatCount; ++i)
			{
				UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

				UTextBlock* Label = MakeText(WidgetTree, GStatLabels[i], 12, false);
				if (UHorizontalBoxSlot* LabelSlot = RowBox->AddChildToHorizontalBox(Label))
				{
					LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				}

				UTextBlock* Value = MakeText(WidgetTree, TEXT("-"), 12, true);
				RowBox->AddChildToHorizontalBox(Value);
				StatValueTexts.Add(Value);

				if (UVerticalBoxSlot* RowSlot = StatList->AddChildToVerticalBox(RowBox))
				{
					RowSlot->SetPadding(FMargin(0.f, 1.f));
				}
			}
		}

		// Right column: essences.
		{
			USizeBox* RightBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			RightBox->SetWidthOverride(320.f);
			UVerticalBox* RightCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			RightBox->AddChild(RightCol);
			Columns->AddChildToHorizontalBox(RightBox);

			RightCol->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Essences"), 14, true));

			EssenceSlotList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			RightCol->AddChildToVerticalBox(EssenceSlotList);

			RightCol->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Unlocked"), 13, true));

			UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
			UnlockedEssenceList = Scroll;
			USizeBox* ScrollBounds = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			ScrollBounds->SetHeightOverride(220.f);
			ScrollBounds->AddChild(Scroll);
			RightCol->AddChildToVerticalBox(ScrollBounds);
		}

		// Third column: materials (placeholder inventory) + gear.
		{
			USizeBox* InvBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			InvBox->SetWidthOverride(300.f);
			UVerticalBox* InvCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			InvBox->AddChild(InvCol);
			if (UHorizontalBoxSlot* ColSlot = Columns->AddChildToHorizontalBox(InvBox))
			{
				ColSlot->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));
			}

			InvCol->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Materials"), 14, true));
			MaterialList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			InvCol->AddChildToVerticalBox(MaterialList);

			if (UVerticalBoxSlot* GearHeaderSlot =
				InvCol->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Gear"), 14, true)))
			{
				GearHeaderSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
			}
			GearList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			InvCol->AddChildToVerticalBox(GearList);
		}

		WidgetTree->RootWidget = Backing;
	}

	return true;
}

void URLCharacterPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Track gathering so the materials list updates as nodes are harvested.
	if (!bBoundRunInventory)
	{
		if (URLRunManagerSubsystem* Run = GetRunManager())
		{
			Run->OnRunInventoryChanged.AddDynamic(this, &URLCharacterPanelWidget::HandleRunInventoryChanged);
			bBoundRunInventory = true;
		}
	}

	RefreshEssences();
}

URLRunManagerSubsystem* URLCharacterPanelWidget::GetRunManager() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
}

void URLCharacterPanelWidget::HandleRunInventoryChanged()
{
	RefreshMaterials();
}

void URLCharacterPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The panel keeps ticking while hidden; skip the work until it's shown.
	if (GetVisibility() == ESlateVisibility::Collapsed || GetVisibility() == ESlateVisibility::Hidden)
	{
		return;
	}
	RefreshStats();
}

UAbilitySystemComponent* URLCharacterPanelWidget::GetHeroASC() const
{
	APawn* Pawn = GetOwningPlayerPawn();
	return Pawn ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn) : nullptr;
}

URLGameInstance* URLCharacterPanelWidget::GetRLGameInstance() const
{
	return Cast<URLGameInstance>(GetGameInstance());
}

void URLCharacterPanelWidget::RefreshStats()
{
	if (HeroTitleText)
	{
		if (URLGameInstance* GI = GetRLGameInstance())
		{
			const FRLHeroData Hero = GI->GetActiveHeroData();
			HeroTitleText->SetText(FText::Format(
				NSLOCTEXT("RL", "PanelTitle", "{0} — Level {1} {2}"),
				FText::FromString(Hero.HeroName), FText::AsNumber(Hero.Level), GI->GetDisplayedClassName()));
		}
	}

	UAbilitySystemComponent* ASC = GetHeroASC();
	if (!ASC || StatValueTexts.Num() != GStatCount)
	{
		return;
	}

	auto Attr = [ASC](const FGameplayAttribute& A) { return ASC->GetNumericAttribute(A); };

	int32 AdaptStacks = 0;
	if (const URLAbilitySystemComponent* RLASC = Cast<URLAbilitySystemComponent>(ASC))
	{
		AdaptStacks = RLASC->GetAdaptabilityStacks();
	}
	// Battle Trance's haste/heal/speed bonuses live outside the AttributeSet and
	// are folded in at each point of consumption, so the raw attribute values
	// never reflect them. Read the live bonuses off the hero here so Haste,
	// Health Regen, and Move Speed show what the player actually has right now.
	const ARELIQUARYCharacter* Hero = Cast<ARELIQUARYCharacter>(GetOwningPlayerPawn());
	const float TrancePct = Hero ? Hero->GetBattleTranceIntensity() : 0.f;
	const float TranceHaste = Hero ? Hero->GetTranceHasteBonus() : 0.f;
	const float TranceHealMul = Hero ? (1.f + Hero->GetTranceHealingBonus()) : 1.f;
	const float TranceSpeedMul = Hero ? (1.f + Hero->GetTranceSpeedBonus()) : 1.f;

	// Order must match GStatLabels above.
	TArray<FString> Values;
	Values.Add(FString::Printf(TEXT("%.0f / %.0f"), Attr(URLAttributeSet::GetHealthAttribute()), Attr(URLAttributeSet::GetMaxHealthAttribute())));
	Values.Add(FString::Printf(TEXT("%.1f/s"), Attr(URLAttributeSet::GetHealthRegenAttribute()) * TranceHealMul));
	Values.Add(FString::Printf(TEXT("%.0f / %.0f"), Attr(URLAttributeSet::GetManaAttribute()), Attr(URLAttributeSet::GetMaxManaAttribute())));
	Values.Add(FString::Printf(TEXT("%.1f/s"), Attr(URLAttributeSet::GetManaRegenAttribute())));
	Values.Add(FString::Printf(TEXT("%.0f"), Attr(URLAttributeSet::GetStrengthAttribute())));
	Values.Add(FString::Printf(TEXT("%.0f"), Attr(URLAttributeSet::GetAgilityAttribute())));
	Values.Add(FString::Printf(TEXT("%.0f"), Attr(URLAttributeSet::GetIntellectAttribute())));
	Values.Add(FString::Printf(TEXT("%.1f%%"), Attr(URLAttributeSet::GetCritChanceAttribute()) * 100.f));
	Values.Add(FString::Printf(TEXT("x%.2f"), Attr(URLAttributeSet::GetCritDamageAttribute())));
	Values.Add(FString::Printf(TEXT("%.1f%%"), (Attr(URLAttributeSet::GetHasteAttribute()) + TranceHaste) * 100.f));
	Values.Add(FString::Printf(TEXT("%.0f"), Attr(URLAttributeSet::GetArmorAttribute())));
	Values.Add(FString::Printf(TEXT("%.0f"), Attr(URLAttributeSet::GetMoveSpeedAttribute()) * TranceSpeedMul));
	Values.Add(FString::Printf(TEXT("%.1f%% (x%d)"), Attr(URLAttributeSet::GetAdaptabilityAttribute()) * 100.f, AdaptStacks));
	Values.Add(FString::Printf(TEXT("%.0f%%"), TrancePct * 100.f));

	for (int32 i = 0; i < GStatCount && i < Values.Num(); ++i)
	{
		if (StatValueTexts[i])
		{
			StatValueTexts[i]->SetText(FText::FromString(Values[i]));
		}
	}
}

void URLCharacterPanelWidget::RefreshEssences()
{
	URLGameInstance* GI = GetRLGameInstance();
	if (!GI)
	{
		return;
	}
	const URLDataSubsystem* Data = GI->GetSubsystem<URLDataSubsystem>();
	const FRLHeroData Hero = GI->GetActiveHeroData();
	const int32 UnlockedSlots = GI->GetUnlockedEssenceSlots();

	auto DisplayNameOf = [&](FName EssenceId) -> FText
	{
		if (Data)
		{
			if (const FRLEssenceRow* Row = Data->FindEssence(EssenceId))
			{
				if (!Row->DisplayName.IsEmpty())
				{
					return Row->DisplayName;
				}
			}
		}
		return FText::FromName(EssenceId);
	};

	// --- Slots (Major, Minor 1..3) ---
	if (EssenceSlotList)
	{
		EssenceSlotList->ClearChildren();

		const FName UnsocketHandlers[4] = {
			GET_FUNCTION_NAME_CHECKED(URLCharacterPanelWidget, HandleUnsocketMajor),
			GET_FUNCTION_NAME_CHECKED(URLCharacterPanelWidget, HandleUnsocketMinor1),
			GET_FUNCTION_NAME_CHECKED(URLCharacterPanelWidget, HandleUnsocketMinor2),
			GET_FUNCTION_NAME_CHECKED(URLCharacterPanelWidget, HandleUnsocketMinor3),
		};

		for (int32 SlotIdx = 0; SlotIdx < 4; ++SlotIdx)
		{
			const FString SlotName = (SlotIdx == 0) ? TEXT("Major") : FString::Printf(TEXT("Minor %d"), SlotIdx);

			// Level at which this slot unlocks (for the "Locked" hint).
			const int32 RequiredLevel = (SlotIdx == 0)
				? RLBalance::MajorEssenceUnlockLevel
				: RLBalance::MinorEssenceUnlockLevels[SlotIdx - 1];

			const FRLSocketedEssence* Socketed = Hero.Essences.FindByPredicate(
				[SlotIdx](const FRLSocketedEssence& E) { return E.SlotIndex == SlotIdx; });

			FString Content;
			if (SlotIdx >= UnlockedSlots)
			{
				Content = FString::Printf(TEXT("%s: Locked (Lv %d)"), *SlotName, RequiredLevel);
			}
			else if (Socketed)
			{
				Content = FString::Printf(TEXT("%s: %s  (R%d)"), *SlotName,
					*DisplayNameOf(Socketed->EssenceId).ToString(), Socketed->Rank);
			}
			else
			{
				Content = FString::Printf(TEXT("%s: empty"), *SlotName);
			}

			UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

			UTextBlock* Label = MakeText(WidgetTree, Content, 12, false);
			if (UHorizontalBoxSlot* LabelSlot = RowBox->AddChildToHorizontalBox(Label))
			{
				LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				LabelSlot->SetVerticalAlignment(VAlign_Center);
			}

			// Only offer Unsocket on a filled, unlocked slot.
			if (Socketed && SlotIdx < UnlockedSlots)
			{
				UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
				UTextBlock* Text = MakeText(WidgetTree, TEXT("Unsocket"), 10, false);
				Button->AddChild(Text);
				FScriptDelegate Del;
				Del.BindUFunction(this, UnsocketHandlers[SlotIdx]);
				Button->OnClicked.Add(Del);
				RowBox->AddChildToHorizontalBox(Button);
			}

			if (UVerticalBoxSlot* RowSlot = EssenceSlotList->AddChildToVerticalBox(RowBox))
			{
				RowSlot->SetPadding(FMargin(0.f, 2.f));
			}
		}
	}

	// --- Unlocked essence list ---
	if (UnlockedEssenceList)
	{
		UnlockedEssenceList->ClearChildren();
		const TSubclassOf<URLEssencePanelEntry> EntryClass =
			EntryWidgetClass ? *EntryWidgetClass : URLEssencePanelEntry::StaticClass();

		for (const FName& EssenceId : GI->GetUnlockedEssenceIds())
		{
			URLEssencePanelEntry* Entry = CreateWidget<URLEssencePanelEntry>(this, EntryClass);
			if (Entry)
			{
				Entry->Setup(this, EssenceId);
				UnlockedEssenceList->AddChild(Entry);
			}
		}
	}

	// The inventory column travels with the rest of the loadout refresh.
	RefreshMaterials();
	RefreshGear();
}

void URLCharacterPanelWidget::RefreshMaterials()
{
	if (!MaterialList)
	{
		return;
	}
	MaterialList->ClearChildren();

	URLGameInstance* GI = GetRLGameInstance();
	const URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;

	// Source: what you've gathered this run, or the stash back at base camp.
	TArray<FRLItemStack> Items;
	if (URLRunManagerSubsystem* Run = GetRunManager(); Run && Run->IsOnRun())
	{
		Items = Run->GetRunInventory();
	}
	else if (GI)
	{
		Items = GI->GetActiveHeroData().Stash;
	}

	int32 Shown = 0;
	for (const FRLItemStack& Stack : Items)
	{
		// Only materials belong in this readout; gear shows in the gear list.
		if (Data)
		{
			const FRLItemRow* Row = Data->FindItem(Stack.ItemId);
			if (Row && Row->ItemType != ERLItemType::Material)
			{
				continue;
			}
		}
		const FString Line = FString::Printf(TEXT("%s  x%d"),
			*ItemDisplayName(Data, Stack.ItemId).ToString(), Stack.Count);
		MaterialList->AddChildToVerticalBox(MakeText(WidgetTree, Line, 11, false));
		++Shown;
	}

	if (Shown == 0)
	{
		MaterialList->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("(none)"), 11, false));
	}
}

void URLCharacterPanelWidget::RefreshGear()
{
	if (!GearList)
	{
		return;
	}
	GearList->ClearChildren();

	URLGameInstance* GI = GetRLGameInstance();
	if (!GI)
	{
		return;
	}
	const URLDataSubsystem* Data = GI->GetSubsystem<URLDataSubsystem>();
	const FRLHeroData Hero = GI->GetActiveHeroData();

	const TSubclassOf<URLGearPanelEntry> EntryClass =
		GearEntryWidgetClass ? *GearEntryWidgetClass : URLGearPanelEntry::StaticClass();

	// Worn gear first (each with an Unequip action).
	for (const TPair<ERLEquipSlot, FName>& Pair : Hero.Equipped)
	{
		if (Pair.Value == NAME_None)
		{
			continue;
		}
		if (URLGearPanelEntry* Entry = CreateWidget<URLGearPanelEntry>(this, EntryClass))
		{
			Entry->Setup(this, Pair.Value, /*bEquipped=*/true, Pair.Key);
			GearList->AddChild(Entry);
		}
	}

	// Then equippable gear sitting in the stash (each with an Equip action).
	for (const FRLItemStack& Stack : Hero.Stash)
	{
		const FRLItemRow* Item = Data ? Data->FindItem(Stack.ItemId) : nullptr;
		if (!Item || Item->EquipSlot == ERLEquipSlot::None)
		{
			continue;
		}
		if (URLGearPanelEntry* Entry = CreateWidget<URLGearPanelEntry>(this, EntryClass))
		{
			Entry->Setup(this, Stack.ItemId, /*bEquipped=*/false, Item->EquipSlot);
			GearList->AddChild(Entry);
		}
	}

	if (GearList->GetChildrenCount() == 0)
	{
		GearList->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("(none)"), 11, false));
	}
}

void URLCharacterPanelWidget::EquipItemById(FName ItemId)
{
	if (URLGameInstance* GI = GetRLGameInstance())
	{
		if (GI->EquipFromStash(ItemId))
		{
			GI->SaveToDisk();
		}
	}
	ApplyLoadoutChange();
}

void URLCharacterPanelWidget::UnequipSlot(ERLEquipSlot InSlot)
{
	if (URLGameInstance* GI = GetRLGameInstance())
	{
		if (GI->Unequip(InSlot))
		{
			GI->SaveToDisk();
		}
	}
	ApplyLoadoutChange();
}

void URLCharacterPanelWidget::ApplyLoadoutChange()
{
	if (ARELIQUARYCharacter* Hero = Cast<ARELIQUARYCharacter>(GetOwningPlayerPawn()))
	{
		Hero->RefreshHeroBuild();
	}
	RefreshEssences();
}

void URLCharacterPanelWidget::SocketEssenceIntoSlot(FName EssenceId, int32 SlotIndex)
{
	if (URLGameInstance* GI = GetRLGameInstance())
	{
		if (GI->SocketEssence(EssenceId, SlotIndex))
		{
			ApplyLoadoutChange();
		}
	}
}

void URLCharacterPanelWidget::SocketEssenceIntoFreeMinor(FName EssenceId)
{
	URLGameInstance* GI = GetRLGameInstance();
	if (!GI)
	{
		return;
	}

	const FRLHeroData Hero = GI->GetActiveHeroData();
	const int32 UnlockedSlots = GI->GetUnlockedEssenceSlots();

	// First unlocked minor slot (1..3) not already occupied.
	for (int32 SlotIdx = 1; SlotIdx < UnlockedSlots; ++SlotIdx)
	{
		const bool bTaken = Hero.Essences.ContainsByPredicate(
			[SlotIdx](const FRLSocketedEssence& E) { return E.SlotIndex == SlotIdx; });
		if (!bTaken)
		{
			SocketEssenceIntoSlot(EssenceId, SlotIdx);
			return;
		}
	}
}

void URLCharacterPanelWidget::UnsocketSlot(int32 SlotIndex)
{
	if (URLGameInstance* GI = GetRLGameInstance())
	{
		if (GI->UnsocketEssence(SlotIndex))
		{
			ApplyLoadoutChange();
		}
	}
}

void URLCharacterPanelWidget::UpgradeEssenceById(FName EssenceId)
{
	if (URLGameInstance* GI = GetRLGameInstance())
	{
		if (GI->UpgradeEssence(EssenceId))
		{
			ApplyLoadoutChange();
		}
	}
}

void URLCharacterPanelWidget::HandleUnsocketMajor()  { UnsocketSlot(0); }
void URLCharacterPanelWidget::HandleUnsocketMinor1() { UnsocketSlot(1); }
void URLCharacterPanelWidget::HandleUnsocketMinor2() { UnsocketSlot(2); }
void URLCharacterPanelWidget::HandleUnsocketMinor3() { UnsocketSlot(3); }
