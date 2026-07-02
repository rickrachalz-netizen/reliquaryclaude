#include "RLAbilitySystemComponent.h"
#include "RLGameplayTags.h"
#include "RLGameplayAbility.h"
#include "RLAttributeSet.h"
#include "RLCombatFormulas.h"
#include "Engine/World.h"

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

double URLAbilitySystemComponent::NowSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

void URLAbilitySystemComponent::NotifyDamageDealt(AActor* Victim, bool bCrit, int32 Instances)
{
	// Hatred: chains build on one victim and break when you switch targets.
	if (Victim)
	{
		if (HatredVictim.Get() == Victim)
		{
			HatredStacks = FMath::Min(HatredStacks + 1, RLCombat::HatredMaxStacks);
		}
		else
		{
			HatredVictim = Victim;
			HatredStacks = 1;
		}
	}

	// Synergy: crit chains build; the first non-crit spends them all.
	if (bCrit)
	{
		SynergyStacks = FMath::Min(SynergyStacks + 1, RLCombat::SynergyMaxStacks);
	}
	else
	{
		SynergyStacks = 0;
	}

	// Frenzy: N damage instances inside the window lights the fuse.
	const double Now = NowSeconds();
	for (int32 i = 0; i < Instances; ++i)
	{
		RecentInstanceTimes.Add(Now);
	}
	RecentInstanceTimes.RemoveAll([&](double Time)
	{
		return Now - Time > RLCombat::FrenzyWindowSeconds;
	});

	if (RecentInstanceTimes.Num() >= RLCombat::FrenzyInstanceCount)
	{
		const bool bWasFrenzied = IsFrenzied();
		FrenzyActiveUntil = Now + RLCombat::FrenzyDurationSeconds;
		if (!bWasFrenzied)
		{
			OnFrenzyTriggered.Broadcast();
		}
	}
}

float URLAbilitySystemComponent::GetHatredHasteBonus() const
{
	const float HatredPerStack = GetNumericAttribute(URLAttributeSet::GetHatredAttribute());
	return HatredPerStack * static_cast<float>(HatredStacks);
}

bool URLAbilitySystemComponent::IsFrenzied() const
{
	return FrenzyActiveUntil > 0.0 && NowSeconds() < FrenzyActiveUntil;
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
