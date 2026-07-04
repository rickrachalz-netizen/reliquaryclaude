#include "RELIQUARYCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "RELIQUARY.h"
#include "RLAbilitySystemComponent.h"
#include "RLAttributeSet.h"
#include "RLGameplayTags.h"
#include "RLProgressionComponent.h"
#include "RLEquipmentComponent.h"
#include "RLRunPowerComponent.h"
#include "RLRunManagerSubsystem.h"
#include "RLGameInstance.h"
#include "RLInteractable.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ARELIQUARYCharacter::ARELIQUARYCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// GAS: the hero's ability system and full stat suite.
	AbilitySystemComponent = CreateDefaultSubobject<URLAbilitySystemComponent>(TEXT("ASC"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	Attributes = CreateDefaultSubobject<URLAttributeSet>(TEXT("Attributes"));

	// Build assembly: permanent power, gear, and temporary run boons.
	Progression = CreateDefaultSubobject<URLProgressionComponent>(TEXT("Progression"));
	Equipment = CreateDefaultSubobject<URLEquipmentComponent>(TEXT("Equipment"));
	RunPower = CreateDefaultSubobject<URLRunPowerComponent>(TEXT("RunPower"));
}

UAbilitySystemComponent* ARELIQUARYCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ARELIQUARYCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARELIQUARYCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ARELIQUARYCharacter::StopMove);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ARELIQUARYCharacter::StopMove);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ARELIQUARYCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARELIQUARYCharacter::Look);

		// Ability kit
		if (PrimaryAbilityAction)
		{
			EnhancedInputComponent->BindAction(PrimaryAbilityAction, ETriggerEvent::Started, this, &ARELIQUARYCharacter::OnPrimaryAbility);
		}
		if (SecondaryAbilityAction)
		{
			EnhancedInputComponent->BindAction(SecondaryAbilityAction, ETriggerEvent::Started, this, &ARELIQUARYCharacter::OnSecondaryAbility);
		}
		if (UtilityAbilityAction)
		{
			EnhancedInputComponent->BindAction(UtilityAbilityAction, ETriggerEvent::Started, this, &ARELIQUARYCharacter::OnUtilityAbility);
		}
		if (SpecialAbilityAction)
		{
			EnhancedInputComponent->BindAction(SpecialAbilityAction, ETriggerEvent::Started, this, &ARELIQUARYCharacter::OnSpecialAbility);
		}
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ARELIQUARYCharacter::OnInteract);
		}
	}
	else
	{
		UE_LOG(LogRELIQUARY, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ARELIQUARYCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	RawMoveInput = Value.Get<FVector2D>();
}

void ARELIQUARYCharacter::StopMove(const FInputActionValue& Value)
{
	RawMoveInput = FVector2D::ZeroVector;
}

void ARELIQUARYCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ARELIQUARYCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ARELIQUARYCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ARELIQUARYCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ARELIQUARYCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void ARELIQUARYCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->OnDeath.AddDynamic(this, &ARELIQUARYCharacter::HandleDeath);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(URLAttributeSet::GetMoveSpeedAttribute())
		.AddUObject(this, &ARELIQUARYCharacter::HandleMoveSpeedChanged);

	RefreshHeroBuild();
}

void ARELIQUARYCharacter::RefreshHeroBuild()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Order matters: base identity first, then gear on top, then any
	// temporary run power recorded before this map loaded.
	Progression->RebuildHero(AbilitySystemComponent);
	Equipment->RefreshEquipment(AbilitySystemComponent);
	RunPower->RestoreFromRunManager(AbilitySystemComponent);

	// Battle Trance is the warrior's identity passive; War_K3 upgrades it.
	bHasBattleTrance = false;
	bImprovedTrance = false;
	if (URLGameInstance* GI = Cast<URLGameInstance>(GetWorld()->GetGameInstance()))
	{
		if (const FRLHeroData* Hero = GI->GetActiveHero())
		{
			bHasBattleTrance = Hero->HeroClass == ERLHeroClass::Warrior;
			bImprovedTrance = bHasBattleTrance && GI->GetTalentRank(FName(TEXT("War_K3"))) > 0;
		}
	}

	GetCharacterMovement()->MaxWalkSpeed =
		AbilitySystemComponent->GetNumericAttribute(URLAttributeSet::GetMoveSpeedAttribute());
}

// --- Battle Trance ---

namespace RLTrance
{
	// Normal / Improved (War_K3) values, straight from the design sheet.
	constexpr float Threshold[2]  = { 0.75f, 0.90f };
	constexpr float MaxDR[2]      = { 0.30f, 0.50f };
	constexpr float MaxCDR[2]     = { 0.90f, 0.95f };
	constexpr float MaxHealing[2] = { 1.00f, 1.50f };
	constexpr float MaxHaste[2]   = { 0.10f, 0.30f };
	constexpr float MaxSpeed[2]   = { 0.30f, 0.50f };

	// Ramp: ~zero at the threshold, full effect approaching 1% health.
	constexpr float FloorFraction = 0.01f;
	constexpr float Exponent = 3.f;
}

