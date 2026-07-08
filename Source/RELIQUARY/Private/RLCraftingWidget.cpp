#include "RLCraftingWidget.h"
#include "RLCraftingLibrary.h"
#include "RLGameInstance.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "RLTypes.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, int32 FontSize = 12, bool bBold = false)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(FText::FromString(Text));
		Block->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FontSize));
		return Block;
	}

	FText ItemDisplayName(URLDataSubsystem* Data, FName ItemId)
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

// --- Recipe row ---

void URLCraftingRecipeEntry::Setup(URLCraftingWidget* InOwner, FName InRecipeId)
{
	Owner = InOwner;
	RecipeId = InRecipeId;

	URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance());
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;
	const FRLRecipeRow* Recipe = Data ? Data->FindRecipe(RecipeId) : nullptr;
	if (!Recipe || !WidgetTree)
	{
		return;
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	WidgetTree->RootWidget = Row;

	// Left: result + ingredient breakdown.
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	const FString Header = FString::Printf(TEXT("%s x%d"),
		*ItemDisplayName(Data, Recipe->ResultItemId).ToString(), Recipe->ResultCount);
	Info->AddChildToVerticalBox(MakeText(WidgetTree, Header, 14, true));

	TArray<FRLItemStack> Ingredients;
	Recipe->GetIngredients(Ingredients);
	for (const FRLItemStack& Ing : Ingredients)
	{
		const int32 Have = GI ? GI->CountInStash(Ing.ItemId) : 0;
		const FString Line = FString::Printf(TEXT("   %s  %d/%d"),
			*ItemDisplayName(Data, Ing.ItemId).ToString(), Have, Ing.Count);
		Info->AddChildToVerticalBox(MakeText(WidgetTree, Line, 11, false));
	}

	if (UHorizontalBoxSlot* InfoSlot = Row->AddChildToHorizontalBox(Info))
	{
		InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		InfoSlot->SetVerticalAlignment(VAlign_Center);
	}

	// Right: Craft button, disabled when unaffordable.
	UButton* CraftButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CraftButton->AddChild(MakeText(WidgetTree, TEXT("Craft"), 12, true));
	CraftButton->SetIsEnabled(URLCraftingLibrary::CanCraft(this, RecipeId));
	FScriptDelegate Del;
	Del.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(URLCraftingRecipeEntry, HandleCraft));
	CraftButton->OnClicked.Add(Del);
	if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(CraftButton))
	{
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void URLCraftingRecipeEntry::HandleCraft()
{
	if (Owner.IsValid())
	{
		Owner->CraftRecipe(RecipeId);
	}
}

// --- Crafting panel ---

bool URLCraftingWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	// Pure native use (no WBP tree): build a simple forge panel.
	if (!RecipeList && WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
		Root->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
		Root->SetPadding(FMargin(24.f));
		Root->SetHorizontalAlignment(HAlign_Center);
		Root->SetVerticalAlignment(VAlign_Fill);

		UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Outer->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Forge"), 20, true));

		UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Close->AddChild(MakeText(WidgetTree, TEXT("Close"), 12, false));
		FScriptDelegate Del;
		Del.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(URLCraftingWidget, HandleClose));
		Close->OnClicked.Add(Del);
		Outer->AddChildToVerticalBox(Close);

		RecipeList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RecipeList"));
		if (UVerticalBoxSlot* ListSlot = Outer->AddChildToVerticalBox(RecipeList))
		{
			ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ListSlot->SetPadding(FMargin(0.f, 8.f));
		}

		Root->AddChild(Outer);
		WidgetTree->RootWidget = Root;
	}
	return true;
}

void URLCraftingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(true);
		PC->SetInputMode(FInputModeGameAndUI());
	}
	RefreshRecipes();
}

URLGameInstance* URLCraftingWidget::GetRLGameInstance() const
{
	return Cast<URLGameInstance>(GetGameInstance());
}

void URLCraftingWidget::RefreshRecipes()
{
	if (!RecipeList)
	{
		return;
	}

	RecipeList->ClearChildren();

	TArray<FName> RecipeIds;
	URLCraftingLibrary::GetAllRecipeIds(this, RecipeIds);

	const TSubclassOf<URLCraftingRecipeEntry> EntryClass =
		EntryWidgetClass ? *EntryWidgetClass : URLCraftingRecipeEntry::StaticClass();

	for (const FName& RecipeId : RecipeIds)
	{
		if (URLCraftingRecipeEntry* Entry = CreateWidget<URLCraftingRecipeEntry>(this, EntryClass))
		{
			Entry->Setup(this, RecipeId);
			RecipeList->AddChild(Entry);
		}
	}
}

void URLCraftingWidget::CraftRecipe(FName RecipeId)
{
	URLCraftingLibrary::Craft(this, RecipeId);
	RefreshRecipes();
}

void URLCraftingWidget::HandleClose()
{
	CloseCrafting();
}

void URLCraftingWidget::CloseCrafting()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
	RemoveFromParent();
}
