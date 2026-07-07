#include "RLAbilityBarWidget.h"
#include "RLAbilitySlotWidget.h"
#include "RLGameplayTags.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

bool URLAbilityBarWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	// Pure native use (no WBP tree): the row is the whole widget.
	if (!SlotRow && WidgetTree && !WidgetTree->RootWidget)
	{
		SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotRow"));
		WidgetTree->RootWidget = SlotRow;
	}

	return true;
}

void URLAbilityBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Slots are UserWidgets, so they need the runtime CreateWidget path;
	// populate once, when the bar first enters the screen.
	if (SlotRow && SlotRow->GetChildrenCount() == 0)
	{
		AddSlotWidget(RLTags::Ability_Primary,   NSLOCTEXT("RL", "KeyPrimary", "LMB"),   false);
		AddSlotWidget(RLTags::Ability_Secondary, NSLOCTEXT("RL", "KeySecondary", "RMB"), false);
		AddSlotWidget(RLTags::Ability_Utility,   NSLOCTEXT("RL", "KeyUtility", "SHIFT"), false);
		AddSlotWidget(RLTags::Ability_Special,   NSLOCTEXT("RL", "KeySpecial", "R"),     false);
		AddSlotWidget(RLTags::Ability_Essence,   NSLOCTEXT("RL", "KeyEssence", "Q"),     true);
	}
}

void URLAbilityBarWidget::AddSlotWidget(const FGameplayTag& ActionTag, const FText& KeybindLabel, bool bCollapseWhenEmpty)
{
	const TSubclassOf<URLAbilitySlotWidget> ClassToUse =
		SlotWidgetClass ? *SlotWidgetClass : URLAbilitySlotWidget::StaticClass();
	URLAbilitySlotWidget* SlotWidget = CreateWidget<URLAbilitySlotWidget>(this, ClassToUse);
	if (!SlotWidget)
	{
		return;
	}

	SlotWidget->ActionTag = ActionTag;
	SlotWidget->KeybindLabel = KeybindLabel;
	SlotWidget->bCollapseWhenEmpty = bCollapseWhenEmpty;

	if (UHorizontalBoxSlot* BoxSlot = SlotRow->AddChildToHorizontalBox(SlotWidget))
	{
		BoxSlot->SetPadding(FMargin(3.f, 0.f));
	}
}
