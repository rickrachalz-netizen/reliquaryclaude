#include "RLRecklessAbandonAbility.h"
#include "RELIQUARYCharacter.h"
#include "RLAttributeSet.h"
#include "RLDamageEffect.h"
#include "RLEnemyBase.h"
#include "RLGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

URLRecklessAbandonAbility::URLRecklessAbandonAbility()
{
	ActionTag = RLTags::Ability_Special;
	DamageEffectClass = URLDamageEffect::StaticClass();
	BaseDamage.Value = 45.f;

	// Kit button releases reach InputReleased without netcode plumbing.
	bReplicateInputDirectly = true;
}

bool URLRecklessAbandonAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

void URLRecklessAbandonAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	UWorld* World = GetWorld();
	ACharacter* AvatarChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!World || !AvatarChar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	bCharging = true;
	bDashing = false;
	ChargeStartTimeSeconds = World->GetTimeSeconds();

	// Dig in: the feet plant while grounded. A midair charge keeps falling.
	if (AvatarChar->GetCharacterMovement()->IsMovingOnGround())
	{
		AvatarChar->GetCharacterMovement()->DisableMovement();
		bMovementLocked = true;
	}

	OnChargeStarted();
	World->GetTimerManager().SetTimer(ChargeTickTimerHandle,
		FTimerDelegate::CreateUObject(this, &URLRecklessAbandonAbility::DoChargeTick),
		0.05f, /*bLoop=*/true);
}

float URLRecklessAbandonAbility::GetCooldownDuration() const
{
	return CooldownSeconds > 0.f ? GetHastedDuration(CooldownSeconds) : 0.f;
}

float URLRecklessAbandonAbility::GetCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	if (!World || CooldownSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f,
		static_cast<float>(LastEndTimeSeconds + GetHastedDuration(CooldownSeconds) - World->GetTimeSeconds()));
}

float URLRecklessAbandonAbility::GetChargeAlpha() const
{
	const UWorld* World = GetWorld();
	if (!bCharging || MaxChargeSeconds <= 0.f || !World)
	{
		return 0.f;
	}
	const float Held = static_cast<float>(World->GetTimeSeconds() - ChargeStartTimeSeconds);
	return FMath::Clamp(Held / MaxChargeSeconds, 0.f, 1.f);
}

void URLRecklessAbandonAbility::DoChargeTick()
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Avatar || (ASC && ASC->HasMatchingGameplayTag(RLTags::State_Dead)))
	{
		EndSelf(/*bWasCancelled=*/true);
		return;
	}
	OnChargeTick(GetChargeAlpha());
}

void URLRecklessAbandonAbility::InputReleased(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// Releases during windup/dash mean nothing; the charge is committed.
	if (!bCharging)
	{
		return;
	}
	bCharging = false;

	UWorld* World = GetWorld();
	if (!World)
	{
		EndSelf(/*bWasCancelled=*/true);
		return;
	}
	World->GetTimerManager().ClearTimer(ChargeTickTimerHandle);

	const float Held = static_cast<float>(World->GetTimeSeconds() - ChargeStartTimeSeconds);
	const float Alpha = MaxChargeSeconds > 0.f ? FMath::Clamp(Held / MaxChargeSeconds, 0.f, 1.f) : 1.f;

	// The dig-in always reads: even a tap plants for the minimum windup.
	const float RemainingWindup = MinWindupSeconds - Held;
	if (RemainingWindup > 0.f)
	{
		World->GetTimerManager().SetTimer(WindupTimerHandle,
			FTimerDelegate::CreateUObject(this, &URLRecklessAbandonAbility::LaunchDash, Alpha),
			RemainingWindup, /*bLoop=*/false);
	}
	else
	{
		LaunchDash(Alpha);
	}
}

