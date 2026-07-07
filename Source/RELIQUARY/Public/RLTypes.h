// RELIQUARY — shared enums and structs used across gameplay systems.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RLTypes.generated.h"

/** The three base classes chosen at hero creation. */
UENUM(BlueprintType)
enum class ERLHeroClass : uint8
{
	Warrior,
	Rogue,
	Mage
};

/** Broad item categories. */
UENUM(BlueprintType)
enum class ERLItemType : uint8
{
	Material,	// distinct gatherables: oak, ironwood, feywood, riverstone...
	Weapon,
	Armor,
	Consumable
};

/** Equipment slots. Multiple weapon and armor types share these slots. */
UENUM(BlueprintType)
enum class ERLEquipSlot : uint8
{
	None,
	Weapon,
	Head,
	Chest,
	Legs,
	Hands,
	Trinket
};

/** Rarity drives boon/gear power tiers and UI color. */
UENUM(BlueprintType)
enum class ERLRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

/** High-level state of the current expedition. */
UENUM(BlueprintType)
enum class ERLRunState : uint8
{
	AtBaseCamp,		// safe ground on the fallen Wild God
	InZone,			// exploring / gathering / fighting
	AltarChallenge,	// challenge altar active, empowered boss alive or altar charging
	AltarCharged,	// boss dead + altar charged: choose extract or push on
	Extracting,		// returning home with the haul
	Dead			// died on the run; resources forfeit
};

/** A stack of one item kind, identified by its DataTable row name. */
USTRUCT(BlueprintType)
struct FRLItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item")
	int32 Count = 0;

	bool IsValid() const { return ItemId != NAME_None && Count > 0; }
};

/** One rank purchase in a talent tree, keyed by talent row name. */
USTRUCT(BlueprintType)
struct FRLTalentRank
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Talents")
	FName TalentId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Talents")
	int32 Ranks = 0;
};

/** An essence socketed into the hero's Reliquary Shard (BfA 8.3-style). */
USTRUCT(BlueprintType)
struct FRLSocketedEssence
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Essences")
	FName EssenceId = NAME_None;

	/** Slot 0 is the major slot (grants active + passive); others are minor (passive only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Essences")
	int32 SlotIndex = 0;

	/** Essence rank 1..4, upgraded with banked materials. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Essences")
	int32 Rank = 1;
};

/**
 * A persistent, player-created hero. Everything here survives between runs
 * and is what the save game serializes. Roguelike run power never touches this.
 */
USTRUCT(BlueprintType)
struct FRLHeroData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	FString HeroName = TEXT("Wanderer");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	ERLHeroClass HeroClass = ERLHeroClass::Warrior;

	/** Chosen spec row name from DT_Specs (drives talent tree + evolved title). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	FName SpecId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	int64 Experience = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	TArray<FRLTalentRank> Talents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	TArray<FRLSocketedEssence> Essences;

	/**
	 * Essences this hero has learned. Enemy-sourced essences unlock the first
	 * time that enemy type dies to the hero; world-sourced ones unlock when a
	 * world event calls URLGameInstance::UnlockEssence. Only unlocked essences
	 * can be socketed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	TArray<FName> UnlockedEssenceIds;

	/** Reliquary Shard level — raises essence rank caps (Heart of Azeroth analog). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	int32 ShardLevel = 1;

	/** Materials and gear stored safely at base camp. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	TArray<FRLItemStack> Stash;

	/** Currently equipped gear: slot -> item row name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	TMap<ERLEquipSlot, FName> Equipped;

	/** Lifetime bookkeeping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	int32 RunsCompleted = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	int32 DeepestZoneReached = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Hero")
	bool bDefeatedWildGod = false;
};

/** Outcome summary handed to UI when a run ends, however it ends. */
USTRUCT(BlueprintType)
struct FRLRunSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	bool bDied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 ZonesCleared = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float RunSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	TArray<FRLItemStack> ExtractedResources;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int64 ExperienceEarned = 0;
};

namespace RLBalance
{
	/** Hero level cap. */
	constexpr int32 MaxHeroLevel = 30;

	/** Number of zones on the realm's path. */
	constexpr int32 ZonesPerRun = 10;

	/** A banking crate is offered after every Nth cleared zone. */
	constexpr int32 ZonesPerBankingCrate = 3;

	/** Talent points earned per level from level 2 onward. */
	constexpr int32 TalentPointsPerLevel = 1;

	/** Essence minor slots unlock at these hero levels (major slot at 10). */
	constexpr int32 MajorEssenceUnlockLevel = 10;
	constexpr int32 MinorEssenceUnlockLevels[3] = { 18, 26, 30 };

	// --- Excess mana economy (the roguelike run currency) ---

	/** Base excess mana a resource node yields when shattered (pre-scaling). */
	constexpr int32 BaseNodeManaYield = 8;

	/** Base excess mana a normal enemy drops on death (pre-scaling). */
	constexpr int32 BaseEnemyManaReward = 5;

	/** Elites drop this multiple of their base mana; bosses drop 4x (bEmpowered). */
	constexpr float EliteManaMultiplier = 2.f;
	constexpr float BossManaMultiplier = 4.f;

	/** Mana packed into each physical orb; a reward splits into orbs of this size. */
	constexpr int32 ManaOrbUnitValue = 5;

	/**
	 * Mana rewards and altar prices both scale with the run's difficulty
	 * coefficient, so the economy holds its shape as the realm grows lethal
	 * (Risk of Rain 2 chest-price idiom). Never rounds below 1.
	 */
	FORCEINLINE int32 ScaledManaReward(int32 Base, float Difficulty)
	{
		return FMath::Max(1, FMath::RoundToInt(Base * Difficulty));
	}

	/**
	 * XP required to go from `Level` to `Level + 1`.
	 * Exponential per the production map: fast early, each level ~35% more.
	 * Level 1->2 costs 100 XP; 29->30 costs ~about 500k. Leveling stays quick
	 * because kill/gather XP also scales with zone depth and difficulty.
	 */
	FORCEINLINE int64 XPForNextLevel(int32 Level)
	{
		return static_cast<int64>(100.0 * FMath::Pow(1.35, static_cast<double>(Level - 1)));
	}

	/**
	 * Run difficulty coefficient, Risk of Rain 2 style: creeps up with time
	 * and jumps with each zone transition. Feeds enemy HP/damage multipliers
	 * and director credit income.
	 */
	FORCEINLINE float DifficultyCoefficient(float RunSeconds, int32 ZoneIndex)
	{
		const float TimeFactor = 1.f + (RunSeconds / 60.f) * 0.08f;	// +8% per minute
		const float ZoneFactor = FMath::Pow(1.20f, static_cast<float>(FMath::Max(ZoneIndex - 1, 0)));	// +20% per zone
		return TimeFactor * ZoneFactor;
	}
}
