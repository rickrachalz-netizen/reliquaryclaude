#include "RLProgressionComponent.h"
#include "RLGameInstance.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "RLStatLibrary.h"
#include "RLGameplayAbility.h"
#include "RLGameplayTags.h"
#include "AbilitySystemComponent.h"

namespace
{
	FGameplayTag ClassTagFor(ERLHeroClass HeroClass)
	{
		switch (HeroClass)
		{
		case ERLHeroClass::Warrior: return RLTags::Class_Warrior;
		case ERLHeroClass::Rogue:   return RLTags::Class_Rogue;
		case ERLHeroClass::Mage:    return RLTags::Class_Mage;
		default:                    return FGameplayTag();
		}
	}
}

void URLProgressionComponent::RebuildHero(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	ClearAll(ASC);

	URLGameInstance* GI = Cast<URLGameInstance>(GetWorld()->GetGameInstance());
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;
	const FRLHeroData* Hero = GI ? GI->GetActiveHero() : nullptr;
	if (!Hero || !Data)
	{
		return;
	}

	// Class identity tag: the Classic-WoW damage execution reads this to
	// pick the attack power ratio (Warrior Str x2, Rogue Str + Agi).
	const FGameplayTag ClassTag = ClassTagFor(Hero->HeroClass);
	if (ClassTag.IsValid() && !ASC->HasMatchingGameplayTag(ClassTag))
	{
		ASC->AddLooseGameplayTag(ClassTag);
	}

	const FRLClassRow* ClassRow = Data->FindClass(Hero->HeroClass);
	if (ClassRow)
	{
		// Base identity: level-1 stats plus growth for every level gained.
		FRLStatBlock BaseBlock = ClassRow->BaseStats;
		const FRLStatBlock Growth = ClassRow->StatsPerLevel * static_cast<float>(Hero->Level - 1);
		BaseBlock.Health += Growth.Health;           BaseBlock.Mana += Growth.Mana;
		BaseBlock.HealthRegen += Growth.HealthRegen; BaseBlock.ManaRegen += Growth.ManaRegen;
		BaseBlock.Strength += Growth.Strength;       BaseBlock.Agility += Growth.Agility;
		BaseBlock.Intellect += Growth.Intellect;     BaseBlock.CritChance += Growth.CritChance;
		BaseBlock.CritDamage += Growth.CritDamage;   BaseBlock.Haste += Growth.Haste;
		BaseBlock.Armor += Growth.Armor;             BaseBlock.MoveSpeed += Growth.MoveSpeed;
		BaseBlock.Adaptability += Growth.Adaptability;
		URLStatLibrary::SetBaseStats(ASC, BaseBlock);

		// The class kit: primary through special.
		GrantAbility(ASC, ClassRow->PrimaryAbility, Hero->Level);
		GrantAbility(ASC, ClassRow->SecondaryAbility, Hero->Level);
		GrantAbility(ASC, ClassRow->UtilityAbility, Hero->Level);
		GrantAbility(ASC, ClassRow->SpecialAbility, Hero->Level);
	}

	// Talents: stats per rank, keystone abilities at rank 1+.
	for (const FRLTalentRank& Rank : Hero->Talents)
	{
		const FRLTalentRow* Talent = Data->GetTalentTable()
			? Data->GetTalentTable()->FindRow<FRLTalentRow>(Rank.TalentId, TEXT("Progression"), false) : nullptr;
		if (!Talent || Rank.Ranks <= 0)
		{
			continue;
		}

		FActiveGameplayEffectHandle Handle = URLStatLibrary::ApplyStatBlock(
			ASC, Talent->StatsPerRank * static_cast<float>(Rank.Ranks), Rank.TalentId.ToString());
		if (Handle.IsValid())
		{
			AppliedEffects.Add(Handle);
		}
		GrantAbility(ASC, Talent->GrantedAbility, Hero->Level);
	}

	// Reliquary Shard essences: passives from any slot, active from slot 0.
	for (const FRLSocketedEssence& Socketed : Hero->Essences)
	{
		const FRLEssenceRow* Essence = Data->FindEssence(Socketed.EssenceId);
		if (!Essence)
		{
			continue;
		}

		FActiveGameplayEffectHandle Handle = URLStatLibrary::ApplyStatBlock(
			ASC, Essence->StatsPerRank * static_cast<float>(Socketed.Rank), Socketed.EssenceId.ToString());
		if (Handle.IsValid())
		{
			AppliedEffects.Add(Handle);
		}

		if (Socketed.SlotIndex == 0)
		{
			GrantAbility(ASC, Essence->MajorAbility, Socketed.Rank);
		}
	}

	URLStatLibrary::FillVitals(ASC);
}

void URLProgressionComponent::ClearAll(UAbilitySystemComponent* ASC)
{
	if (ASC)
	{
		for (const FActiveGameplayEffectHandle& Handle : AppliedEffects)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
		for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilities)
		{
			ASC->ClearAbility(Handle);
		}
	}
	AppliedEffects.Reset();
	GrantedAbilities.Reset();
}

void URLProgressionComponent::GrantAbility(UAbilitySystemComponent* ASC,
	const TSoftClassPtr<URLGameplayAbility>& AbilityClass, int32 Level)
{
	if (AbilityClass.IsNull())
	{
		return;
	}

	// Content abilities are BP assets that may not exist yet; skip quietly.
	UClass* Loaded = AbilityClass.LoadSynchronous();
	if (!Loaded)
	{
		return;
	}

	FGameplayAbilitySpec Spec(Loaded, Level);
	GrantedAbilities.Add(ASC->GiveAbility(Spec));
}
