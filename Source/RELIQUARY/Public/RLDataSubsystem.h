// RELIQUARY — central registry for all gameplay DataTables.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "RLDataTypes.h"
#include "RLXoshiro.h"
#include "RLDataSubsystem.generated.h"

/**
 * Loads the game's DataTables once at startup and hands out typed rows.
 * Table asset paths default to /Game/Data/DT_* and can be overridden in
 * DefaultGame.ini under [/Script/RELIQUARY.RLDataSubsystem].
 */
UCLASS(Config = Game)
class RELIQUARY_API URLDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- Typed row lookups (nullptr when missing) ---
	const FRLItemRow* FindItem(FName ItemId) const;
	const FRLRecipeRow* FindRecipe(FName RecipeId) const;
	const FRLBoonRow* FindBoon(FName BoonId) const;
	const FRLZoneRow* FindZone(int32 ZoneIndex) const;
	const FRLSpawnCardRow* FindSpawnCard(FName CardId) const;
	const FRLClassRow* FindClass(ERLHeroClass HeroClass) const;
	const FRLSpecRow* FindSpec(FName SpecId) const;
	const FRLEssenceRow* FindEssence(FName EssenceId) const;

	/** All talents belonging to one spec's tree. */
	void GetTalentsForTree(FName TreeId, TArray<TPair<FName, const FRLTalentRow*>>& Out) const;

	/** All specs belonging to one class. */
	void GetSpecsForClass(ERLHeroClass HeroClass, TArray<TPair<FName, const FRLSpecRow*>>& Out) const;

	/** All recipes, for crafting UI. */
	void GetAllRecipes(TArray<TPair<FName, const FRLRecipeRow*>>& Out) const;

	/** Set bonus rows for a set tag, sorted by RequiredPieces ascending. */
	void GetSetBonuses(const FGameplayTag& SetTag, TArray<const FRLSetBonusRow*>& Out) const;

	/**
	 * Weighted random draw of boon row names, excluding those already at max
	 * stacks. Used by upgrade altars to build their three offered choices.
	 */
	void DrawRandomBoons(int32 Count, const TMap<FName, int32>& CurrentStacks,
		FRLXoshiro256& Rng, TArray<FName>& Out) const;

	/** Weighted random non-boss spawn card affordable within MaxCost. */
	FName DrawSpawnCard(int32 ZoneIndex, float Difficulty, float MaxCost, FRLXoshiro256& Rng) const;

	UDataTable* GetItemTable() const { return ItemTable; }
	UDataTable* GetBoonTable() const { return BoonTable; }
	UDataTable* GetTalentTable() const { return TalentTable; }

protected:
	UPROPERTY(Config) FSoftObjectPath ItemTablePath = FSoftObjectPath(TEXT("/Game/Data/DT_Items.DT_Items"));
	UPROPERTY(Config) FSoftObjectPath SetBonusTablePath = FSoftObjectPath(TEXT("/Game/Data/DT_SetBonuses.DT_SetBonuses"));
	UPROPERTY(Config) FSoftObjectPath RecipeTablePath = FSoftObjectPath(TEXT("/Game/Data/DT_Recipes.DT_Recipes"));
	UPROPERTY(Config) FSoftObjectPath BoonTablePath = FSoftObjectPath(TEXT("/Game/Data/DT_Boons.DT_Boons"));
	UPROPERTY(Config) FSoftObjectPath ZoneTablePath = FSoftObjectPath(TEXT("/Game/Data/DT_Zones.DT_Zones"));
	UPROPERTY(Config) FSoftObjectPath SpawnCardTablePath = FSoftObjectPath(TEXT("/Game/Data/DT_SpawnCards.DT_SpawnCards"));
	UPROPERTY(Config) FSoftObjectPath ClassTablePath = FSoftObjectPath(TEXT("/Game/Data/DT_Classes.DT_Classes"));
	UPROPERTY(Config) FSoftObjectPath SpecTablePath = FSoftObjectPath(TEXT("/Game/Data/DT_Specs.DT_Specs"));
	UPROPERTY(Config) FSoftObjectPath TalentTablePath = FSoftObjectPath(TEXT("/Game/Data/DT_Talents.DT_Talents"));
	UPROPERTY(Config) FSoftObjectPath EssenceTablePath = FSoftObjectPath(TEXT("/Game/Data/DT_Essences.DT_Essences"));

	UPROPERTY(Transient) TObjectPtr<UDataTable> ItemTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> SetBonusTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> RecipeTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> BoonTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> ZoneTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> SpawnCardTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> ClassTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> SpecTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> TalentTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> EssenceTable;

	static UDataTable* LoadTable(const FSoftObjectPath& Path);
};
