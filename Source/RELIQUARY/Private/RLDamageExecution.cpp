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
		DECLARE_ATTRIBUTE_CAPTUREDEF(Multistrike);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Sanguination);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Force);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Synergy);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Frenzy);
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
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, Multistrike, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, Sanguination, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, Force, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, Synergy, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(URLAttributeSet, Frenzy, Source, true);
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
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().MultistrikeDef);
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().SanguinationDef);
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().ForceDef);
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().SynergyDef);
	RelevantAttributesToCapture.Add(RLDamageStatics::Get().FrenzyDef);
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

	// School selector: which coefficient tag the ability filled.
	const float PhysCoefficient = Spec.GetSetByCallerMagnitude(RLTags::SetByCaller_PhysicalCoefficient, false, 0.f);
	const float SpellCoefficient = Spec.GetSetByCallerMagnitude(RLTags::SetByCaller_SpellCoefficient, false, 0.f);
	const bool bSpell = SpellCoefficient > 0.f;

	// Live combat state on the attacker (built by PREVIOUS hits).
	URLAbilitySystemComponent* SourceASC =
		Cast<URLAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());
	const int32 AdaptStacks = SourceASC ? SourceASC->GetAdaptabilityStacks() : 0;
	const int32 SynergyStacks = SourceASC ? SourceASC->GetSynergyStacks() : 0;
	const bool bFrenzied = SourceASC && SourceASC->IsFrenzied();

	// Synergy: crit chains sharpen Multistrike, Hatred (read at cooldown
	// time), and Adaptability until a non-crit spends the stacks.
	const float SynergyBonus = Capture(RLDamageStatics::Get().SynergyDef) * static_cast<float>(SynergyStacks);

	// --- Hit: Base + Coefficient x Power ---
	float Power;
	float CritChance = Capture(RLDamageStatics::Get().CritChanceDef);
	if (bSpell)
	{
		Power = RLCombat::SpellPowerFromIntellect(Intellect);
		CritChance += RLCombat::SpellCritFromIntellect(Intellect);
	}
	else
	{
		if (EvalParams.SourceTags && EvalParams.SourceTags->HasTag(RLTags::Class_Rogue))
		{
			Power = RLCombat::RogueAttackPower(Strength, Agility);
		}
		else if (EvalParams.SourceTags && EvalParams.SourceTags->HasTag(RLTags::Class_Warrior))
		{
			Power = RLCombat::WarriorAttackPower(Strength);
		}
		else
		{
			Power = RLCombat::DefaultAttackPower(Strength);
		}
		CritChance += RLCombat::MeleeCritFromAgility(Agility);
	}

	const float Coefficient = bSpell ? SpellCoefficient : (PhysCoefficient > 0.f ? PhysCoefficient : 1.f);
	float Damage = RLCombat::AbilityHitDamage(BaseDamage, Coefficient, Power);

	// --- Sanguination: blood in, damage out ---
	const float Sanguination = Capture(RLDamageStatics::Get().SanguinationDef);
	if (Sanguination > 0.f && SourceASC)
	{
		Damage *= 1.f + Sanguination;

		const float CurrentHealth = SourceASC->GetNumericAttribute(URLAttributeSet::GetHealthAttribute());
		const float BloodCost = FMath::Min(
			CurrentHealth * Sanguination * RLCombat::SanguinationBloodCostRatio,
			FMath::Max(CurrentHealth - 1.f, 0.f));	// the price is steep but never lethal
		if (BloodCost > 0.f)
		{
			SourceASC->ApplyModToAttribute(URLAttributeSet::GetHealthAttribute(),
				EGameplayModOp::Additive, -BloodCost);
		}
	}

	// --- Adaptability (raised by Synergy while crits chain) ---
	const float AdaptPerStack = Capture(RLDamageStatics::Get().AdaptabilityDef) + SynergyBonus;
	if (AdaptPerStack > 0.f)
	{
		Damage *= 1.f + AdaptPerStack * static_cast<float>(AdaptStacks);
	}

	// --- Crit (Frenzy supercharges both halves) ---
	const float FrenzyStat = Capture(RLDamageStatics::Get().FrenzyDef);
	if (bFrenzied && FrenzyStat > 0.f)
	{
		CritChance += RLCombat::FrenzyCritBonus * FrenzyStat;
	}

	bool bCrit = false;
	if (CritChance > 0.f && FMath::FRand() < CritChance)
	{
		bCrit = true;
		float CritMultiplier = Capture(RLDamageStatics::Get().CritDamageDef)
			+ Capture(RLDamageStatics::Get().ForceDef);
		if (bFrenzied && FrenzyStat > 0.f)
		{
			CritMultiplier += RLCombat::FrenzyForceBonus * FrenzyStat;
		}
		Damage *= FMath::Max(CritMultiplier, 1.f);
	}

	// --- Armor: physical only; spells slip straight through (Classic) ---
	if (!bSpell)
	{
		const float Armor = Capture(RLDamageStatics::Get().ArmorDef);
		const float AttackerLevel = FMath::Max(Spec.GetLevel(), 1.f);
		Damage *= 1.f - RLCombat::ArmorDamageReduction(Armor, AttackerLevel);
	}

	// --- Multistrike: the hit echoes at reduced effectiveness ---
	int32 Instances = 1;
	float TotalDamage = Damage;
	const float MultistrikeChance = FMath::Clamp(
		Capture(RLDamageStatics::Get().MultistrikeDef) + SynergyBonus, 0.f, 1.f);
	if (MultistrikeChance > 0.f && FMath::FRand() < MultistrikeChance)
	{
		TotalDamage += Damage * RLCombat::MultistrikeEffectiveness;
		Instances = 2;
	}

	// Advance the attacker's proc state for the NEXT hit.
	if (SourceASC)
	{
		AActor* Victim = ExecutionParams.GetTargetAbilitySystemComponent()
			? ExecutionParams.GetTargetAbilitySystemComponent()->GetAvatarActor()
			: nullptr;
		SourceASC->NotifyDamageDealt(Victim, bCrit, Instances);
	}

	if (TotalDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			RLDamageStatics::Get().IncomingDamageProperty, EGameplayModOp::Additive, TotalDamage));
	}
}
