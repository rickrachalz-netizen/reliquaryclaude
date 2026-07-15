#include "RLAllInAbility.h"
#include "RELIQUARYCharacter.h"
#include "RLGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

URLAllInAbility::URLAllInAbility()
{
	ActionTag = RLTags::Ability_Utility;
	AbilityDisplayName = NSLOCTEXT("RELIQUARY", "AllInAbilityName", "All in!");
}

void URLAllInAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Avatar || !ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	OnAllIn();

	// Parry stance: same loose-tag-with-removal-timer pattern as Mortal Wounds
	// and Demoralized; the damage execution reads it per-hit for any attacker.
	ASC->AddLooseGameplayTag(RLTags::State_Parry);

	FTimerHandle TimerHandle;
	TWeakObjectPtr<UAbilitySystemComponent> WeakASC = ASC;
	Avatar->GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([WeakASC]()
	{
		if (WeakASC.IsValid())
		{
			WeakASC->RemoveLooseGameplayTag(RLTags::State_Parry);
		}
	}), BuffDurationSeconds, /*bLoop=*/false);

	if (ARELIQUARYCharacter* Hero = Cast<ARELIQUARYCharacter>(Avatar))
	{
		Hero->ApplyTemporaryRegenMultiplier(RegenMultiplier, BuffDurationSeconds);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

bool URLAllInAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (CooldownSeconds > 0.f && ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		if (const UWorld* World = ActorInfo->AvatarActor->GetWorld())
		{
			if (World->GetTimeSeconds() < LastEndTimeSeconds + GetHastedDuration(CooldownSeconds))
			{
				return false;
			}
		}
	}
	return true;
}

void URLAllInAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// Only start the cooldown on a successful cast, so a blocked commit
	// doesn't lock the button.
	if (!bWasCancelled)
	{
		if (const UWorld* World = GetWorld())
		{
			LastEndTimeSeconds = World->GetTimeSeconds();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

float URLAllInAbility::GetCooldownDuration() const
{
	return CooldownSeconds > 0.f ? GetHastedDuration(CooldownSeconds) : 0.f;
}

float URLAllInAbility::GetCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	if (!World || CooldownSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f,
		static_cast<float>(LastEndTimeSeconds + GetHastedDuration(CooldownSeconds) - World->GetTimeSeconds()));
}
