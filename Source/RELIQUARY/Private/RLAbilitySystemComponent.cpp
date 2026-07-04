#include "RLAbilitySystemComponent.h"
#include "RLGameplayTags.h"
#include "RLGameplayAbility.h"

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
	OnDamageTaken.Broadcast(Damage, InstigatorActor);

	if (bLethal && !bDeathHandled)
	{
		bDeathHandled = true;
		AddLooseGameplayTag(RLTags::State_Dead);
		OnDeath.Broadcast();
	}
}
