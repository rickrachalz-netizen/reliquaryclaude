#include "RLCraftingLibrary.h"
#include "RLGameInstance.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "RLTypes.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	URLGameInstance* GetRLGameInstance(const UObject* WorldContextObject)
	{
		return Cast<URLGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
	}

	bool HasIngredients(URLGameInstance* GI, const FRLRecipeRow& Recipe)
	{
		TArray<FRLItemStack> Ingredients;
		Recipe.GetIngredients(Ingredients);
		for (const FRLItemStack& Ingredient : Ingredients)
		{
			if (GI->CountInStash(Ingredient.ItemId) < Ingredient.Count)
			{
				return false;
			}
		}
		return Ingredients.Num() > 0;
	}
}

bool URLCraftingLibrary::CanCraft(const UObject* WorldContextObject, FName RecipeId)
{
	URLGameInstance* GI = GetRLGameInstance(WorldContextObject);
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;
	const FRLRecipeRow* Recipe = Data ? Data->FindRecipe(RecipeId) : nullptr;
	return Recipe && HasIngredients(GI, *Recipe);
}

bool URLCraftingLibrary::Craft(const UObject* WorldContextObject, FName RecipeId)
{
	URLGameInstance* GI = GetRLGameInstance(WorldContextObject);
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;
	const FRLRecipeRow* Recipe = Data ? Data->FindRecipe(RecipeId) : nullptr;
	if (!Recipe || !HasIngredients(GI, *Recipe))
	{
		return false;
	}

	TArray<FRLItemStack> Ingredients;
	Recipe->GetIngredients(Ingredients);
	for (const FRLItemStack& Ingredient : Ingredients)
	{
		GI->RemoveFromStash(Ingredient.ItemId, Ingredient.Count);
	}

	TArray<FRLItemStack> Result;
	Result.Add({ Recipe->ResultItemId, Recipe->ResultCount });
	GI->AddToStash(Result);
	GI->SaveToDisk();
	return true;
}

void URLCraftingLibrary::GetCraftableRecipes(const UObject* WorldContextObject, TArray<FName>& OutRecipeIds)
{
	URLGameInstance* GI = GetRLGameInstance(WorldContextObject);
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;
	if (!Data)
	{
		return;
	}

	TArray<TPair<FName, const FRLRecipeRow*>> Recipes;
	Data->GetAllRecipes(Recipes);
	for (const auto& Pair : Recipes)
	{
		if (HasIngredients(GI, *Pair.Value))
		{
			OutRecipeIds.Add(Pair.Key);
		}
	}
}
