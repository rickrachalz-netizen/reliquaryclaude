#include "RLAbilitySystemComponent.h"
#include "RLGameplayTags.h"
#include "RLGameplayAbility.h"

bool URLAbilitySystemComponent::TryActivateByActionTag(FGameplayTag ActionTag)
{
	if (!ActionTag.IsValid())
	{
		return false;
	}

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const URLGameplayAbility* Ability = Cast<URLGameplayAbility>(Spec.Ability);
		if (Ability && Ability->ActionTag == ActionTag)
		{
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
