#include "RLGameplayAbility.h"
#include "RLAbilitySystemComponent.h"
#include "RLAttributeSet.h"
#include "RLGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"

URLGameplayAbility::URLGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Dead actors can't act.
	ActivationBlockedTags.AddTag(RLTags::State_Dead);
}

bool URLGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (ManaCost > 0.f)
	{
		const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (!ASC || ASC->GetNumericAttribute(URLAttributeSet::GetManaAttribute()) < ManaCost)
		{
			return false;
		}
	}
	return true;
}

void URLGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (URLAbilitySystemComponent* RLASC = Cast<URLAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		RLASC->NotifyActionUsed(ActionTag);

		if (ManaCost > 0.f)
		{
			RLASC->ApplyModToAttribute(URLAttributeSet::GetManaAttribute(),
				EGameplayModOp::Additive, -ManaCost);
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void URLGameplayAbility::ApplyDamageToTarget(AActor* Target, float DamageMultiplier)
{
	if (!Target || !DamageEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!TargetASC || !SourceASC)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		const float Damage = BaseDamage.GetValueAtLevel(GetAbilityLevel()) * DamageMultiplier;
		SpecHandle.Data->SetSetByCallerMagnitude(RLTags::SetByCaller_Damage, Damage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

float URLGameplayAbility::GetHastedDuration(float BaseSeconds) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return BaseSeconds;
	}

	float Haste = ASC->GetNumericAttribute(URLAttributeSet::GetHasteAttribute());
	float Duration = BaseSeconds;

	// Battle Trance: bonus haste plus heavy cooldown compression near death.
	if (const ARELIQUARYCharacter* Hero = Cast<ARELIQUARYCharacter>(GetAvatarActorFromActorInfo()))
	{
		Haste += Hero->GetTranceHasteBonus();
		Duration *= (1.f - Hero->GetTranceCooldownReduction());
	}

	return Duration / FMath::Max(1.f + Haste, 0.1f);
}