void URLRecklessAbandonAbility::LaunchDash(float Alpha)
{
	UWorld* World = GetWorld();
	ACharacter* AvatarChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!World || !AvatarChar || (ASC && ASC->HasMatchingGameplayTag(RLTags::State_Dead)))
	{
		EndSelf(/*bWasCancelled=*/true);
		return;
	}

	UCharacterMovementComponent* MoveComp = AvatarChar->GetCharacterMovement();
	const bool bAirborne = !bMovementLocked && MoveComp->IsFalling();

	// Un-plant before flight.
	if (bMovementLocked)
	{
		MoveComp->SetMovementMode(MOVE_Walking);
		bMovementLocked = false;
	}

	// Yaw-only visual facing; the launch itself follows the full 3D aim.
	FaceCameraDirection();

	// Loader's gauntlet: launch along the camera's pitch and yaw. Aim up
	// and it throws you into the air; midair you can dive.
	const APlayerController* PC = CurrentActorInfo
		? Cast<APlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr;
	FVector Aim = PC ? PC->GetControlRotation().Vector() : AvatarChar->GetActorForwardVector();

	// From the ground you can't punch into the floor you stand on.
	if (!bAirborne && Aim.Z < 0.f)
	{
		Aim.Z = 0.f;
	}
	DashDirection = Aim.GetSafeNormal();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = AvatarChar->GetActorForwardVector().GetSafeNormal2D();
	}
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = FVector::ForwardVector;
	}

	DashDamageMultiplier = FMath::Lerp(MinDamageMultiplier, 1.f, Alpha);
	DashDistanceBudget = FMath::Lerp(MinDashDistance, MaxDashDistance, Alpha)
		* (bAirborne ? AirDistanceMultiplier : 1.f);
	LastDashLocation = AvatarChar->GetActorLocation();
	DashDistanceTraveled = 0.f;
	DashStartTimeSeconds = World->GetTimeSeconds();

	// Flying keeps the line straight (no gravity) and works midair.
	MoveComp->SetMovementMode(MOVE_Flying);
	MoveComp->Velocity = DashDirection * DashSpeed;

	bDashing = true;
	OnDashStarted(Alpha);

	World->GetTimerManager().SetTimer(DashTickTimerHandle,
		FTimerDelegate::CreateUObject(this, &URLRecklessAbandonAbility::DoDashTick),
		1.f / 120.f, /*bLoop=*/true);
}

void URLRecklessAbandonAbility::DoDashTick()
{
	UWorld* World = GetWorld();
	ACharacter* AvatarChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!World || !AvatarChar)
	{
		EndSelf(/*bWasCancelled=*/true);
		return;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(RLTags::State_Dead))
	{
		EndSelf(/*bWasCancelled=*/true);
		return;
	}

	const FVector Location = AvatarChar->GetActorLocation();

	// Path length, not displacement along one axis: skims bend the route.
	DashDistanceTraveled += static_cast<float>(FVector::Dist(Location, LastDashLocation));
	LastDashLocation = Location;

	// Sweep ahead of the capsule for the first thing the charge slams into.
	float CapsuleRadius = 42.f;
	if (const UCapsuleComponent* Capsule = AvatarChar->GetCapsuleComponent())
	{
		CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	}
	const float Lookahead = FMath::Max(DashSpeed / 60.f, CapsuleRadius) + CapsuleRadius;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RLRecklessAbandon), /*bTraceComplex=*/false, AvatarChar);
	World->SweepMultiByChannel(Hits, Location, Location + DashDirection * Lookahead,
		FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(CapsuleRadius + 20.f), Params);

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == AvatarChar)
		{
			continue;
		}

		if (ARLEnemyBase* Enemy = Cast<ARLEnemyBase>(HitActor))
		{
			if (!Enemy->IsDead())
			{
				ResolveEnemyImpact(Enemy);
				return;
			}
			continue;
		}

		if (Hit.bBlockingHit)
		{
			// Walkable ground deflects the punch along its surface — ride up
			// ramps, skim flats. Only objects (walls, trees, rocks) stop it.
			if (bSkimWalkableGround && Hit.ImpactNormal.Z >= SkimFloorNormalZ)
			{
				const FVector Deflected =
					FVector::VectorPlaneProject(DashDirection, Hit.ImpactNormal).GetSafeNormal();
				if (Deflected.IsNearlyZero())
				{
					// Near-vertical dive into flat ground: a landing, not an impact.
					EndDashWithMomentum();
					return;
				}
				DashDirection = Deflected;
				break; // keep dashing along the deflected direction
			}

			ResolveObstacleImpact(HitActor, Hit.bStartPenetrating ? Location : Hit.ImpactPoint);
			return;
		}
	}

	if (DashDistanceTraveled >= DashDistanceBudget)
	{
		// A clean whiff costs nothing — and keeps its momentum.
		EndDashWithMomentum();
		return;
	}

	// Hard stop in case geometry wedges the dash into an endless slide.
	const float ExpectedSeconds = DashDistanceBudget / FMath::Max(DashSpeed, 1.f);
	if (World->GetTimeSeconds() - DashStartTimeSeconds > ExpectedSeconds * 2.f + 0.5f)
	{
		EndDashWithMomentum();
		return;
	}

	AvatarChar->GetCharacterMovement()->Velocity = DashDirection * DashSpeed;
}

