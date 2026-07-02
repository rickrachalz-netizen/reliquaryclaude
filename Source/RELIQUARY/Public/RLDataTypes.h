// RELIQUARY — DataTable row definitions. All game content (materials, gear,
// recipes, boons, zones, enemies, classes, specs, talents, essences) is
// authored in CSV files (see /Data at the repo root) and imported into
// DataTables under /Game/Data. Row names are the stable IDs referenced by
// saves and code.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "RLTypes.h"
#include "RLDataTypes.generated.h"

class UGameplayEffect;
class URLGameplayAbility;

/**
 * Flat attribute bonuses used by gear, talents, boons, and essences.
 * Named float columns keep the CSVs legible and hand-editable.
 */
USTRUCT(BlueprintType)
struct FRLStatBlock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float Health = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float Mana = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float HealthRegen = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float ManaRegen = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float Strength = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float Agility = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float Intellect = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float CritChance = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float CritDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float Haste = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float Armor = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float MoveSpeed = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float Adaptability = 0.f;

	bool IsEmpty() const
	{
		return Health == 0.f && Mana == 0.f && HealthRegen == 0.f && ManaRegen == 0.f
			&& Strength == 0.f && Agility == 0.f && Intellect == 0.f
			&& CritChance == 0.f && CritDamage == 0.f && Haste == 0.f
			&& Armor == 0.f && MoveSpeed == 0.f && Adaptability == 0.f;
	}

	FRLStatBlock operator*(float Scale) const
	{
		FRLStatBlock Out;
		Out.Health = Health * Scale;           Out.Mana = Mana * Scale;
		Out.HealthRegen = HealthRegen * Scale; Out.ManaRegen = ManaRegen * Scale;
		Out.Strength = Strength * Scale;       Out.Agility = Agility * Scale;
		Out.Intellect = Intellect * Scale;     Out.CritChance = CritChance * Scale;
		Out.CritDamage = CritDamage * Scale;   Out.Haste = Haste * Scale;
		Out.Armor = Armor * Scale;             Out.MoveSpeed = MoveSpeed * Scale;
		Out.Adaptability = Adaptability * Scale;
		return Out;
	}
};

/** Materials, weapons, armor, and consumables — one table, DT_Items. */
USTRUCT(BlueprintType)
struct FRLItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	ERLItemType ItemType = ERLItemType::Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	ERLEquipSlot EquipSlot = ERLEquipSlot::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	ERLRarity Rarity = ERLRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 MaxStack = 99;

	/** Stat bonuses granted while equipped (gear only). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Gear")
	FRLStatBlock Stats;

	/** Cantrip: a passive GameplayEffect applied while the piece is worn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Gear")
	TSoftClassPtr<UGameplayEffect> CantripEffect;

	/** Gear set this piece belongs to (Set.* tags); empty = no set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Gear")
	FGameplayTag SetTag;
};

/** Set bonuses — DT_SetBonuses. Row per (set, piece-count) threshold. */
USTRUCT(BlueprintType)
struct FRLSetBonusRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set")
	FGameplayTag SetTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set")
	int32 RequiredPieces = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set")
	FRLStatBlock Stats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set")
	TSoftClassPtr<UGameplayEffect> BonusEffect;
};

/** Crafting recipes — DT_Recipes. Up to three distinct ingredients. */
USTRUCT(BlueprintType)
struct FRLRecipeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName ResultItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	int32 ResultCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName Ingredient1Id = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	int32 Ingredient1Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName Ingredient2Id = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	int32 Ingredient2Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName Ingredient3Id = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	int32 Ingredient3Count = 0;

	void GetIngredients(TArray<FRLItemStack>& Out) const
	{
		if (Ingredient1Id != NAME_None && Ingredient1Count > 0) { Out.Add({ Ingredient1Id, Ingredient1Count }); }
		if (Ingredient2Id != NAME_None && Ingredient2Count > 0) { Out.Add({ Ingredient2Id, Ingredient2Count }); }
		if (Ingredient3Id != NAME_None && Ingredient3Count > 0) { Out.Add({ Ingredient3Id, Ingredient3Count }); }
	}
};

