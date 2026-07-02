#include "RLDamageExecution.h"
#include "RLAttributeSet.h"
#include "RLAbilitySystemComponent.h"
#include "RLGameplayTags.h"
#include "RLCombatFormulas.h"

namespace RLDamageStatics
{
	struct FDamageCaptures
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(Strength);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Agility);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Intellect);
		DECLARE_ATTRIBUTE_CAPTUREDEF(CritChance);
		DECLARE_ATTRIBUTE_CAPTUREDEF(CritDamage);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Adaptability);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
		DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);

		FDamageCaptures()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, Strength, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, Agility, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, Intellect, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, CritChance, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, CritDamage, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, Adaptability, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, Armor, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, IncomingDamage, Target, false);
		}
	};

	static const FDamageCaptures& Get()
	{
		static FDamageCaptures Captures;
		return Captures;
	}
}

URLDamageExecution::URLDamageExecution()
{
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().StrengthDef);
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().AgilityDef);
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().IntellectDef);
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().CritChanceDef);
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().CritDamageDef);
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().AdaptabilityDef);
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().ArmorDef);
}

void URLDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// Invulnerable targets shrug everything off.
	if (EvalParams.TargetTags && EvalParams.TargetTags->HasTag(RLTags::State_Invulnerable))
	{
		return;
	}

	const float BaseDamage = Spec.GetSetByCallerMagnitude(RLTags::SetByCaller_Damage, /*WarnIfNotFound=*/false, 0.f);
	if (BaseDamage <= 0.f)
	{
		return;
	}

	auto Capture = [&](const FGameplayEffectAttributeCaptureDefinition& Def)
	{
		float Value = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Def, EvalParams, Value);
		return Value;
	};

	const float Strength = Capture(RLDamageStatics::Get().StrengthDef);
	const float Agility = Capture(RLDamageStatics::Get().AgilityDef);
	const float Intellect = Capture(RLDamageStatics::Get().IntellectDef);

	// School selector: WeaponSpeed -> Classic swing, CastTime -> Classic spell.
	const float WeaponSpeed = Spec.GetSetByCallerMagnitude(RLTags::SetByCaller_WeaponSpeed, false, 0.f);
	const float CastTime = Spec.GetSetByCallerMagnitude(RLTags::SetByCaller_CastTime, false, 0.f);
	const bool bSpell = CastTime > 0.f;

	float Damage;
	float CritChance = Capture(RLDamageStatics::Get().CritChanceDef);
	float CritMultiplier;

	if (bSpell)
	{
		const float SpellPower = RLCombat::SpellPowerFromIntellect(Intellect);
		Damage = RLCombat::SpellHitDamage(BaseDamage, SpellPower, CastTime);
		CritChance += RLCombat::SpellCritFromIntellect(Intellect);

		// Classic asymmetry: gear crit-damage scales off the 1.5x spell base.
		const float GearCritDamage = FMath::Max(Capture(RLDamageStatics::Get().CritDamageDef), 1.f);
		CritMultiplier = RLCombat::SpellCritMultiplier * (GearCritDamage / RLCombat::MeleeCritMultiplier);
		CritMultiplier = FMath::Max(CritMultiplier, 1.f);
	}
	else
	{
		// Attack power by class, Classic ratios.
		float AttackPower;
		if (EvalParams.SourceTags && EvalParams.SourceTags->HasTag(RLTags::Class_Rogue))
		{
			AttackPower = RLCombat::RogueAttackPower(Strength, Agility);
		}
		else if (EvalParams.SourceTags && EvalParams.SourceTags->HasTag(RLTags::Class_Warrior))
		{
			AttackPower = RLCombat::WarriorAttackPower(Strength);
		}
		else
		{
			AttackPower = RLCombat::DefaultAttackPower(Strength);
		}

		const float Speed = WeaponSpeed > 0.f ? WeaponSpeed : 2.f;
		Damage = RLCombat::MeleeSwingDamage(BaseDamage, AttackPower, Speed);
		CritChance += RLCombat::MeleeCritFromAgility(Agility);
		CritMultiplier = FMath::Max(Capture(RLDamageStatics::Get().CritDamageDef), 1.f);
	}

	// Adaptability: reward varied play, up to five stacks (RELIQUARY's own).
	const float AdaptPerStack = Capture(RLDamageStatics::Get().AdaptabilityDef);
	if (AdaptPerStack > 0.f)
	{
		int32 Stacks = 0;
		if (const URLAbilitySystemComponent* SourceASC =
			Cast<URLAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent()))
		{
			Stacks = SourceASC->GetAdaptabilityStacks();
		}
		Damage *= 1.f + AdaptPerStack * static_cast<float>(Stacks);
	}

	// Crit roll.
	if (CritChance > 0.f && FMath::FRand() < CritChance)
	{
		Damage *= CritMultiplier;
	}

	// Armor mitigation — physical only; spells ignore armor (Classic).
	if (!bSpell)
	{
		const float Armor = Capture(RLDamageStatics::Get().ArmorDef);
		const float AttackerLevel = FMath::Max(Spec.GetLevel(), 1.f);
		Damage *= 1.f - RLCombat::ArmorDamageReduction(Armor, AttackerLevel);
	}

	if (Damage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			RLDamageStatics::Get().IncomingDamageProperty, EGameplayModOp::Additive, Damage));
	}
}