void URLRecklessAbandonAbility::ResolveEnemyImpact(AActor* Victim)
{
	StopDashMovement(/*bImpactBounce=*/true);

	ACharacter* AvatarChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!AvatarChar || !Victim)
	{
		EndSelf(/*bWasCancelled=*/true);
		return;
	}

	// The freight-train hit.
	ApplyDamageToTarget(Victim, DashDamageMultiplier);

	// Big hitstun on the body that stopped it.
	if (ARLEnemyBase* Enemy = Cast<ARLEnemyBase>(Victim))
	{
		Enemy->ApplyStun(HitStunSeconds);
	}

	// Crushed Armor: the victim is wide open for the follow-up.
	if (UAbilitySystemComponent* VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim))
	{
		VictimASC->AddLooseGameplayTag(RLTags::State_CrushedArmor);

		FTimerHandle Handle;
		TWeakObjectPtr<UAbilitySystemComponent> WeakASC = VictimASC;
		Victim->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakASC]()
		{
			if (WeakASC.IsValid())
			{
				WeakASC->RemoveLooseGameplayTag(RLTags::State_CrushedArmor);
			}
		}), CrushedArmorSeconds, /*bLoop=*/false);
	}

	// Shockwave: AoE damage plus Demoralizing Shout on everything caught.
	const FVector ImpactLocation = Victim->GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RLRecklessAoE), /*bTraceComplex=*/false, AvatarChar);
	GetWorld()->OverlapMultiByChannel(Overlaps, ImpactLocation, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(AoERadius), Params);

	TSet<AActor*> AlreadyHit;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Caught = Overlap.GetActor();
		if (!Caught || Caught == AvatarChar || AlreadyHit.Contains(Caught))
		{
			continue;
		}
		AlreadyHit.Add(Caught);

		if (ARLEnemyBase* CaughtEnemy = Cast<ARLEnemyBase>(Caught))
		{
			if (CaughtEnemy->IsDead())
			{
				continue;
			}

			ApplyDamageToTarget(Caught, AoEDamageMultiplier * DashDamageMultiplier);

			if (UAbilitySystemComponent* CaughtASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Caught))
			{
				CaughtASC->AddLooseGameplayTag(RLTags::State_Demoralized);

				FTimerHandle Handle;
				TWeakObjectPtr<UAbilitySystemComponent> WeakASC = CaughtASC;
				Caught->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakASC]()
				{
					if (WeakASC.IsValid())
					{
						WeakASC->RemoveLooseGameplayTag(RLTags::State_Demoralized);
					}
				}), DemoralizeSeconds, /*bLoop=*/false);
			}
		}
		else if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Caught))
		{
			// Destructible world: the shockwave shatters what it can.
			const float WorldDamage = BaseDamage.GetValueAtLevel(GetAbilityLevel())
				* AoEDamageMultiplier * DashDamageMultiplier;
			Caught->TakeDamage(WorldDamage, FDamageEvent(), nullptr, AvatarChar);
		}
	}

	ApplyImpactFeedback(Victim);
	ApplyRecoil();
	OnChargeImpact(Victim, ImpactLocation);
	EndSelf(/*bWasCancelled=*/false);
}

void URLRecklessAbandonAbility::ResolveObstacleImpact(AActor* HitActor, const FVector& ImpactLocation)
{
	StopDashMovement(/*bImpactBounce=*/true);

	ACharacter* AvatarChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	// Destructibles eat the direct hit: charging a tree shatters it.
	if (HitActor && AvatarChar && !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor))
	{
		const float WorldDamage = BaseDamage.GetValueAtLevel(GetAbilityLevel()) * DashDamageMultiplier;
		HitActor->TakeDamage(WorldDamage, FDamageEvent(), nullptr, AvatarChar);
	}

	ApplyImpactFeedback(nullptr);
	ApplyRecoil();
	OnChargeImpact(nullptr, ImpactLocation);
	EndSelf(/*bWasCancelled=*/false);
}

