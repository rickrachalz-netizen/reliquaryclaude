#include "RLEnemyBase.h"
#include "RLAbilitySystemComponent.h"
#include "RLEnemyGroup.h"
#include "RLAttributeSet.h"
#include "RLDamageEffect.h"
#include "RLEnemyHealthBarWidget.h"
#include "RLDamageNumberWidget.h"
#include "RLGameplayTags.h"
#include "RLGameInstance.h"
#include "RLRunManagerSubsystem.h"
#include "RLDataSubsystem.h"
#include "RLResourcePickup.h"
#include "RLEssenceShardPickup.h"
#include "RLManaOrbPickup.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "DetourCrowdAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

ARLEnemyBase::ARLEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Spawned enemies possess themselves; without a controller the movement
	// component never simulates, leaving them frozen in the air.
	// DetourCrowd path following makes converging enemies flow around each
	// other instead of shoving into a slow-motion scrum.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ADetourCrowdAIController::StaticClass();
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);

	// Seat skeletal meshes at the capsule floor facing forward (most art is
	// authored facing +Y). Blueprints that hand-tweaked the mesh transform
	// keep their own values until reset to default.
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	AbilitySystemComponent = CreateDefaultSubobject<URLAbilitySystemComponent>(TEXT("ASC"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Attributes = CreateDefaultSubobject<URLAttributeSet>(TEXT("Attributes"));

	// Combat health bar: hidden until the enemy actually gets hurt.
	HealthBarWidgetClass = URLEnemyHealthBarWidget::StaticClass();
	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthBarComponent->SetupAttachment(GetCapsuleComponent());
	HealthBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComponent->SetDrawAtDesiredSize(true);
	HealthBarComponent->SetVisibility(false);

	DamageNumberWidgetClass = URLDamageNumberWidget::StaticClass();
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

	if (HealthBarComponent && HealthBarWidgetClass)
	{
		HealthBarComponent->SetWidgetClass(HealthBarWidgetClass);
	}

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
		ExcessManaReward = FMath::RoundToInt(ExcessManaReward * RLBalance::EliteManaMultiplier);
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
		ExcessManaReward = FMath::RoundToInt(ExcessManaReward * RLBalance::BossManaMultiplier);
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

	// Gentle crowd separation keeps clumped bodies from stacking; DetourCrowd
	// (the controller's path following) does the actual mutual avoidance.
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UCrowdFollowingComponent* Crowd = Cast<UCrowdFollowingComponent>(AI->GetPathFollowingComponent()))
		{
			Crowd->SetCrowdSeparation(true);
			Crowd->SetCrowdSeparationWeight(2.f);
		}
	}

	StuckCheckOrigin = GetActorLocation();
}

void ARLEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDead || !bChaseHero)
	{
		return;
	}

	// Stunned: no chasing, no striking.
	if (GetWorld()->GetTimeSeconds() < StunnedUntilSeconds)
	{
		return;
	}

	// A coordinating group (wolf pack, goblin gang) replaces the solo brain.
	if (IsValid(Group))
	{
		ExecuteGroupOrder(DeltaSeconds);
		return;
	}

	APawn* Hero = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Hero)
	{
		return;
	}

	if (FVector::Dist(GetActorLocation(), Hero->GetActorLocation()) > AggroRange)
	{
		return;
	}

	TickChaseAndStrike(Hero, DeltaSeconds);
}

