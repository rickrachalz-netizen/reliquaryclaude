#include "RLMeleeAttackAbility.h"
#include "RLGameplayTags.h"
#include "RLDamageEffect.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Actor.h"
#include "Engine/OverlapResult.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"

URLMeleeAttackAbility::URLMeleeAttackAbility()
{
	ActionTag = RLTags::Ability_Primary;
	DamageEffectClass = URLDamageEffect::StaticClass();
	BaseDamage.Value = 12.f;

	// Bread-and-butter swing: physical, modest attack-power scaling.
	DamageSchool = ERLDamageSchool::Physical;
	PowerCoefficient = 0.8f;
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

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (Avatar)
	{
		const FVector Center = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * Range;

		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(RLMeleeAttack), /*bTraceComplex=*/false, Avatar);
		Avatar->GetWorld()->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity,
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

			// Damageable through GAS?
			if (UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim))
			{
				ApplyDamageToTarget(Victim);
				OnMeleeHit(Victim);
			}
			else
			{
				// Destructible world: trees, rocks, and the like use plain
				// point damage so anything can shatter into resources.
				const float WorldDamage = BaseDamage.GetValueAtLevel(GetAbilityLevel());
				Victim->TakeDamage(WorldDamage, FDamageEvent(), nullptr, Avatar);
				OnMeleeHit(Victim);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
