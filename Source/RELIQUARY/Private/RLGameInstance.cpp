#include "RLGameInstance.h"
#include "RLSaveGame.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogRLHero, Log, All);

void URLGameInstance::Init()
{
	Super::Init();
	LoadFromDisk();
}

URLDataSubsystem* URLGameInstance::GetData() const
{
	return GetSubsystem<URLDataSubsystem>();
}

// --- Roster ---

int32 URLGameInstance::CreateHero(const FString& HeroName, ERLHeroClass HeroClass, FName SpecId)
{
	FRLHeroData Hero;
	Hero.HeroName = HeroName.IsEmpty() ? TEXT("Wanderer") : HeroName;
	Hero.HeroClass = HeroClass;
	Hero.SpecId = SpecId;

	// Every hero starts with a humble crafted weapon and a handful of timber.
	const TCHAR* StarterWeapon =
		HeroClass == ERLHeroClass::Warrior ? TEXT("OakheartSword") :
		HeroClass == ERLHeroClass::Rogue ? TEXT("OakheartShivs") : TEXT("OakheartStaff");
	Hero.Equipped.Add(ERLEquipSlot::Weapon, FName(StarterWeapon));
	Hero.Stash.Add({ FName(TEXT("OakheartTimber")), 5 });

	const int32 NewIndex = Heroes.Add(MoveTemp(Hero));
	ActiveHeroIndex = NewIndex;
	OnActiveHeroChanged.Broadcast();
	SaveToDisk();
	return NewIndex;
}

bool URLGameInstance::SelectHero(int32 HeroIndex)
{
	if (!Heroes.IsValidIndex(HeroIndex))
	{
		return false;
	}
	ActiveHeroIndex = HeroIndex;
	OnActiveHeroChanged.Broadcast();
	return true;
}

FRLHeroData URLGameInstance::GetActiveHeroData() const
{
	const FRLHeroData* Hero = GetActiveHero();
	return Hero ? *Hero : FRLHeroData();
}

FRLHeroData* URLGameInstance::GetActiveHero()
{
	return Heroes.IsValidIndex(ActiveHeroIndex) ? &Heroes[ActiveHeroIndex] : nullptr;
}

const FRLHeroData* URLGameInstance::GetActiveHero() const
{
	return Heroes.IsValidIndex(ActiveHeroIndex) ? &Heroes[ActiveHeroIndex] : nullptr;
}

// --- Leveling ---

int32 URLGameInstance::AddExperience(int64 Amount)
{
	FRLHeroData* Hero = GetActiveHero();
	if (!Hero || Amount <= 0 || Hero->Level >= RLBalance::MaxHeroLevel)
	{
		return 0;
	}

	Hero->Experience += Amount;

	int32 LevelsGained = 0;
	while (Hero->Level < RLBalance::MaxHeroLevel &&
		Hero->Experience >= RLBalance::XPForNextLevel(Hero->Level))
	{
		Hero->Experience -= RLBalance::XPForNextLevel(Hero->Level);
		++Hero->Level;
		++LevelsGained;
	}

	if (LevelsGained > 0)
	{
		OnLeveledUp.Broadcast(Hero->Level, Hero->Level >= RLBalance::MaxHeroLevel);
	}
	return LevelsGained;
}

int64 URLGameInstance::GetXPForNextLevel() const
{
	const FRLHeroData* Hero = GetActiveHero();
	if (!Hero || Hero->Level >= RLBalance::MaxHeroLevel)
	{
		return 0;
	}
	return RLBalance::XPForNextLevel(Hero->Level);
}

int32 URLGameInstance::GetAvailableTalentPoints() const
{
	const FRLHeroData* Hero = GetActiveHero();
	if (!Hero)
	{
		return 0;
	}

	int32 Spent = 0;
	for (const FRLTalentRank& Rank : Hero->Talents)
	{
		Spent += Rank.Ranks;
	}
	return (Hero->Level - 1) * RLBalance::TalentPointsPerLevel - Spent;
}