void ARLEnemyBase::TickChaseAndStrike(APawn* Hero, float DeltaSeconds)
{
	TimeSinceAttack += DeltaSeconds;

	const float Distance = FVector::Dist(GetActorLocation(), Hero->GetActorLocation());
	const float Reach = GetStrikeReach();

	// Sticky melee: captured at reach, released only clearly beyond it — a
	// strafing hero on the boundary can't make the chaser stop/start-stutter.
	bInMeleeHold = bInMeleeHold ? (Distance <= Reach * 1.15f) : (Distance <= Reach);
	if (bInMeleeHold)
	{
		StopMoving();

		// Square up to the hero while winding the next strike.
		FaceLocation(Hero->GetActorLocation(), DeltaSeconds);

		if (TimeSinceAttack >= AttackInterval)
		{
			TimeSinceAttack = 0.f;
			OnTouchAttack(Hero);
			DealTouchDamage(Hero);
		}
		return;
	}

	// Chase: nav pathfollowing when a navmesh is available, beeline otherwise.
	if (TickUnstick(Hero->GetActorLocation(), DeltaSeconds))
	{
		return;
	}
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

void ARLEnemyBase::MoveTowardsLocation(const FVector& Target, float AcceptanceRadius, float DeltaSeconds)
{
	if (TickUnstick(Target, DeltaSeconds))
	{
		return;
	}
	AAIController* AI = Cast<AAIController>(GetController());
	RepathTimer -= DeltaSeconds;
	if (AI && RepathTimer <= 0.f)
	{
		RepathTimer = 0.35f;

		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		const bool bWorldHasNav = NavSystem && NavSystem->GetDefaultNavDataInstance() != nullptr;

		// Snap the goal onto the navmesh ourselves, with a generous extent.
		// Group targets are geometric (ring slots, formation offsets, wander
		// legs) and can hang past the mesh edge; the engine's own projection
		// uses a tiny extent, fails, and the old straight-line fallback then
		// marched enemies clean off the map.
		FVector Goal = Target;
		bool bGoalReachable = !bWorldHasNav;
		if (bWorldHasNav)
		{
			FNavLocation Projected;
			if (NavSystem->ProjectPointToNavigation(Target, Projected, FVector(600.f, 600.f, 2000.f)))
			{
				Goal = Projected.Location;
				bGoalReachable = true;
			}
		}

		if (bGoalReachable)
		{
			// bStopOnOverlap=false: location moves use the EXACT acceptance
			// radius. Capsule-inflated arrival would finish paths outside the
			// caller's stop ring and leave the pawn micro-stepping.
			const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(
				Goal, AcceptanceRadius, /*bStopOnOverlap=*/false, /*bUsePathfinding=*/true,
				/*bProjectDestinationToNavigation=*/false);
			bNavMovement = Result != EPathFollowingRequestResult::Failed;
		}
		else
		{
			bNavMovement = false;
		}

		// With a navmesh present, a failed move means the spot is genuinely
		// unreachable — hold this beat rather than walking off the world.
		bBeelineAllowed = !bWorldHasNav;
	}
	if (!bNavMovement && bBeelineAllowed)
	{
		AddMovementInput((Target - GetActorLocation()).GetSafeNormal2D());
	}
}

bool ARLEnemyBase::TickUnstick(const FVector& Goal, float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < UnstickUntilSeconds)
	{
		AddMovementInput(UnstickDirection);
		return true;
	}

	// Sample displacement over ~0.7s windows, but only while trying to move
	// (StopMoving resets the window, so striking/standing never counts).
	StuckTimer += DeltaSeconds;
	if (StuckTimer < 0.7f)
	{
		return false;
	}

	const float Moved = FVector::Dist2D(GetActorLocation(), StuckCheckOrigin);
	if (Moved < 40.f && FVector::Dist2D(GetActorLocation(), Goal) > 150.f)
	{
		// Wedged: sidestep perpendicular for a beat, alternating sides per
		// attempt, then let the mover plan a fresh path.
		bUnstickSideRight = !bUnstickSideRight;
		const FVector ToGoal = (Goal - GetActorLocation()).GetSafeNormal2D();
		UnstickDirection = FVector(-ToGoal.Y, ToGoal.X, 0.f) * (bUnstickSideRight ? 1.f : -1.f);
		UnstickUntilSeconds = Now + 0.6;
		StopMoving();
		RepathTimer = 0.f;
	}
	StuckTimer = 0.f;
	StuckCheckOrigin = GetActorLocation();
	return false;
}

void ARLEnemyBase::StopMoving()
{
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
	bNavMovement = false;
	StuckTimer = 0.f;
	StuckCheckOrigin = GetActorLocation();
}

