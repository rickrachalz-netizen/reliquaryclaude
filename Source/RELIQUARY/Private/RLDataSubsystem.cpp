#include "RLDataSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogRLData, Log, All);

UDataTable* URLDataSubsystem::LoadTable(const FSoftObjectPath& Path)
{
	UDataTable* Table = Cast<UDataTable>(Path.TryLoad());
	if (!Table)
	{
		UE_LOG(LogRLData, Warning, TEXT("DataTable missing: %s (import its CSV from /Data — see docs/EDITOR_SETUP.md)"), *Path.ToString());
	}
	return Table;
}

void URLDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ItemTable = LoadTable(ItemTablePath);
	SetBonusTable = LoadTable(SetBonusTablePath);
	RecipeTable = LoadTable(RecipeTablePath);
	BoonTable = LoadTable(BoonTablePath);
	ZoneTable = LoadTable(ZoneTablePath);
	SpawnCardTable = LoadTable(SpawnCardTablePath);
	ClassTable = LoadTable(ClassTablePath);
	SpecTable = LoadTable(SpecTablePath);
	TalentTable = LoadTable(TalentTablePath);
	EssenceTable = LoadTable(EssenceTablePath);
}

namespace
{
	template <typename RowType>
	const RowType* FindRowChecked(const UDataTable* Table, FName RowName)
	{
		return Table ? Table->FindRow<RowType>(RowName, TEXT("RLData"), /*bWarnIfRowMissing=*/false) : nullptr;
	}
}

const FRLItemRow* URLDataSubsystem::FindItem(FName ItemId) const { return FindRowChecked<FRLItemRow>(ItemTable, ItemId); }
const FRLRecipeRow* URLDataSubsystem::FindRecipe(FName RecipeId) const { return FindRowChecked<FRLRecipeRow>(RecipeTable, RecipeId); }
const FRLBoonRow* URLDataSubsystem::FindBoon(FName BoonId) const { return FindRowChecked<FRLBoonRow>(BoonTable, BoonId); }
const FRLSpawnCardRow* URLDataSubsystem::FindSpawnCard(FName CardId) const { return FindRowChecked<FRLSpawnCardRow>(SpawnCardTable, CardId); }
const FRLSpecRow* URLDataSubsystem::FindSpec(FName SpecId) const { return FindRowChecked<FRLSpecRow>(SpecTable, SpecId); }
const FRLEssenceRow* URLDataSubsystem::FindEssence(FName EssenceId) const { return FindRowChecked<FRLEssenceRow>(EssenceTable, EssenceId); }

const FRLEssenceRow* URLDataSubsystem::FindEssenceForEnemy(FName EnemyTypeId, FName& OutEssenceId) const
{
	OutEssenceId = NAME_None;
	if (!EssenceTable || EnemyTypeId == NAME_None)
	{
		return nullptr;
	}
	const FRLEssenceRow* Found = nullptr;
	EssenceTable->ForeachRow<FRLEssenceRow>(TEXT("RLData"), [&](const FName& RowName, const FRLEssenceRow& Row)
	{
		if (Row.SourceEnemyId == EnemyTypeId)
		{
			Found = &Row;
			OutEssenceId = RowName;
		}
	});
	return Found;
}

const FRLZoneRow* URLDataSubsystem::FindZone(int32 ZoneIndex) const
{
	if (!ZoneTable)
	{
		return nullptr;
	}
	const FRLZoneRow* Found = nullptr;
	ZoneTable->ForeachRow<FRLZoneRow>(TEXT("RLData"), [&](const FName& RowName, const FRLZoneRow& Row)
	{
		if (Row.ZoneIndex == ZoneIndex)
		{
			Found = &Row;
		}
	});
	return Found;
}

const FRLClassRow* URLDataSubsystem::FindClass(ERLHeroClass HeroClass) const
{
	if (!ClassTable)
	{
		return nullptr;
	}
	const FRLClassRow* Found = nullptr;
	ClassTable->ForeachRow<FRLClassRow>(TEXT("RLData"), [&](const FName& RowName, const FRLClassRow& Row)
	{
		if (Row.HeroClass == HeroClass)
		{
			Found = &Row;
		}
	});
	return Found;
}

void URLDataSubsystem::GetTalentsForTree(FName TreeId, TArray<TPair<FName, const FRLTalentRow*>>& Out) const
{
	if (!TalentTable)
	{
		return;
	}
	TalentTable->ForeachRow<FRLTalentRow>(TEXT("RLData"), [&](const FName& RowName, const FRLTalentRow& Row)
	{
		if (Row.TreeId == TreeId)
		{
			Out.Emplace(RowName, &Row);
		}
	});
}

