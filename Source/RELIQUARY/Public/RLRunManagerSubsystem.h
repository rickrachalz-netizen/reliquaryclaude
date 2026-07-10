// RELIQUARY — the extraction-run state machine. Owns everything that lives
// and dies with a single run: timer, difficulty, zone index, excess mana,
// gathered resources, and purchased boons. All of it resets the moment the
// hero returns to base camp.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RLXoshiro.h"
#include "RLTypes.h"
#include "RLRunManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRLRunStateChangedSignature, ERLRunState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRLExcessManaChangedSignature, int32, NewAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRLRunInventoryChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRLRunEndedSignature, FRLRunSummary, Summary);

UCLASS(Config = Game)
class RELIQUARY_API URLRunManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- Run lifecycle ---

	/**
	 * Rolls and locks the run seed (no map fishing: the RNG for all ten
	 * zones is decided here), then travels to zone 1.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Run")
	void EmbarkOnRun();

	/**
	 * Starts run state on the already-loaded map (PIE straight into a run
	 * level, or the embark portal standing inside zone 1 itself).
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Run")
	void EmbarkInPlace();

	/** Challenge altar, boss dead + charged: push deeper along the path. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Run")
	void AdvanceToNextZone();

	/** Challenge altar, boss dead + charged: go home with the haul. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Run")
	void ExtractToBaseCamp();

	/**
	 * Extract from ANY on-run state: bank the haul and travel home. The hook
	 * for bespoke extract actors (e.g. a placed extract altar Blueprint) that
	 * aren't part of the charged-challenge-altar flow. No-op off-run.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Run")
	void RequestExtraction();

	/**
	 * Called by the base camp game mode on arrival. If a run is somehow still
	 * live (a Blueprint opened L_Lobby directly instead of extracting), the
	 * haul is banked instead of silently wiped, then run state resets. Always
	 * leaves the state at AtBaseCamp.
	 */
	void NotifyArrivedAtBaseCamp();

	/** Death: forfeit everything gathered (banked crates stay safe). */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Run")
	void HandleHeroDeath();

	/** Zone 10 boss (or the Wild God itself) has fallen. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Run")
	void HandleFinalBossKilled(bool bWildGod);

	/**
	 * Quit the current run from a menu: drop all run-scoped state without
	 * banking (carried resources are forfeit, as on death). Banked crates and
	 * XP already earned this run stay. No-op at base camp.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Run")
	void AbandonRun();

	/** Called by challenge altars to move the state machine along. */
	void SetRunState(ERLRunState NewState);

	// --- Queries ---

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Run")
	ERLRunState GetRunState() const { return RunState; }

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Run")
	bool IsOnRun() const { return RunState != ERLRunState::AtBaseCamp && RunState != ERLRunState::Dead; }

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Run")
	int32 GetCurrentZoneIndex() const { return CurrentZoneIndex; }

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Run")
	float GetRunSeconds() const;

	/** RoR2-style: creeps up with time, jumps with each zone. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Run")
	float GetDifficultyCoefficient() const;

	/** Seeded xoshiro256++ stream for the current zone; identical for re-entries. */
	FRLXoshiro256 MakeZoneRng() const;

	/** Run-scoped stream for combat rolls (crits); reseeded each embark. */
	FRLXoshiro256& GetCombatRng() { return CombatRng; }

	/** A crate home is offered after every third cleared zone. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Run")
	bool ShouldOfferBankingCrate() const;

	// --- Excess mana (roguelike currency, reset at camp) ---

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Mana")
	void AddExcessMana(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Mana")
	bool SpendExcessMana(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Mana")
	int32 GetExcessMana() const { return ExcessMana; }

	// --- Run inventory & banking ---

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Inventory")
	void AddRunResource(FName ItemId, int32 Count);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Inventory")
	const TArray<FRLItemStack>& GetRunInventory() const { return RunInventory; }

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Inventory")
	int32 GetRunInventoryTotal() const;

	/** Ships the whole run inventory home; it is now safe even if you die. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Inventory")
	void BankRunInventory();

	// --- Boons (temporary by design) ---

	/** Records a purchased boon stack; RunPowerComponent applies the stats. */
	void RecordBoonStack(FName BoonId);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Boons")
	int32 GetBoonStacks(FName BoonId) const { return BoonStacks.FindRef(BoonId); }

	const TMap<FName, int32>& GetAllBoonStacks() const { return BoonStacks; }

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Run")
	FRLRunStateChangedSignature OnRunStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Run")
	FRLExcessManaChangedSignature OnExcessManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Run")
	FRLRunInventoryChangedSignature OnRunInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Run")
	FRLRunEndedSignature OnRunEnded;

protected:
	/** Map opened when returning to the fallen god's corpse. */
	UPROPERTY(Config)
	FString BaseCampMapName = TEXT("L_Lobby");

	ERLRunState RunState = ERLRunState::AtBaseCamp;
	int32 CurrentZoneIndex = 0;
	int32 ZonesCleared = 0;
	int32 RunSeed = 0;
	int32 ExcessMana = 0;
	FRLXoshiro256 CombatRng;
	double RunStartPlatformSeconds = 0.0;
	int64 ExperienceThisRun = 0;

	TArray<FRLItemStack> RunInventory;
	TMap<FName, int32> BoonStacks;

	void TravelToZone(int32 ZoneIndex);
	void FinishRun(bool bDied);
	void ResetRunState();
};
