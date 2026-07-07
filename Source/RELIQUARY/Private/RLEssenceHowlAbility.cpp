#include "RLEssenceHowlAbility.h"
#include "RLDamageEffect.h"
#include "RLGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

URLEssenceHowlAbility::URLEssenceHowlAbility()
{
	ActionTag = RLTags::Ability_Essence;
	DamageEffectClass = URLDamageEffect::StaticClass();
	BaseDamage.Value = 15.f;
}

void URLEssenceHowlAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	OnHowl();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RLEssenceHowl), /*bTraceComplex=*/false, Avatar);
	Avatar->GetWorld()->OverlapMultiByChannel(Overlaps, Avatar->GetActorLocation(), FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params);

	TSet<AActor*> AlreadyHit;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Victim = Overlap.GetActor();
		if (!Victim || Victim == Avatar || AlreadyHit.Contains(Victim))
		{
			continue;
		}
		AlreadyHit.Add(Victim);

		UAbilitySystemComponent* VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim);
		if (!VictimASC)
		{
			continue;
		}

		ApplyDamageToTarget(Victim);

		// Demoralized: same loose-tag-with-removal-timer pattern the Reckless
		// Abandon shockwave uses; the damage execution reads it per-hit.
		VictimASC->AddLooseGameplayTag(RLTags::State_Demoralized);

		FTimerHandle TimerHandle;
		TWeakObjectPtr<UAbilitySystemComponent> WeakASC = VictimASC;
		Victim->GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([WeakASC]()
		{
			if (WeakASC.IsValid())
			{
				WeakASC->RemoveLooseGameplayTag(RLTags::State_Demoralized);
			}
		}), DemoralizeSeconds, /*bLoop=*/false);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

bool URLEssenceHowlAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

void URLEssenceHowlAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
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

float URLEssenceHowlAbility::GetCooldownDuration() const
{
	return CooldownSeconds > 0.f ? GetHastedDuration(CooldownSeconds) : 0.f;
}

float URLEssenceHowlAbility::GetCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	if (!World || CooldownSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f,
		static_cast<float>(LastEndTimeSeconds + GetHastedDuration(CooldownSeconds) - World->GetTimeSeconds()));
}
