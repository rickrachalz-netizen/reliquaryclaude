// RELIQUARY — persistent hero roster, leveling, talents, essences, stash.
// Reparent Content/BP_RLGameInstance to this class.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RLTypes.h"
#include "RLGameInstance.generated.h"

class URLDataSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRLHeroChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRLLeveledUpSignature, int32, NewLevel, bool, bMaxLevel);

/**
 * Owns everything that survives between runs. The game autosaves whenever
 * the player enters base camp (called by ARLBaseCampGameMode); active runs
 * are never saved.
 */
UCLASS()
class RELIQUARY_API URLGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	// --- Roster ---

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Heroes")
	int32 CreateHero(const FString& HeroName, ERLHeroClass HeroClass, FName SpecId);

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Heroes")
	bool SelectHero(int32 HeroIndex);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Heroes")
	int32 GetHeroCount() const { return Heroes.Num(); }

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Heroes")
	FRLHeroData GetActiveHeroData() const;

	/** Mutable access for systems; may return nullptr before any hero exists. */
	FRLHeroData* GetActiveHero();
	const FRLHeroData* GetActiveHero() const;

	// --- Leveling (fast by design, exponential cost, cap 30) ---

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Progression")
	int32 AddExperience(int64 Amount);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Progression")
	int64 GetXPForNextLevel() const;

	/** Unspent talent points: (Level-1) * PointsPerLevel minus spent. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Progression")
	int32 GetAvailableTalentPoints() const;

	/** Ranks the active hero owns in a talent (0 when absent). */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Progression")
	int32 GetTalentRank(FName TalentId) const;

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Progression")
	bool SpendTalentPoint(FName TalentId);

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Progression")
	void ResetTalents();

	/**
	 * The class name shown in the UI. Below max level this is the base class;
	 * at level 30 it evolves into the dominant spec's title (e.g. a Warrior
	 * deep in Juggernaut becomes "Godbreaker").
	 */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Progression")
	FText GetDisplayedClassName() const;

	// --- Reliquary Shard essences ---

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Essences")
	int32 GetUnlockedEssenceSlots() const;

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Essences")
	bool SocketEssence(FName EssenceId, int32 SlotIndex);

	/** Spends UpgradeMaterial from the stash to raise an essence's rank. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Essences")
	bool UpgradeEssence(FName EssenceId);

	// --- Stash & equipment (base camp only) ---

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Stash")
	void AddToStash(const TArray<FRLItemStack>& Stacks);

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Stash")
	bool RemoveFromStash(FName ItemId, int32 Count);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Stash")
	int32 CountInStash(FName ItemId) const;

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Stash")
	bool EquipFromStash(FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Stash")
	bool Unequip(ERLEquipSlot Slot);

	// --- Persistence ---

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Save")
	void SaveToDisk();

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Save")
	void LoadFromDisk();

	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Heroes")
	FRLHeroChangedSignature OnActiveHeroChanged;

	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Progression")
	FRLLeveledUpSignature OnLeveledUp;

protected:
	UPROPERTY()
	TArray<FRLHeroData> Heroes;

	UPROPERTY()
	int32 ActiveHeroIndex = INDEX_NONE;

	URLDataSubsystem* GetData() const;
};