bool URLGameInstance::SpendTalentPoint(FName TalentId)
{
	FRLHeroData* Hero = GetActiveHero();
	URLDataSubsystem* Data = GetData();
	if (!Hero || !Data || GetAvailableTalentPoints() <= 0)
	{
		return false;
	}

	const FRLTalentRow* Talent = Data->GetTalentTable()
		? Data->GetTalentTable()->FindRow<FRLTalentRow>(TalentId, TEXT("Talents"), false) : nullptr;
	if (!Talent)
	{
		return false;
	}

	// Graph gating: a node unlocks once any prerequisite holds a rank.
	if (Talent->Prerequisites.Num() > 0)
	{
		const bool bUnlocked = Talent->Prerequisites.ContainsByPredicate(
			[&](const FName& Prereq)
			{
				const FRLTalentRank* Owned = Hero->Talents.FindByPredicate(
					[&](const FRLTalentRank& R) { return R.TalentId == Prereq; });
				return Owned && Owned->Ranks > 0;
			});
		if (!bUnlocked)
		{
			return false;
		}
	}

	FRLTalentRank* Existing = Hero->Talents.FindByPredicate(
		[&](const FRLTalentRank& R) { return R.TalentId == TalentId; });
	if (Existing)
	{
		if (Existing->Ranks >= Talent->MaxRanks)
		{
			return false;
		}
		++Existing->Ranks;
	}
	else
	{
		Hero->Talents.Add({ TalentId, 1 });
	}
	return true;
}

void URLGameInstance::ResetTalents()
{
	if (FRLHeroData* Hero = GetActiveHero())
	{
		Hero->Talents.Reset();
	}
}

FText URLGameInstance::GetDisplayedClassName() const
{
	const FRLHeroData* Hero = GetActiveHero();
	URLDataSubsystem* Data = GetData();
	if (!Hero || !Data)
	{
		return FText::GetEmpty();
	}

	if (Hero->Level >= RLBalance::MaxHeroLevel)
	{
		// Evolved identity: the tree holding the most points names you.
		TMap<FName, int32> PointsPerTree;
		for (const FRLTalentRank& Rank : Hero->Talents)
		{
			if (const FRLTalentRow* Talent = Data->GetTalentTable()
				? Data->GetTalentTable()->FindRow<FRLTalentRow>(Rank.TalentId, TEXT("Talents"), false) : nullptr)
			{
				PointsPerTree.FindOrAdd(Talent->TreeId) += Rank.Ranks;
			}
		}

		FName DominantTree = Hero->SpecId;
		int32 BestPoints = -1;
		for (const auto& Pair : PointsPerTree)
		{
			if (Pair.Value > BestPoints)
			{
				BestPoints = Pair.Value;
				DominantTree = Pair.Key;
			}
		}

		if (const FRLSpecRow* Spec = Data->FindSpec(DominantTree))
		{
			return Spec->EvolvedTitle;
		}
	}

	if (const FRLClassRow* ClassRow = Data->FindClass(Hero->HeroClass))
	{
		return ClassRow->DisplayName;
	}
	return FText::GetEmpty();
}

// --- Essences ---

int32 URLGameInstance::GetUnlockedEssenceSlots() const
{
	const FRLHeroData* Hero = GetActiveHero();
	if (!Hero)
	{
		return 0;
	}

	int32 Slots = Hero->Level >= RLBalance::MajorEssenceUnlockLevel ? 1 : 0;
	for (int32 UnlockLevel : RLBalance::MinorEssenceUnlockLevels)
	{
		if (Hero->Level >= UnlockLevel)
		{
			++Slots;
		}
	}
	return Slots;
}

bool URLGameInstance::SocketEssence(FName EssenceId, int32 SlotIndex)
{
	FRLHeroData* Hero = GetActiveHero();
	URLDataSubsystem* Data = GetData();
	if (!Hero || !Data || !Data->FindEssence(EssenceId))
	{
		return false;
	}
	if (SlotIndex < 0 || SlotIndex >= GetUnlockedEssenceSlots())
	{
		return false;
	}

	// One essence per slot; an essence can only be socketed once.
	Hero->Essences.RemoveAll([&](const FRLSocketedEssence& E)
	{
		return E.SlotIndex == SlotIndex || E.EssenceId == EssenceId;
	});

	FRLSocketedEssence Socketed;
	Socketed.EssenceId = EssenceId;
	Socketed.SlotIndex = SlotIndex;
	Socketed.Rank = 1;
	Hero->Essences.Add(Socketed);
	return true;
}