void URLDataSubsystem::GetSpecsForClass(ERLHeroClass HeroClass, TArray<TPair<FName, const FRLSpecRow*>>& Out) const
{
	if (!SpecTable)
	{
		return;
	}
	SpecTable->ForeachRow<FRLSpecRow>(TEXT("RLData"), [&](const FName& RowName, const FRLSpecRow& Row)
	{
		if (Row.HeroClass == HeroClass)
		{
			Out.Emplace(RowName, &Row);
		}
	});
}

void URLDataSubsystem::GetAllRecipes(TArray<TPair<FName, const FRLRecipeRow*>>& Out) const
{
	if (!RecipeTable)
	{
		return;
	}
	RecipeTable->ForeachRow<FRLRecipeRow>(TEXT("RLData"), [&](const FName& RowName, const FRLRecipeRow& Row)
	{
		Out.Emplace(RowName, &Row);
	});
}

void URLDataSubsystem::GetSetBonuses(const FGameplayTag& SetTag, TArray<const FRLSetBonusRow*>& Out) const
{
	if (!SetBonusTable || !SetTag.IsValid())
	{
		return;
	}
	SetBonusTable->ForeachRow<FRLSetBonusRow>(TEXT("RLData"), [&](const FName& RowName, const FRLSetBonusRow& Row)
	{
		if (Row.SetTag == SetTag)
		{
			Out.Add(&Row);
		}
	});
	Out.Sort([](const FRLSetBonusRow& A, const FRLSetBonusRow& B) { return A.RequiredPieces < B.RequiredPieces; });
}

void URLDataSubsystem::DrawRandomBoons(int32 Count, const TMap<FName, int32>& CurrentStacks,
	FRLXoshiro256& Rng, TArray<FName>& Out) const
{
	if (!BoonTable)
	{
		return;
	}

	struct FCandidate { FName Id; float Weight; };
	TArray<FCandidate> Pool;
	BoonTable->ForeachRow<FRLBoonRow>(TEXT("RLData"), [&](const FName& RowName, const FRLBoonRow& Row)
	{
		const int32 Stacks = CurrentStacks.FindRef(RowName);
		if (Row.MaxStacks > 0 && Stacks >= Row.MaxStacks)
		{
			return;
		}
		Pool.Add({ RowName, FMath::Max(Row.Weight, 0.f) });
	});

	for (int32 Draw = 0; Draw < Count && Pool.Num() > 0; ++Draw)
	{
		float TotalWeight = 0.f;
		for (const FCandidate& C : Pool) { TotalWeight += C.Weight; }
		if (TotalWeight <= 0.f)
		{
			break;
		}

		float Roll = Rng.FRandRange(0.f, TotalWeight);
		int32 Picked = Pool.Num() - 1;
		for (int32 i = 0; i < Pool.Num(); ++i)
		{
			Roll -= Pool[i].Weight;
			if (Roll <= 0.f)
			{
				Picked = i;
				break;
			}
		}

		Out.Add(Pool[Picked].Id);
		Pool.RemoveAt(Picked);	// each altar choice is distinct
	}
}

FName URLDataSubsystem::DrawSpawnCard(int32 ZoneIndex, float Difficulty, float MaxCost, FRLXoshiro256& Rng) const
{
	if (!SpawnCardTable)
	{
		return NAME_None;
	}

	struct FCandidate { FName Id; float Weight; };
	TArray<FCandidate> Pool;
	SpawnCardTable->ForeachRow<FRLSpawnCardRow>(TEXT("RLData"), [&](const FName& RowName, const FRLSpawnCardRow& Row)
	{
		if (Row.bBoss || Row.MinZone > ZoneIndex || Row.MinDifficulty > Difficulty || Row.Cost > MaxCost)
		{
			return;
		}
		Pool.Add({ RowName, FMath::Max(Row.Weight, 0.f) });
	});

	float TotalWeight = 0.f;
	for (const FCandidate& C : Pool) { TotalWeight += C.Weight; }
	if (TotalWeight <= 0.f)
	{
		return NAME_None;
	}

	float Roll = Rng.FRandRange(0.f, TotalWeight);
	for (const FCandidate& C : Pool)
	{
		Roll -= C.Weight;
		if (Roll <= 0.f)
		{
			return C.Id;
		}
	}
	return Pool.Last().Id;
}
