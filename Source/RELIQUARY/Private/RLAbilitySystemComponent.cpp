#include "RLAbilitySystemComponent.h"
#include "RLGameplayTags.h"
#include "RLGameplayAbility.h"
#include "RLGameplayEffectContext.h"

bool URLAbilitySystemComponent::TryActivateByActionTag(FGameplayTag ActionTag)
{
	if (!ActionTag.IsValid())
	{
		return false;
	}

	// Walk newest-first: talent keystones are granted after the class kit,
	// so a keystone sharing an ActionTag replaces the base ability.
	const TArray<FGameplayAbilitySpec>& Specs = GetActivatableAbilities();
	for (int32 Index = Specs.Num() - 1; Index >= 0; --Index)
	{
		const FGameplayAbilitySpec& Spec = Specs[Index];
		const URLGameplayAbility* Ability = Cast<URLGameplayAbility>(Spec.Ability);
		if (Ability && Ability->ActionTag == ActionTag)
		{
			if (Spec.IsActive())
			{
				// Re-press while the ability runs: forward it as a combo
				// input so montage-driven abilities can buffer a next stage.
				FGameplayEventData Payload;
				Payload.EventTag = RLTags::Event_Melee_Combo;
				Payload.Instigator = GetOwnerActor();
				HandleGameplayEvent(RLTags::Event_Melee_Combo, &Payload);
				return true;
			}
			return TryActivateAbility(Spec.Handle);
		}
	}
	return false;
}

void URLAbilitySystemComponent::NotifyActionReleased(FGameplayTag ActionTag)
{
	if (!ActionTag.IsValid())
	{
		return;
	}

	// Newest-first, mirroring TryActivateByActionTag's slot-override rule.
	ABILITYLIST_SCOPE_LOCK();
	TArray<FGameplayAbilitySpec>& Specs = GetActivatableAbilities();
	for (int32 Index = Specs.Num() - 1; Index >= 0; --Index)
	{
		FGameplayAbilitySpec& Spec = Specs[Index];
		const URLGameplayAbility* Ability = Cast<URLGameplayAbility>(Spec.Ability);
		if (Ability && Ability->ActionTag == ActionTag && Spec.IsActive())
		{
			// Pipes InputReleased to the active instance(s); the replicated
			// event also wakes any Blueprint WaitInputRelease tasks.
			AbilitySpecInputReleased(Spec);

			FPredictionKey PredictionKey;
			if (const UGameplayAbility* Instance = Spec.GetPrimaryInstance())
			{
				PredictionKey = Instance->GetCurrentActivationInfo().GetActivationPredictionKey();
			}
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, PredictionKey);
			return;
		}
	}
}

URLGameplayAbility* URLAbilitySystemComponent::FindAbilityByActionTag(FGameplayTag ActionTag) const
{
	if (!ActionTag.IsValid())
	{
		return nullptr;
	}

	// Newest-first, mirroring TryActivateByActionTag's slot-override rule.
	const TArray<FGameplayAbilitySpec>& Specs = GetActivatableAbilities();
	for (int32 Index = Specs.Num() - 1; Index >= 0; --Index)
	{
		const FGameplayAbilitySpec& Spec = Specs[Index];
		URLGameplayAbility* Ability = Cast<URLGameplayAbility>(Spec.Ability);
		if (Ability && Ability->ActionTag == ActionTag)
		{
			// Cooldown timestamps live on the instanced-per-actor copy.
			if (URLGameplayAbility* Instance = Cast<URLGameplayAbility>(Spec.GetPrimaryInstance()))
			{
				return Instance;
			}
			return Ability;
		}
	}
	return nullptr;
}

void URLAbilitySystemComponent::NotifyActionUsed(const FGameplayTag& ActionTag)
{
	if (!ActionTag.IsValid())
	{
		return;
	}

	if (ActionTag == LastActionTag)
	{
		// Lazy play: repeating your last action drops all stacks.
		AdaptabilityStacks = 0;
	}
	else
	{
		AdaptabilityStacks = FMath::Min(AdaptabilityStacks + 1, MaxAdaptabilityStacks);
	}

	LastActionTag = ActionTag;
}

void URLAbilitySystemComponent::HandleDamageTaken(float Damage, const FGameplayEffectContextHandle& Context, bool bLethal)
{
	AActor* InstigatorActor = Context.GetOriginalInstigator();

	// Recover the crit flag the damage execution stashed on our context type.
	bool bCritical = false;
	if (const FGameplayEffectContext* Raw = Context.Get())
	{
		if (Raw->GetScriptStruct() == FRLGameplayEffectContext::StaticStruct())
		{
			bCritical = static_cast<const FRLGameplayEffectContext*>(Raw)->IsCriticalHit();
		}
	}

	OnDamageTaken.Broadcast(Damage, InstigatorActor, bCritical);

	if (bLethal && !bDeathHandled)
	{
		bDeathHandled = true;
		AddLooseGameplayTag(RLTags::State_Dead);
		OnDeath.Broadcast();
	}
}