bool URLGameInstance::UpgradeEssence(FName EssenceId)
{
	FRLHeroData* Hero = GetActiveHero();
	URLDataSubsystem* Data = GetData();
	if (!Hero || !Data)
	{
		return false;
	}

	const FRLEssenceRow* Essence = Data->FindEssence(EssenceId);
	FRLSocketedEssence* Socketed = Hero->Essences.FindByPredicate(
		[&](const FRLSocketedEssence& E) { return E.EssenceId == EssenceId; });
	if (!Essence || !Socketed || Socketed->Rank >= 4)
	{
		return false;
	}

	const int32 Cost = Essence->UpgradeCostPerRank * (Socketed->Rank + 1);
	if (!RemoveFromStash(Essence->UpgradeMaterialId, Cost))
	{
		return false;
	}
	++Socketed->Rank;
	return true;
}

// --- Stash ---

void URLGameInstance::AddToStash(const TArray<FRLItemStack>& Stacks)
{
	FRLHeroData* Hero = GetActiveHero();
	if (!Hero)
	{
		return;
	}

	for (const FRLItemStack& Stack : Stacks)
	{
		if (!Stack.IsValid())
		{
			continue;
		}
		FRLItemStack* Existing = Hero->Stash.FindByPredicate(
			[&](const FRLItemStack& S) { return S.ItemId == Stack.ItemId; });
		if (Existing)
		{
			Existing->Count += Stack.Count;
		}
		else
		{
			Hero->Stash.Add(Stack);
		}
	}
}

bool URLGameInstance::RemoveFromStash(FName ItemId, int32 Count)
{
	FRLHeroData* Hero = GetActiveHero();
	if (!Hero || Count <= 0)
	{
		return false;
	}

	FRLItemStack* Existing = Hero->Stash.FindByPredicate(
		[&](const FRLItemStack& S) { return S.ItemId == ItemId; });
	if (!Existing || Existing->Count < Count)
	{
		return false;
	}

	Existing->Count -= Count;
	if (Existing->Count <= 0)
	{
		Hero->Stash.RemoveAll([&](const FRLItemStack& S) { return S.ItemId == ItemId; });
	}
	return true;
}

int32 URLGameInstance::CountInStash(FName ItemId) const
{
	const FRLHeroData* Hero = GetActiveHero();
	if (!Hero)
	{
		return 0;
	}
	const FRLItemStack* Existing = Hero->Stash.FindByPredicate(
		[&](const FRLItemStack& S) { return S.ItemId == ItemId; });
	return Existing ? Existing->Count : 0;
}

bool URLGameInstance::EquipFromStash(FName ItemId)
{
	FRLHeroData* Hero = GetActiveHero();
	URLDataSubsystem* Data = GetData();
	if (!Hero || !Data)
	{
		return false;
	}

	const FRLItemRow* Item = Data->FindItem(ItemId);
	if (!Item || Item->EquipSlot == ERLEquipSlot::None || CountInStash(ItemId) <= 0)
	{
		return false;
	}

	Unequip(Item->EquipSlot);
	RemoveFromStash(ItemId, 1);
	Hero->Equipped.Add(Item->EquipSlot, ItemId);
	return true;
}

bool URLGameInstance::Unequip(ERLEquipSlot Slot)
{
	FRLHeroData* Hero = GetActiveHero();
	if (!Hero)
	{
		return false;
	}

	if (const FName* Equipped = Hero->Equipped.Find(Slot))
	{
		TArray<FRLItemStack> Back;
		Back.Add({ *Equipped, 1 });
		Hero->Equipped.Remove(Slot);
		AddToStash(Back);
		return true;
	}
	return false;
}

// --- Persistence ---

void URLGameInstance::SaveToDisk()
{
	URLSaveGame* Save = Cast<URLSaveGame>(UGameplayStatics::CreateSaveGameObject(URLSaveGame::StaticClass()));
	Save->Heroes = Heroes;
	Save->ActiveHeroIndex = ActiveHeroIndex;

	if (!UGameplayStatics::SaveGameToSlot(Save, URLSaveGame::SlotName, URLSaveGame::UserIndex))
	{
		UE_LOG(LogRLHero, Error, TEXT("Failed to save to slot %s"), *URLSaveGame::SlotName);
	}
}

void URLGameInstance::LoadFromDisk()
{
	if (URLSaveGame* Save = Cast<URLSaveGame>(
		UGameplayStatics::LoadGameFromSlot(URLSaveGame::SlotName, URLSaveGame::UserIndex)))
	{
		Heroes = Save->Heroes;
		ActiveHeroIndex = Save->ActiveHeroIndex;
		OnActiveHeroChanged.Broadcast();
	}
}
