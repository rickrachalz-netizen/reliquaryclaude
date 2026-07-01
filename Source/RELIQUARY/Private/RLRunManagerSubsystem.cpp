#include "RLRunManagerSubsystem.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "RLGameInstance.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogRLRun, Log, All);

// --- Lifecycle ---

void URLRunManagerSubsystem::EmbarkOnRun()
{
	if (IsOnRun())
	{
		UE_LOG(LogRLRun, Warning, TEXT("EmbarkOnRun called mid-run; ignoring."));
		return;
	}

	ResetRunState();

	// Seed rolled and locked at embark — restarting cannot reroll the realm.
	RunSeed = FMath::RandRange(1, MAX_int32 - 1);
	RunStartPlatformSeconds = FPlatformTime::Seconds();
	CurrentZoneIndex = 1;
	SetRunState(ERLRunState::InZone);

	UE_LOG(LogRLRun, Log, TEXT("Embarking. Seed=%d"), RunSeed);
	TravelToZone(CurrentZoneIndex);
}

void URLRunManagerSubsystem::EmbarkInPlace()
{
	if (IsOnRun())
	{
		return;
	}

	ResetRunState();
	RunSeed = FMath::RandRange(1, MAX_int32 - 1);
	RunStartPlatformSeconds = FPlatformTime::Seconds();
	CurrentZoneIndex = 1;
	SetRunState(ERLRunState::InZone);
	UE_LOG(LogRLRun, Log, TEXT("Embarking in place. Seed=%d"), RunSeed);
}

void URLRunManagerSubsystem::AdvanceToNextZone()
{
	if (RunState != ERLRunState::AltarCharged)
	{
		UE_LOG(LogRLRun, Warning, TEXT("AdvanceToNextZone outside AltarCharged state; ignoring."));
		return;
	}

	++ZonesCleared;

	if (CurrentZoneIndex >= RLBalance::ZonesPerRun)
	{
		// Nothing deeper: the tenth altar always sends you home victorious.
		ExtractToBaseCamp();
		return;
	}

	++CurrentZoneIndex;
	SetRunState(ERLRunState::InZone);
	TravelToZone(CurrentZoneIndex);
}

void URLRunManagerSubsystem::ExtractToBaseCamp()
{
	if (RunState != ERLRunState::AltarCharged && RunState != ERLRunState::Extracting)
	{
		UE_LOG(LogRLRun, Warning, TEXT("Extraction requested outside a charged altar; ignoring."));
		return;
	}

	++ZonesCleared;
	SetRunState(ERLRunState::Extracting);
	FinishRun(/*bDied=*/false);
}

void URLRunManagerSubsystem::HandleHeroDeath()
{
	if (!IsOnRun())
	{
		return;
	}

	SetRunState(ERLRunState::Dead);
	FinishRun(/*bDied=*/true);
}

void URLRunManagerSubsystem::HandleFinalBossKilled(bool bWildGod)
{
	if (bWildGod)
	{
		if (URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance()))
		{
			if (FRLHeroData* Hero = GI->GetActiveHero())
			{
				Hero->bDefeatedWildGod = true;
			}
		}
		UE_LOG(LogRLRun, Log, TEXT("The Wild God has fallen. The way home is open."));
	}
}

void URLRunManagerSubsystem::SetRunState(ERLRunState NewState)
{
	if (RunState == NewState)
	{
		return;
	}
	RunState = NewState;
	OnRunStateChanged.Broadcast(NewState);
}

// --- Queries ---

float URLRunManagerSubsystem::GetRunSeconds() const
{
	if (!IsOnRun() || RunStartPlatformSeconds <= 0.0)
	{
		return 0.f;
	}
	return static_cast<float>(FPlatformTime::Seconds() - RunStartPlatformSeconds);
}

float URLRunManagerSubsystem::GetDifficultyCoefficient() const
{
	return RLBalance::DifficultyCoefficient(GetRunSeconds(), CurrentZoneIndex);
}

FRandomStream URLRunManagerSubsystem::MakeZoneRandomStream() const
{
	// Deterministic per (seed, zone): re-entering PIE cannot fish new layouts.
	return FRandomStream(RunSeed ^ (CurrentZoneIndex * 7919));
}

bool URLRunManagerSubsystem::ShouldOfferBankingCrate() const
{
	return ZonesCleared > 0 && (ZonesCleared % RLBalance::ZonesPerBankingCrate) == 0;
}

// --- Excess mana ---

