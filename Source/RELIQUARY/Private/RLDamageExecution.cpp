#include "RLDamageExecution.h"
#include "RLAttributeSet.h"
#include "RLAbilitySystemComponent.h"
#include "RLGameplayEffectContext.h"
#include "RLGameplayTags.h"
#include "RLRunManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

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

	// Parrying targets turn every attack aside ("All in!"). Kept distinct from
	// Invulnerable so a future unparryable/magic damage type can pierce it.
	if (EvalParams.TargetTags && EvalParams.TargetTags->HasTag(RLTags::State_Parry))
	{
		return;
	}

	float BaseDamage = Spec.GetSetByCallerMagnitude(RLTags::SetByCaller_Damage, /*WarnIfNotFound=*/false, 0.f);
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

	// Stat scaling: whichever primary stat is highest carries the build.
	const float Strength = Capture(RLDamageStatics::Get().StrengthDef);
	const float Agility = Capture(RLDamageStatics::Get().AgilityDef);
	const float Intellect = Capture(RLDamageStatics::Get().IntellectDef);
	const float PrimaryStat = FMath::Max3(Strength, Agility, Intellect);
	float Damage = BaseDamage * (1.f + PrimaryStat / 100.f);

	// Adaptability: reward varied play, up to five stacks.
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

	// Crit roll through the run's xoshiro256++ stream (seeded per embark);
	// falls back to global RNG outside a run context.
	const float CritChance = Capture(RLDamageStatics::Get().CritChanceDef);
	if (CritChance > 0.f)
	{
		float Roll = FMath::FRand();
		if (const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent())
		{
			const UWorld* World = SourceASC->GetWorld();
			UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
			if (URLRunManagerSubsystem* RunManager = GI ? GI->GetSubsystem<URLRunManagerSubsystem>() : nullptr)
			{
				Roll = RunManager->GetCombatRng().FRand();
			}
		}

		if (Roll < CritChance)
		{
			const float CritDamage = Capture(RLDamageStatics::Get().CritDamageDef);
			Damage *= FMath::Max(CritDamage, 1.f);

			// Stash the crit on the shared context so the target's damage number
			// can flag it. Copying the handle keeps a mutable view of the same
			// underlying context the const spec holds.
			FGameplayEffectContextHandle ContextHandle = Spec.GetContext();
			if (FGameplayEffectContext* RawContext = ContextHandle.Get())
			{
				if (RawContext->GetScriptStruct() == FRLGameplayEffectContext::StaticStruct())
				{
					static_cast<FRLGameplayEffectContext*>(RawContext)->SetIsCriticalHit(true);
				}
			}
		}
	}

	// Demoralizing Shout: a demoralized attacker hits with less conviction.
	if (EvalParams.SourceTags && EvalParams.SourceTags->HasTag(RLTags::State_Demoralized))
	{
		Damage *= 0.7f;
	}

	// Armor mitigation with diminishing returns.
	const float Armor = FMath::Max(Capture(RLDamageStatics::Get().ArmorDef), 0.f);
	Damage *= 1.f - Armor / (Armor + 300.f);

	// Crushed Armor: Reckless Abandon's collision leaves the victim wide open.
	if (EvalParams.TargetTags && EvalParams.TargetTags->HasTag(RLTags::State_CrushedArmor))
	{
		Damage *= 1.3f;
	}

	if (Damage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			RLDamageStatics::Get().IncomingDamageProperty, EGameplayModOp::Additive, Damage));
	}
}
