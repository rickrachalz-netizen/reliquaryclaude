#include "RLCharacterPanelWidget.h"
#include "RELIQUARYCharacter.h"
#include "RLAbilitySystemComponent.h"
#include "RLAttributeSet.h"
#include "RLGameInstance.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "RLTypes.h"
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

		WidgetTree->RootWidget = Backing;
	}

	return true;
}

void URLCharacterPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshEssences();
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
	float TrancePct = 0.f;
	if (const ARELIQUARYCharacter* Hero = Cast<ARELIQUARYCharacter>(GetOwningPlayerPawn()))
	{
		TrancePct = Hero->GetBattleTranceIntensity();
	}

	// Order must match GStatLabels above.
	TArray<FString> Values;
	Values.Add(FString::Printf(TEXT("%.0f / %.0f"), Attr(URLAttributeSet::GetHealthAttribute()), Attr(URLAttributeSet::GetMaxHealthAttribute())));
	Values.Add(FString::Printf(TEXT("%.1f/s"), Attr(URLAttributeSet::GetHealthRegenAttribute())));
	Values.Add(FString::Printf(TEXT("%.0f / %.0f"), Attr(URLAttributeSet::GetManaAttribute()), Attr(URLAttributeSet::GetMaxManaAttribute())));
	Values.Add(FString::Printf(TEXT("%.1f/s"), Attr(URLAttributeSet::GetManaRegenAttribute())));
	Values.Add(FString::Printf(TEXT("%.0f"), Attr(URLAttributeSet::GetStrengthAttribute())));
	Values.Add(FString::Printf(TEXT("%.0f"), Attr(URLAttributeSet::GetAgilityAttribute())));
	Values.Add(FString::Printf(TEXT("%.0f"), Attr(URLAttributeSet::GetIntellectAttribute())));
	Values.Add(FString::Printf(TEXT("%.1f%%"), Attr(URLAttributeSet::GetCritChanceAttribute()) * 100.f));
	Values.Add(FString::Printf(TEXT("x%.2f"), Attr(URLAttributeSet::GetCritDamageAttribute())));
	Values.Add(FString::Printf(TEXT("%.1f%%"), Attr(URLAttributeSet::GetHasteAttribute()) * 100.f));
	Values.Add(FString::Printf(TEXT("%.0f"), Attr(URLAttributeSet::GetArmorAttribute())));
	Values.Add(FString::Printf(TEXT("%.0f"), Attr(URLAttributeSet::GetMoveSpeedAttribute())));
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
