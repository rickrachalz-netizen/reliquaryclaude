// RELIQUARY — crafting, the heart of the game. All crafting happens at base
// camp against the stash; runs only ever gather.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RLCraftingLibrary.generated.h"

UCLASS()
class RELIQUARY_API URLCraftingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** True when the stash holds every ingredient of the recipe. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Crafting", meta = (WorldContext = "WorldContextObject"))
	static bool CanCraft(const UObject* WorldContextObject, FName RecipeId);

	/**
	 * Consumes ingredients from the stash and adds the result. Returns false
	 * (and consumes nothing) when ingredients are missing.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Crafting", meta = (WorldContext = "WorldContextObject"))
	static bool Craft(const UObject* WorldContextObject, FName RecipeId);

	/** Row names of every recipe the stash can currently afford. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Crafting", meta = (WorldContext = "WorldContextObject"))
	static void GetCraftableRecipes(const UObject* WorldContextObject, TArray<FName>& OutRecipeIds);
};