void URLRecklessAbandonAbility::ApplyRecoil()
{
	// Flat cut of current health, unmitigated: the same direct channel the
	// regen tick uses. Can never kill on its own, but feeds Battle Trance.
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && RecoilCurrentHealthFraction > 0.f)
	{
		const float Current = ASC->GetNumericAttribute(URLAttributeSet::GetHealthAttribute());
		if (Current > 0.f)
		{
			ASC->ApplyModToAttribute(URLAttributeSet::GetHealthAttribute(),
				EGameplayModOp::Additive, -Current * RecoilCurrentHealthFraction);
		}
	}

	if (ARELIQUARYCharacter* Hero = Cast<ARELIQUARYCharacter>(GetAvatarActorFromActorInfo()))
	{
		Hero->ApplyTemporarySpeedMultiplier(RecoilSlowMultiplier, RecoilSlowSeconds);
	}
}

void URLRecklessAbandonAbility::ApplyImpactFeedback(AActor* Victim)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	// Hit-stop: a blink of frozen time on both sides sells the collision.
	if (HitStopSeconds > 0.f)
	{
		TArray<TWeakObjectPtr<AActor>> Frozen;
		Avatar->CustomTimeDilation = 0.05f;
		Frozen.Add(Avatar);
		if (Victim)
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

	if (ImpactCameraShake)
	{
		if (const APawn* Pawn = Cast<APawn>(Avatar))
		{
			if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
			{
				PC->ClientStartCameraShake(ImpactCameraShake);
			}
		}
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Avatar, ImpactSound,
			(Victim ? Victim : Avatar)->GetActorLocation());
	}
}

void URLRecklessAbandonAbility::StopDashMovement(bool bImpactBounce)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashTickTimerHandle);
	}
	bDashing = false;

	if (ACharacter* AvatarChar = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		UCharacterMovementComponent* MoveComp = AvatarChar->GetCharacterMovement();
		if (MoveComp->MovementMode == MOVE_Flying)
		{
			MoveComp->SetMovementMode(MOVE_Falling); // resolves to walking on touch-down
			if (bImpactBounce)
			{
				// Bonk: pop up and back off whatever stopped the punch.
				// (A straight-down dive bounces purely upward.)
				const FVector Back = -DashDirection.GetSafeNormal2D();
				MoveComp->Velocity = Back * ImpactBounceBackSpeed
					+ FVector(0.f, 0.f, ImpactBounceUpSpeed);
			}
			else
			{
				MoveComp->Velocity = FVector::ZeroVector;
			}
		}
	}
}

void URLRecklessAbandonAbility::EndDashWithMomentum()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashTickTimerHandle);
	}
	bDashing = false;

	// Hand the dash velocity to gravity: aim up and you sail into an arc.
	if (ACharacter* AvatarChar = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		UCharacterMovementComponent* MoveComp = AvatarChar->GetCharacterMovement();
		if (MoveComp->MovementMode == MOVE_Flying)
		{
			const FVector Carried = MoveComp->Velocity * MomentumCarryFraction;
			MoveComp->SetMovementMode(MOVE_Falling);
			MoveComp->Velocity = Carried;
		}
	}

	EndSelf(/*bWasCancelled=*/false);
}

void URLRecklessAbandonAbility::EndSelf(bool bWasCancelled)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
		/*bReplicateEndAbility=*/true, bWasCancelled);
}

void URLRecklessAbandonAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChargeTickTimerHandle);
		World->GetTimerManager().ClearTimer(WindupTimerHandle);
		World->GetTimerManager().ClearTimer(DashTickTimerHandle);
		LastEndTimeSeconds = World->GetTimeSeconds();
	}

	// Put the avatar's feet back, whatever phase ended here — unless it died
	// (death owns the movement component from there).
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const bool bAvatarDead = ASC && ASC->HasMatchingGameplayTag(RLTags::State_Dead);
	if (ACharacter* AvatarChar = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		UCharacterMovementComponent* MoveComp = AvatarChar->GetCharacterMovement();
		if (!bAvatarDead)
		{
			if (bMovementLocked && MoveComp->MovementMode == MOVE_None)
			{
				MoveComp->SetMovementMode(MOVE_Walking);
			}
			if (MoveComp->MovementMode == MOVE_Flying)
			{
				MoveComp->Velocity = FVector::ZeroVector;
				MoveComp->SetMovementMode(MOVE_Falling);
			}
		}
	}
	bMovementLocked = false;
	bCharging = false;
	bDashing = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
