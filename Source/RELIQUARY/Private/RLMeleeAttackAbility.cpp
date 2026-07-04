#include "RLMeleeAttackAbility.h"
#include "RLGameplayTags.h"
#include "RLDamageEffect.h"
#include "RLResourceNode.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimMontage.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

URLMeleeAttackAbility::URLMeleeAttackAbility()
{
	ActionTag = RLTags::Ability_Primary;
	DamageEffectClass = URLDamageEffect::StaticClass();
	BaseDamage.Value = 12.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void URLMeleeAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	ComboIndex = 0;
	bComboQueued = false;
	bHitFiredThisSwing = false;

	// No montages: the original instant swing, playable from day one.
	if (ComboMontages.IsEmpty())
	{
		DoSweep();
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
		return;
	}

	// One listener pair for the whole combo chain.
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, RLTags::Event_Melee_Hit, nullptr, /*OnlyTriggerOnce=*/false, /*OnlyMatchExact=*/true);
	HitTask->EventReceived.AddDynamic(this, &URLMeleeAttackAbility::HandleHitEvent);
	HitTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* ComboTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, RLTags::Event_Melee_Combo, nullptr, /*OnlyTriggerOnce=*/false, /*OnlyMatchExact=*/true);
	ComboTask->EventReceived.AddDynamic(this, &URLMeleeAttackAbility::HandleComboEvent);
	ComboTask->ReadyForActivation();

	PlayComboStage();
}

void URLMeleeAttackAbility::PlayComboStage()
{
	bComboQueued = false;
	bHitFiredThisSwing = false;

	UAbilityTask_PlayMontageAndWait* Stage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, ComboMontages[ComboIndex], PlayRate, NAME_None, /*bStopWhenAbilityEnds=*/true);
	Stage->OnBlendOut.AddDynamic(this, &URLMeleeAttackAbility::HandleStageBlendOut);
	Stage->OnInterrupted.AddDynamic(this, &URLMeleeAttackAbility::HandleStageCancelled);
	Stage->OnCancelled.AddDynamic(this, &URLMeleeAttackAbility::HandleStageCancelled);
	Stage->ReadyForActivation();
}

void URLMeleeAttackAbility::HandleHitEvent(FGameplayEventData Payload)
{
	bHitFiredThisSwing = true;
	DoSweep();
}

void URLMeleeAttackAbility::HandleComboEvent(FGameplayEventData Payload)
{
	bComboQueued = true;
}

void URLMeleeAttackAbility::HandleStageBlendOut()
{
	// A montage without the RL Melee Hit notify still lands its damage.
	if (!bHitFiredThisSwing)
	{
		DoSweep();
	}

	if (bComboQueued && ComboMontages.IsValidIndex(ComboIndex + 1))
	{
		++ComboIndex;
		PlayComboStage();
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
		/*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

void URLMeleeAttackAbility::HandleStageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
		/*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
}

void URLMeleeAttackAbility::DoSweep()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	const bool bFinisher = ComboMontages.Num() > 1 && ComboIndex == ComboMontages.Num() - 1;
	const float Multiplier = bFinisher ? FinisherMultiplier : 1.f;

	const FVector Center = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * Range;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RLMeleeAttack), /*bTraceComplex=*/false, Avatar);
	Avatar->GetWorld()->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params);

	// Feel feedback only for things worth crunching on — enemies and
	// resource nodes, never the floor or walls the sphere happens to touch.
	TSet<AActor*> AlreadyHit;
	TArray<AActor*> FeelVictims;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Victim = Overlap.GetActor();
		if (!Victim || Victim == Avatar || AlreadyHit.Contains(Victim))
		{
			continue;
		}
		AlreadyHit.Add(Victim);

		// Damageable through GAS?
		if (UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim))
		{
			ApplyDamageToTarget(Victim, Multiplier);
			FeelVictims.Add(Victim);
			OnMeleeHit(Victim);
		}
		else
		{
			// Destructible world: trees, rocks, and the like use plain
			// point damage so anything can shatter into resources.
			const float WorldDamage = BaseDamage.GetValueAtLevel(GetAbilityLevel()) * Multiplier;
			Victim->TakeDamage(WorldDamage, FDamageEvent(), nullptr, Avatar);
			if (Victim->IsA<ARLResourceNode>())
			{
				FeelVictims.Add(Victim);
			}
			OnMeleeHit(Victim);
		}
	}

	if (FeelVictims.Num() > 0)
	{
		ApplyHitFeedback(Avatar, FeelVictims);
	}
}

void URLMeleeAttackAbility::ApplyHitFeedback(AActor* Avatar, const TArray<AActor*>& Victims)
{
	// Shove victims away from the attacker.
	if (KnockbackStrength > 0.f)
	{
		for (AActor* Victim : Victims)
		{
			if (ACharacter* VictimChar = Cast<ACharacter>(Victim))
			{
				const FVector Away = (VictimChar->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal2D();
				VictimChar->LaunchCharacter(
					Away * KnockbackStrength + FVector(0.f, 0.f, KnockbackStrength * 0.4f),
					/*bXYOverride=*/true, /*bZOverride=*/true);
			}
		}
	}

	// Hit-stop: a blink of frozen time on both sides sells the contact.
	if (HitStopSeconds > 0.f)
	{
		TArray<TWeakObjectPtr<AActor>> Frozen;
		Frozen.Add(Avatar);
		Avatar->CustomTimeDilation = 0.05f;
		for (AActor* Victim : Victims)
		{
			Victim->CustomTimeDilation = 0.05f;
			Frozen.Add(Victim);
		}

		FTimerHandle Handle;
		Avatar->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([Frozen]()
		{
			for (const TWeakObjectPtr<AActor>& Actor : Frozen)
			{
				if (Actor.IsValid())
				{
					Actor->CustomTimeDilation = 1.f;
				}
			}
		}), HitStopSeconds, /*bLoop=*/false);
	}

	if (HitCameraShake)
	{
		if (const APawn* Pawn = Cast<APawn>(Avatar))
		{
			if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
			{
				PC->ClientStartCameraShake(HitCameraShake);
			}
		}
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Avatar, HitSound, Victims[0]->GetActorLocation());
	}
}