void ARLEnemyBase::FaceLocation(const FVector& Target, float DeltaSeconds)
{
	const FVector To = (Target - GetActorLocation()).GetSafeNormal2D();
	if (!To.IsNearlyZero())
	{
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), To.Rotation(), DeltaSeconds, 8.f));
	}
}

void ARLEnemyBase::SetGroup(ARLEnemyGroup* InGroup)
{
	Group = InGroup;
	GroupOrder = ERLGroupOrder::None;
	if (!InGroup)
	{
		// Released: restore attribute speed and let the solo brain take over.
		// (Guarded — groups also release members during world teardown.)
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->MaxWalkSpeed = GetBaseMoveSpeed();
		}
		if (!bDead)
		{
			StopMoving();
		}
	}
}

void ARLEnemyBase::SetGroupOrder(ERLGroupOrder InOrder, const FVector& InLocation,
	float InSpeedScale, float InAcceptanceRadius, const FVector& InFaceLocation)
{
	if (GroupOrder != InOrder)
	{
		// Leaving a movement order: kill the active path request once, and let
		// a fresh movement order repath immediately and act on it.
		if (GroupOrder == ERLGroupOrder::MoveTo || GroupOrder == ERLGroupOrder::EngageHero)
		{
			StopMoving();
		}
		RepathTimer = 0.f;
		bOrderMoving = true;
	}
	GroupOrder = InOrder;
	GroupOrderLocation = InLocation;
	GroupOrderFace = InFaceLocation;
	GroupOrderSpeedScale = InSpeedScale;
	GroupOrderAcceptance = InAcceptanceRadius;
}

void ARLEnemyBase::ExecuteGroupOrder(float DeltaSeconds)
{
	// Group speeds scale off the unmodified MoveSpeed attribute every frame,
	// so scales never compound and releasing the enemy restores full speed.
	GetCharacterMovement()->MaxWalkSpeed = FMath::Max(50.f, GetBaseMoveSpeed() * GroupOrderSpeedScale);

	switch (GroupOrder)
	{
	case ERLGroupOrder::HoldFacing:
		FaceLocation(GroupOrderLocation, DeltaSeconds);
		break;

	case ERLGroupOrder::MoveTo:
	{
		// Latched: stop inside the acceptance ring, resume only well outside
		// it. A single boundary here made drifting targets toggle move/stop
		// every few frames — each toggle brakes, and braking is the molasses.
		const float Distance = FVector::Dist2D(GetActorLocation(), GroupOrderLocation);
		if (bOrderMoving && Distance <= GroupOrderAcceptance)
		{
			StopMoving();
			bOrderMoving = false;
		}
		else if (!bOrderMoving && Distance > GroupOrderAcceptance * 1.8f)
		{
			bOrderMoving = true;
		}

		if (bOrderMoving)
		{
			MoveTowardsLocation(GroupOrderLocation, GroupOrderAcceptance * 0.5f, DeltaSeconds);
		}
		else if (!GroupOrderFace.IsNearlyZero())
		{
			FaceLocation(GroupOrderFace, DeltaSeconds);
		}
		break;
	}

	case ERLGroupOrder::EngageHero:
		if (APawn* Hero = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			TickChaseAndStrike(Hero, DeltaSeconds);
		}
		break;

	default:
		break;
	}
}

void ARLEnemyBase::PerformTouchStrike(AActor* Target)
{
	if (bDead || !Target)
	{
		return;
	}
	TimeSinceAttack = 0.f;
	OnTouchAttack(Target);
	DealTouchDamage(Target);
}

float ARLEnemyBase::GetBaseMoveSpeed() const
{
	return Attributes ? Attributes->GetMoveSpeed() : BaseMoveSpeed;
}

float ARLEnemyBase::GetStrikeReach() const
{
	return AttackRange + GetCapsuleComponent()->GetScaledCapsuleRadius();
}

bool ARLEnemyBase::IsStunned() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < StunnedUntilSeconds;
}

