#include "RLEnemyBase.h"
#include "RLAbilitySystemComponent.h"
#include "RLAttributeSet.h"
#include "RLDamageEffect.h"
#include "RLGameplayTags.h"
#include "RLGameInstance.h"
#include "RLRunManagerSubsystem.h"
#include "RLResourcePickup.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

ARLEnemyBase::ARLEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Spawned enemies possess themselves; without a controller the movement
	// component never simulates, leaving them frozen in the air.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);

	AbilitySystemComponent = CreateDefaultSubobject<URLAbilitySystemComponent>(TEXT("ASC"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Attributes = CreateDefaultSubobject<URLAttributeSet>(TEXT("Attributes"));
}

UAbilitySystemComponent* ARLEnemyBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ARLEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->AddLooseGameplayTag(RLTags::Enemy);

	// Scale with the run: the world grows steadily more lethal over time.
	float Difficulty = 1.f;
	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>() : nullptr)
	{
		Difficulty = RunManager->GetDifficultyCoefficient();
	}

	float HealthMult = Difficulty;
	float DamageMult = FMath::Sqrt(Difficulty);	// damage scales gentler than HP

	if (bElite)
	{
		HealthMult *= 3.f;
		DamageMult *= 1.5f;
		AbilitySystemComponent->AddLooseGameplayTag(RLTags::Enemy_Elite);
		if (DropItemId == NAME_None)
		{
			DropItemId = FName(TEXT("ManalithShard"));
			DropCount = 2;
		}
	}
	if (bEmpowered)
	{
		HealthMult *= 4.f;
		DamageMult *= 1.5f;
		XPReward *= 5;
		ExcessManaReward *= 4;
		AbilitySystemComponent->AddLooseGameplayTag(RLTags::Enemy_Boss);
	}

	const float MaxHealth = BaseHealth * HealthMult;
	Attributes->InitMaxHealth(MaxHealth);
	Attributes->InitHealth(MaxHealth);
	Attributes->InitStrength(BaseDamage * DamageMult);	// feeds the damage formula
	Attributes->InitArmor(BaseArmor);
	Attributes->InitCritChance(0.f);
	Attributes->InitCritDamage(2.f);
	Attributes->InitMoveSpeed(BaseMoveSpeed);
	GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed;

	AbilitySystemComponent->OnDamageTaken.AddDynamic(this, &ARLEnemyBase::HandleDamageTaken);
	AbilitySystemComponent->OnDeath.AddDynamic(this, &ARLEnemyBase::HandleDeath);
}

void ARLEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDead || !bChaseHero)
	{
		return;
	}

	APawn* Hero = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Hero)
	{
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), Hero->GetActorLocation());
	if (Distance > AggroRange)
	{
		return;
	}

	TimeSinceAttack += DeltaSeconds;

	const float StrikeReach = AttackRange + GetCapsuleComponent()->GetScaledCapsuleRadius();
	if (Distance <= StrikeReach)
	{
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
		}
		bNavMovement = false;

		// Square up to the hero while winding the next strike.
		const FVector ToHero = (Hero->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!ToHero.IsNearlyZero())
		{
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), ToHero.Rotation(), DeltaSeconds, 8.f));
		}

		if (TimeSinceAttack >= AttackInterval)
		{
			TimeSinceAttack = 0.f;
			OnTouchAttack(Hero);
			DealTouchDamage(Hero);
		}
		return;
	}

	// Chase: nav pathfollowing when a navmesh is available, beeline otherwise.
	AAIController* AI = Cast<AAIController>(GetController());
	RepathTimer -= DeltaSeconds;
	if (AI && RepathTimer <= 0.f)
	{
		RepathTimer = 0.35f;
		const EPathFollowingRequestResult::Type Result =
			AI->MoveToActor(Hero, AttackRange * 0.5f, /*bStopOnOverlap=*/true);
		bNavMovement = Result != EPathFollowingRequestResult::Failed;
	}
	if (!bNavMovement)
	{
		AddMovementInput((Hero->GetActorLocation() - GetActorLocation()).GetSafeNormal2D());
	}
}

void ARLEnemyBase::DealTouchDamage(AActor* Target)
{
	if (bDead || !Target)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddInstigator(this, this);
	FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(
		URLDamageEffect::StaticClass(), 1.f, Context);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(RLTags::SetByCaller_Damage, BaseDamage);
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void ARLEnemyBase::HandleDamageTaken(float Damage, AActor* InstigatorActor)
{
	if (InstigatorActor && InstigatorActor != this)
	{
		LastDamager = InstigatorActor;
	}
}

void ARLEnemyBase::HandleDeath()
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	AActor* Killer = LastDamager;
	GrantRewards(Killer);
	OnEnemyKilled.Broadcast(this, Killer);
	OnDied(Killer);

	// Ragdoll, then fade.
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
	DetachFromControllerPendingDestroy();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->SetSimulatePhysics(true);
	}
	SetLifeSpan(8.f);
}

void ARLEnemyBase::GrantRewards(AActor* Killer)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	if (URLGameInstance* RLGI = Cast<URLGameInstance>(GI))
	{
		RLGI->AddExperience(XPReward);
	}

	if (URLRunManagerSubsystem* RunManager = GI->GetSubsystem<URLRunManagerSubsystem>())
	{
		RunManager->AddExcessMana(ExcessManaReward);
	}

	if (DropItemId != NAME_None && DropCount > 0)
	{
		for (int32 i = 0; i < DropCount; ++i)
		{
			const FVector Offset(FMath::FRandRange(-80.f, 80.f), FMath::FRandRange(-80.f, 80.f), 60.f);
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ARLResourcePickup* Pickup = GetWorld()->SpawnActor<ARLResourcePickup>(
				ARLResourcePickup::StaticClass(), GetActorLocation() + Offset, FRotator::ZeroRotator, Params))
			{
				Pickup->ItemId = DropItemId;
				Pickup->Count = 1;
			}
		}
	}
}