void URLRunManagerSubsystem::AddExcessMana(int32 Amount)
{
	if (Amount <= 0 || !IsOnRun())
	{
		return;
	}
	ExcessMana += Amount;
	OnExcessManaChanged.Broadcast(ExcessMana);
}

bool URLRunManagerSubsystem::SpendExcessMana(int32 Amount)
{
	if (Amount < 0 || ExcessMana < Amount)
	{
		return false;
	}
	ExcessMana -= Amount;
	OnExcessManaChanged.Broadcast(ExcessMana);
	return true;
}

// --- Run inventory & banking ---

void URLRunManagerSubsystem::AddRunResource(FName ItemId, int32 Count)
{
	if (ItemId == NAME_None || Count <= 0)
	{
		return;
	}

	FRLItemStack* Existing = RunInventory.FindByPredicate(
		[&](const FRLItemStack& S) { return S.ItemId == ItemId; });
	if (Existing)
	{
		Existing->Count += Count;
	}
	else
	{
		RunInventory.Add({ ItemId, Count });
	}
	OnRunInventoryChanged.Broadcast();
}

int32 URLRunManagerSubsystem::GetRunInventoryTotal() const
{
	int32 Total = 0;
	for (const FRLItemStack& Stack : RunInventory)
	{
		Total += Stack.Count;
	}
	return Total;
}

void URLRunManagerSubsystem::BankRunInventory()
{
	if (RunInventory.Num() == 0)
	{
		return;
	}

	if (URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance()))
	{
		GI->AddToStash(RunInventory);
		UE_LOG(LogRLRun, Log, TEXT("Banked %d resources home."), GetRunInventoryTotal());
		RunInventory.Reset();
		OnRunInventoryChanged.Broadcast();
	}
}

// --- Boons ---

void URLRunManagerSubsystem::RecordBoonStack(FName BoonId)
{
	++BoonStacks.FindOrAdd(BoonId);
}

// --- Internals ---

void URLRunManagerSubsystem::TravelToZone(int32 ZoneIndex)
{
	URLDataSubsystem* Data = GetGameInstance()->GetSubsystem<URLDataSubsystem>();
	const FRLZoneRow* Zone = Data ? Data->FindZone(ZoneIndex) : nullptr;

	if (Zone && !Zone->Map.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(GetGameInstance(), Zone->Map);
	}
	else
	{
		UE_LOG(LogRLRun, Warning, TEXT("Zone %d has no map configured; falling back to L_Run."), ZoneIndex);
		UGameplayStatics::OpenLevel(GetGameInstance(), FName(TEXT("L_Run")));
	}
}

void URLRunManagerSubsystem::FinishRun(bool bDied)
{
	FRLRunSummary Summary;
	Summary.bDied = bDied;
	Summary.ZonesCleared = ZonesCleared;
	Summary.RunSeconds = GetRunSeconds();
	Summary.ExperienceEarned = ExperienceThisRun;

	URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance());

	if (bDied)
	{
		// Death forfeits every resource still carried. Banked crates are safe.
		UE_LOG(LogRLRun, Log, TEXT("Died on the run. %d resources lost."), GetRunInventoryTotal());
		RunInventory.Reset();
	}
	else
	{
		Summary.ExtractedResources = RunInventory;
		if (GI)
		{
			GI->AddToStash(RunInventory);
			if (FRLHeroData* Hero = GI->GetActiveHero())
			{
				++Hero->RunsCompleted;
				Hero->DeepestZoneReached = FMath::Max(Hero->DeepestZoneReached, CurrentZoneIndex);
			}
		}
		RunInventory.Reset();
	}

	OnRunEnded.Broadcast(Summary);
	ResetRunState();
	SetRunState(bDied ? ERLRunState::Dead : ERLRunState::AtBaseCamp);

	// Home to the fallen god. ARLBaseCampGameMode autosaves on arrival and
	// flips Dead back to AtBaseCamp.
	UGameplayStatics::OpenLevel(GetGameInstance(), FName(*BaseCampMapName));
}

void URLRunManagerSubsystem::ResetRunState()
{
	// Temporary by design: every scrap of roguelike power evaporates here.
	CurrentZoneIndex = 0;
	ZonesCleared = 0;
	ExcessMana = 0;
	ExperienceThisRun = 0;
	RunInventory.Reset();
	BoonStacks.Reset();
	OnExcessManaChanged.Broadcast(0);
	OnRunInventoryChanged.Broadcast();
}