void ARLEnemyBase::ApplyStun(float Seconds)
{
	if (bDead || Seconds <= 0.f)
	{
		return;
	}

	StunnedUntilSeconds = FMath::Max(StunnedUntilSeconds, GetWorld()->GetTimeSeconds() + Seconds);
	bNavMovement = false;
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
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

void ARLEnemyBase::HandleDamageTaken(float Damage, AActor* InstigatorActor, bool bCritical)
{
	if (InstigatorActor && InstigatorActor != this)
	{
		LastDamager = InstigatorActor;
	}

	// Pain wakes the whole pack, no matter how far the shot came from.
	if (!bDead && IsValid(Group))
	{
		Group->NotifyMemberDamaged(this, InstigatorActor);
	}

	// RoR2-style: the bar exists only while the enemy is being fought.
	if (!bDead)
	{
		RefreshHealthBar();
		SpawnDamageNumber(Damage, bCritical);
	}
}

void ARLEnemyBase::SpawnDamageNumber(float Damage, bool bCritical)
{
	if (!DamageNumberWidgetClass || Damage <= 0.f)
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	if (URLDamageNumberWidget* Widget = CreateWidget<URLDamageNumberWidget>(PC, DamageNumberWidgetClass))
	{
		Widget->AddToViewport(50);
		// Anchor at the head, same offset the health bar uses.
		Widget->InitDamageNumber(Damage, bCritical, GetActorLocation() + FVector(0.f, 0.f, 120.f));
	}
}

void ARLEnemyBase::RefreshHealthBar()
{
	if (!HealthBarComponent || !Attributes)
	{
		return;
	}

	if (URLEnemyHealthBarWidget* Widget = Cast<URLEnemyHealthBarWidget>(HealthBarComponent->GetUserWidgetObject()))
	{
		const float MaxHealth = Attributes->GetMaxHealth();
		Widget->SetHealthFraction(MaxHealth > 0.f ? Attributes->GetHealth() / MaxHealth : 0.f);

		// Bosses burn orange, elites violet, the rabble plain red.
		if (bEmpowered)
		{
			Widget->SetBarColor(FLinearColor(1.f, 0.45f, 0.05f));
		}
		else if (bElite)
		{
			Widget->SetBarColor(FLinearColor(0.55f, 0.2f, 0.9f));
		}
	}

	HealthBarComponent->SetVisibility(true);
	GetWorldTimerManager().SetTimer(HealthBarTimerHandle,
		FTimerDelegate::CreateUObject(this, &ARLEnemyBase::HideHealthBar),
		HealthBarLingerSeconds, /*bLoop=*/false);
}

void ARLEnemyBase::HideHealthBar()
{
	if (HealthBarComponent)
	{
		HealthBarComponent->SetVisibility(false);
	}
}

void ARLEnemyBase::HandleDeath()
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	GetWorldTimerManager().ClearTimer(HealthBarTimerHandle);
	HideHealthBar();

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

	// Death scatters mana orbs, scaled by how deadly the run has become.
	if (ExcessManaReward > 0)
	{
		URLRunManagerSubsystem* RunManager = GI->GetSubsystem<URLRunManagerSubsystem>();
		const float Difficulty = RunManager ? RunManager->GetDifficultyCoefficient() : 1.f;
		ARLManaOrbPickup::SpawnBurst(GetWorld(), GetActorLocation(),
			RLBalance::ScaledManaReward(ExcessManaReward, Difficulty));
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

	// First kill of this enemy type: drop the shard that unlocks its essence.
	// Uncollected shards are no loss — the check re-drops on the next kill.
	if (EnemyTypeId != NAME_None)
	{
		URLGameInstance* RLGI = Cast<URLGameInstance>(GI);
		URLDataSubsystem* Data = GI->GetSubsystem<URLDataSubsystem>();
		FName EssenceId = NAME_None;
		if (RLGI && Data && Data->FindEssenceForEnemy(EnemyTypeId, EssenceId)
			&& !RLGI->IsEssenceUnlocked(EssenceId))
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ARLEssenceShardPickup* Shard = GetWorld()->SpawnActor<ARLEssenceShardPickup>(
				ARLEssenceShardPickup::StaticClass(), GetActorLocation() + FVector(0.f, 0.f, 80.f),
				FRotator::ZeroRotator, Params))
			{
				Shard->EssenceId = EssenceId;
			}
		}
	}
}
