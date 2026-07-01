#include "RLProjectileAbility.h"
#include "RLProjectile.h"
#include "RLDamageEffect.h"
#include "RLGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

URLProjectileAbility::URLProjectileAbility()
{
	ActionTag = RLTags::Ability_Primary;
	DamageEffectClass = URLDamageEffect::StaticClass();
	ProjectileClass = ARLProjectile::StaticClass();
	BaseDamage.Value = 14.f;
	ManaCost = 5.f;
}

void URLProjectileAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APawn* Avatar = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (Avatar && ProjectileClass)
	{
		// Fire along the control rotation so the dynamic third-person camera aims.
		const FRotator AimRot = Avatar->GetController() ? Avatar->GetController()->GetControlRotation() : Avatar->GetActorRotation();
		const FVector SpawnLoc = Avatar->GetActorLocation() + AimRot.Vector() * MuzzleDistance + FVector(0, 0, 40.f);

		FActorSpawnParameters Params;
		Params.Instigator = Avatar;
		Params.Owner = Avatar;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (ARLProjectile* Projectile = Avatar->GetWorld()->SpawnActor<ARLProjectile>(ProjectileClass, SpawnLoc, AimRot, Params))
		{
			FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
			if (Spec.IsValid())
			{
				Spec.Data->SetSetByCallerMagnitude(RLTags::SetByCaller_Damage, BaseDamage.GetValueAtLevel(GetAbilityLevel()));
			}
			Projectile->DamageSpec = Spec;
			Projectile->WorldDamage = BaseDamage.GetValueAtLevel(GetAbilityLevel());
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