/** Temporary run boons bought at upgrade altars with excess mana — DT_Boons. */
USTRUCT(BlueprintType)
struct FRLBoonRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boon")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boon")
	ERLRarity Rarity = ERLRarity::Common;

	/** Excess mana price at the altar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boon")
	int32 ManaCost = 50;

	/** Selection weight within its rarity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boon")
	float Weight = 1.f;

	/** How many times this boon can stack in one run (0 = unlimited). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boon")
	int32 MaxStacks = 0;

	/** Stats granted per stack, applied for the rest of the run. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boon")
	FRLStatBlock Stats;

	/** Optional bespoke effect for boons that do more than stats. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boon")
	TSoftClassPtr<UGameplayEffect> GrantedEffect;
};

/** The realm's path: ten zones — DT_Zones, rows Zone1..Zone10. */
USTRUCT(BlueprintType)
struct FRLZoneRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	int32 ZoneIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FText Description;

	/** Level asset to travel to; multiple zones may share one map shell. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	TSoftObjectPtr<UWorld> Map;

	/** Materials that reliably spawn here — this is how players plan routes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FName Material1 = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FName Material2 = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FName Material3 = NAME_None;

	/** Fixed number of upgrade altars scattered per visit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	int32 UpgradeAltarCount = 3;

	/** Resource node count scattered per visit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	int32 ResourceNodeCount = 18;

	/** Income scale for this zone's enemy directors (1.0 = baseline). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	float DirectorCreditRate = 1.f;

	/** Spawn-card row (DT_SpawnCards) for this zone's challenge-altar boss. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FName BossCardId = NAME_None;

	void GetMaterials(TArray<FName>& Out) const
	{
		if (Material1 != NAME_None) { Out.Add(Material1); }
		if (Material2 != NAME_None) { Out.Add(Material2); }
		if (Material3 != NAME_None) { Out.Add(Material3); }
	}
};

/** Director spawn cards — DT_SpawnCards. */
USTRUCT(BlueprintType)
struct FRLSpawnCardRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TSoftClassPtr<APawn> EnemyClass;

	/** Director credit cost to spawn one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	float Cost = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	float Weight = 1.f;

	/** Zone gating: card only appears from this zone index onward. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	int32 MinZone = 1;

	/** Difficulty gating: card only appears once the coefficient passes this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	float MinDifficulty = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	bool bBoss = false;
};

/** Base class definitions — DT_Classes, rows Warrior/Rogue/Mage. */
USTRUCT(BlueprintType)
struct FRLClassRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	ERLHeroClass HeroClass = ERLHeroClass::Warrior;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	FText DisplayName;

	/** Attributes at level 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	FRLStatBlock BaseStats;

	/** Attribute growth per level beyond 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	FRLStatBlock StatsPerLevel;

	/** Kit granted at creation, RoR2 style: primary/secondary/utility/special. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	TSoftClassPtr<URLGameplayAbility> PrimaryAbility;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	TSoftClassPtr<URLGameplayAbility> SecondaryAbility;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	TSoftClassPtr<URLGameplayAbility> UtilityAbility;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	TSoftClassPtr<URLGameplayAbility> SpecialAbility;
};

/** Specs (talent trees) — DT_Specs. Three per class, WoW style. */
USTRUCT(BlueprintType)
struct FRLSpecRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	ERLHeroClass HeroClass = ERLHeroClass::Warrior;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	FText Description;

	/**
	 * Evolved identity: at max level the class name shown in the UI becomes
	 * this title if the hero has invested the most talent points here.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	FText EvolvedTitle;
};

/** Individual talents — DT_Talents. TreeId matches a DT_Specs row name. */
USTRUCT(BlueprintType)
struct FRLTalentRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Talent")
	FName TreeId = NAME_None;

	/** Tier gating: requires Tier * 5 points already spent in this tree. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Talent")
	int32 Tier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Talent")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Talent")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Talent")
	int32 MaxRanks = 3;

	/** Stats granted per rank. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Talent")
	FRLStatBlock StatsPerRank;

	/** Optional ability unlocked at rank 1 (keystone talents). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Talent")
	TSoftClassPtr<URLGameplayAbility> GrantedAbility;
};

/** Reliquary Shard essences — DT_Essences (BfA 8.3 Heart of Azeroth style). */
USTRUCT(BlueprintType)
struct FRLEssenceRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essence")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essence")
	FText Description;

	/** Active ability granted only while socketed in the major slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essence")
	TSoftClassPtr<URLGameplayAbility> MajorAbility;

	/** Passive stats granted per essence rank (1..4) in any slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essence")
	FRLStatBlock StatsPerRank;

	/** Material that upgrades this essence, and the base cost per rank. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essence")
	FName UpgradeMaterialId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essence")
	int32 UpgradeCostPerRank = 10;
};
