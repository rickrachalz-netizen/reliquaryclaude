#include "RLBladestormAbility.h"
#include "RLDamageEffect.h"
#include "RLGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

URLBladestormAbility::URLBladestormAbility()
{
	ActionTag = RLTags::Ability_Primary;
	DamageEffectClass = URLDamageEffect::StaticClass();
	BaseDamage.Value = 20.f;
}

void URLBladestormAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	TicksDone = 0;

	if (SpinMontage)
	{
		UAbilityTask_PlayMontageAndWait* Spin = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, SpinMontage, MontagePlayRate, NAME_None, /*bStopWhenAbilityEnds=*/true);
		Spin->OnInterrupted.AddDynamic(this, &URLBladestormAbility::HandleMontageCancelled);
		Spin->OnCancelled.AddDynamic(this, &URLBladestormAbility::HandleMontageCancelled);
		Spin->ReadyForActivation();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TickTimerHandle,
			FTimerDelegate::CreateUObject(this, &URLBladestormAbility::DoSpinTick),
			TickInterval, /*bLoop=*/true);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
	}
}

void URLBladestormAbility::DoSpinTick()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	++TicksDone;
	OnSpinTick(TicksDone);

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RLBladestorm), /*bTraceComplex=*/false, Avatar);
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

		if (UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim))
		{
			ApplyDamageToTarget(Victim);
		}
	}

	if (TicksDone >= TickCount)
	{
		// The last tick winds up the next Mortal Strike.
		if (UAbilitySystemComponent* SelfASC = GetAbilitySystemComponentFromActorInfo())
		{
			SelfASC->AddLooseGameplayTag(RLTags::State_BladestormEmpowered);
		}
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
			/*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
	}
}

void URLBladestormAbility::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
		/*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
}

bool URLBladestormAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

void URLBladestormAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickTimerHandle);
		LastEndTimeSeconds = World->GetTimeSeconds();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
