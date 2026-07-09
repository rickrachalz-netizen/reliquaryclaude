// RELIQUARY — the forge UI. Opened by ARLCraftingStation, it lists every known
// recipe against the stash and crafts on click. Works with zero content; to
// restyle, reparent a WBP to this class and supply a ScrollBox named "RecipeList".

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLCraftingWidget.generated.h"

class UScrollBox;
class UTextBlock;
class UButton;
class URLGameInstance;
class URLCraftingWidget;

/**
 * One recipe row: result name, ingredient costs (have/need), and a Craft button
 * that reports its own recipe id back to the panel (UButton::OnClicked carries
 * no payload). Built natively, mirroring URLEssencePanelEntry.
 */
UCLASS()
class RELIQUARY_API URLCraftingRecipeEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Build the row for RecipeId and wire its button back to the owner. */
	void Setup(URLCraftingWidget* InOwner, FName InRecipeId);

protected:
	UPROPERTY()
	TWeakObjectPtr<URLCraftingWidget> Owner;

	FName RecipeId = NAME_None;

	UFUNCTION() void HandleCraft();
};

UCLASS()
class RELIQUARY_API URLCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Rebuild the recipe list from the data tables + current stash. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Crafting")
	void RefreshRecipes();

	/** Craft the recipe (against the stash) and rebuild. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Crafting")
	void CraftRecipe(FName RecipeId);

	/** Dismiss the panel and hand input back to the game. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Crafting")
	void CloseCrafting();

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;

	UFUNCTION() void HandleClose();

	URLGameInstance* GetRLGameInstance() const;

	/** Bound from a WBP subclass, or built natively when absent. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Crafting")
	TObjectPtr<UScrollBox> RecipeList;

	/** Entry class to spawn per recipe (defaults to the native row). */
	UPROPERTY(EditAnywhere, Category = "RELIQUARY|Crafting")
	TSubclassOf<URLCraftingRecipeEntry> EntryWidgetClass;
};
