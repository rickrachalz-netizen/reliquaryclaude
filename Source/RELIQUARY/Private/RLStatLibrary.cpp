#include "RLStatLibrary.h"
#include "RLAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

namespace
{
	struct FStatMapping
	{
		float FRLStatBlock::* Field;
		FGameplayAttribute (*Getter)();
	};

	const TArray<FStatMapping>& GetStatMappings()
	{
		static const TArray<FStatMapping> Mappings = {
			{ &FRLStatBlock::Health,       &URLAttributeSet::GetMaxHealthAttribute },
			{ &FRLStatBlock::Mana,         &URLAttributeSet::GetMaxManaAttribute },
			{ &FRLStatBlock::HealthRegen,  &URLAttributeSet::GetHealthRegenAttribute },
			{ &FRLStatBlock::ManaRegen,    &URLAttributeSet::GetManaRegenAttribute },
			{ &FRLStatBlock::Strength,     &URLAttributeSet::GetStrengthAttribute },
			{ &FRLStatBlock::Agility,      &URLAttributeSet::GetAgilityAttribute },
			{ &FRLStatBlock::Intellect,    &URLAttributeSet::GetIntellectAttribute },
			{ &FRLStatBlock::CritChance,   &URLAttributeSet::GetCritChanceAttribute },
			{ &FRLStatBlock::CritDamage,   &URLAttributeSet::GetCritDamageAttribute },
			{ &FRLStatBlock::Haste,        &URLAttributeSet::GetHasteAttribute },
			{ &FRLStatBlock::Armor,        &URLAttributeSet::GetArmorAttribute },
			{ &FRLStatBlock::MoveSpeed,    &URLAttributeSet::GetMoveSpeedAttribute },
			{ &FRLStatBlock::Adaptability, &URLAttributeSet::GetAdaptabilityAttribute },
		};
		return Mappings;
	}
}

FActiveGameplayEffectHandle URLStatLibrary::ApplyStatBlock(UAbilitySystemComponent* ASC,
	const FRLStatBlock& Block, const FString& DebugName)
{
	if (!ASC || Block.IsEmpty())
	{
		return FActiveGameplayEffectHandle();
	}

	const FName EffectName = MakeUniqueObjectName(GetTransientPackage(), UGameplayEffect::StaticClass(),
		FName(*FString::Printf(TEXT("GE_RL_%s"), *DebugName)));
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), EffectName);
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;

	for (const FStatMapping& Mapping : GetStatMappings())
	{
		const float Value = Block.*(Mapping.Field);
		if (Value == 0.f)
		{
			continue;
		}
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Mapping.Getter();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Value);
		Effect->Modifiers.Add(Modifier);
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	return ASC->ApplyGameplayEffectToSelf(Effect, 1.f, Context);
}

void URLStatLibrary::SetBaseStats(UAbilitySystemComponent* ASC, const FRLStatBlock& Block)
{
	if (!ASC)
	{
		return;
	}

	for (const FStatMapping& Mapping : GetStatMappings())
	{
		ASC->SetNumericAttributeBase(Mapping.Getter(), Block.*(Mapping.Field));
	}
	// Sensible floors so an incomplete stat block still yields a live hero.
	if (Block.CritDamage == 0.f)
	{
		ASC->SetNumericAttributeBase(URLAttributeSet::GetCritDamageAttribute(), 2.f);
	}
	if (Block.MoveSpeed == 0.f)
	{
		ASC->SetNumericAttributeBase(URLAttributeSet::GetMoveSpeedAttribute(), 600.f);
	}
}

void URLStatLibrary::FillVitals(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}
	ASC->SetNumericAttributeBase(URLAttributeSet::GetHealthAttribute(),
		ASC->GetNumericAttribute(URLAttributeSet::GetMaxHealthAttribute()));
	ASC->SetNumericAttributeBase(URLAttributeSet::GetManaAttribute(),
		ASC->GetNumericAttribute(URLAttributeSet::GetMaxManaAttribute()));
}