float ARELIQUARYCharacter::GetBattleTranceIntensity() const
{
	if (!bHasBattleTrance || bDying || !Attributes)
	{
		return 0.f;
	}

	const float MaxHealth = Attributes->GetMaxHealth();
	if (MaxHealth <= 0.f)
	{
		return 0.f;
	}

	const int32 Set = bImprovedTrance ? 1 : 0;
	const float Fraction = Attributes->GetHealth() / MaxHealth;
	if (Fraction >= RLTrance::Threshold[Set])
	{
		return 0.f;
	}

	const float T = FMath::Clamp(
		(RLTrance::Threshold[Set] - Fraction) / (RLTrance::Threshold[Set] - RLTrance::FloorFraction),
		0.f, 1.f);
	return FMath::Pow(T, RLTrance::Exponent);
}

float ARELIQUARYCharacter::GetTranceDamageReduction() const
{
	return GetBattleTranceIntensity() * RLTrance::MaxDR[bImprovedTrance ? 1 : 0];
}

float ARELIQUARYCharacter::GetTranceCooldownReduction() const
{
	return GetBattleTranceIntensity() * RLTrance::MaxCDR[bImprovedTrance ? 1 : 0];
}

float ARELIQUARYCharacter::GetTranceHasteBonus() const
{
	return GetBattleTranceIntensity() * RLTrance::MaxHaste[bImprovedTrance ? 1 : 0];
}

float ARELIQUARYCharacter::GetTranceHealingBonus() const
{
	return GetBattleTranceIntensity() * RLTrance::MaxHealing[bImprovedTrance ? 1 : 0];
}

float ARELIQUARYCharacter::GetTranceSpeedBonus() const
{
	return GetBattleTranceIntensity() * RLTrance::MaxSpeed[bImprovedTrance ? 1 : 0];
}

void ARELIQUARYCharacter::Tick(float Dt)
{
	Super::Tick(Dt);

	SmoothedMoveInput = FMath::Vector2DInterpTo(SmoothedMoveInput, RawMoveInput, Dt, InputSmoothSpeed);

	if (!SmoothedMoveInput.IsNearlyZero())
	{
		DoMove(SmoothedMoveInput.X, SmoothedMoveInput.Y);
	}

	// Passive regeneration, driven by the HealthRegen/ManaRegen attributes.
	// Battle Trance boosts healing received and quickens the step near death.
	if (AbilitySystemComponent && Attributes && !bDying)
	{
		const float HealthRegen = Attributes->GetHealthRegen();
		if (HealthRegen != 0.f && Attributes->GetHealth() < Attributes->GetMaxHealth())
		{
			AbilitySystemComponent->ApplyModToAttribute(
				URLAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive,
				HealthRegen * (1.f + GetTranceHealingBonus()) * Dt);
		}

		GetCharacterMovement()->MaxWalkSpeed =
			Attributes->GetMoveSpeed() * (1.f + GetTranceSpeedBonus());
		const float ManaRegen = Attributes->GetManaRegen();
		if (ManaRegen != 0.f && Attributes->GetMana() < Attributes->GetMaxMana())
		{
			AbilitySystemComponent->ApplyModToAttribute(
				URLAttributeSet::GetManaAttribute(), EGameplayModOp::Additive, ManaRegen * Dt);
		}
	}
}

// --- Ability kit input ---

void ARELIQUARYCharacter::ActivateKitAbility(FGameplayTag ActionTag)
{
	if (AbilitySystemComponent && !bDying)
	{
		AbilitySystemComponent->TryActivateByActionTag(ActionTag);
	}
}

void ARELIQUARYCharacter::OnPrimaryAbility()
{
	// Sheathed weapon: the press draws instead of swinging (BP plays the
	// draw montage and flips bWeaponDrawn).
	if (!bWeaponDrawn)
	{
		OnWeaponDrawRequested();
		return;
	}
	ActivateKitAbility(RLTags::Ability_Primary);
}

void ARELIQUARYCharacter::OnSecondaryAbility() { ActivateKitAbility(RLTags::Ability_Secondary); }
void ARELIQUARYCharacter::OnUtilityAbility()   { ActivateKitAbility(RLTags::Ability_Utility); }
void ARELIQUARYCharacter::OnSpecialAbility()   { ActivateKitAbility(RLTags::Ability_Special); }

void ARELIQUARYCharacter::OnInteract()
{
	if (bDying)
	{
		return;
	}

	// Closest interactable within reach that says yes.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RLInteract), false, this);
	GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity,
		ECC_WorldDynamic, FCollisionShape::MakeSphere(InteractRange), Params);

	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || !Candidate->Implements<URLInteractable>())
		{
			continue;
		}
		if (!IRLInteractable::Execute_CanInteract(Candidate, this))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Candidate->GetActorLocation(), GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}

	if (Best)
	{
		IRLInteractable::Execute_Interact(Best, this);
	}
}

// --- Attribute plumbing ---

void ARELIQUARYCharacter::HandleMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}

// --- Death: respawn at base camp, resources forfeit ---

void ARELIQUARYCharacter::HandleDeath()
{
	if (bDying)
	{
		return;
	}
	bDying = true;

	// Stop movement and input
	GetCharacterMovement()->DisableMovement();
	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

	// Turn the capsule off so it doesn't fight the ragdoll
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Ragdoll: let the mesh simulate physics
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();

	OnHeroDied();

	// A short beat to watch the fall, then home to the fallen god.
	GetWorldTimerManager().SetTimer(DeathTimerHandle, this, &ARELIQUARYCharacter::FinishDeath, 3.f, false);
}

void ARELIQUARYCharacter::FinishDeath()
{
	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>() : nullptr)
	{
		RunManager->HandleHeroDeath();
	}
}
