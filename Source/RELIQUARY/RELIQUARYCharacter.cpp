// Copyright Epic Games, Inc. All Rights Reserved.

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
#include "AbilitySystemComponent.h"
#include "RLAttributeSet.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ARELIQUARYCharacter::ARELIQUARYCharacter()
{

	PrimaryActorTick.bCanEverTick = true;   // <-- add this line

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
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
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

	// In the constructor (anywhere after Super):
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	Attributes = CreateDefaultSubobject<URLAttributeSet>(TEXT("Attributes"));

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
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
	OnTakeAnyDamage.AddDynamic(this, &ARELIQUARYCharacter::HandleTakeAnyDamage);
	InitGAS();
}

void ARELIQUARYCharacter::InitGAS()
{
	// PoC shortcut: set starting values directly. We'll replace this with a
	// proper "default attributes" GameplayEffect once we add effects.
	Attributes->InitMaxHealth(100.f);
	Attributes->InitHealth(100.f);
	Attributes->InitStrength(10.f);
	Attributes->InitIntellect(10.f);
}

void ARELIQUARYCharacter::Tick(float Dt)
{
	Super::Tick(Dt);

	SmoothedMoveInput = FMath::Vector2DInterpTo(SmoothedMoveInput, RawMoveInput, Dt, InputSmoothSpeed);

	if (!SmoothedMoveInput.IsNearlyZero())
	{
		DoMove(SmoothedMoveInput.X, SmoothedMoveInput.Y);
	}

	if (AbilitySystemComponent && Attributes && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Green,
			FString::Printf(TEXT("HP %.0f/%.0f | STR %.0f | INT %.0f"),
				Attributes->GetHealth(), Attributes->GetMaxHealth(),
				Attributes->GetStrength(), Attributes->GetIntellect()));
	}
}

void ARELIQUARYCharacter::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || !Attributes)
	{
		return;
	}

	const float NewHealth = FMath::Clamp(Attributes->GetHealth() - Damage, 0.f, Attributes->GetMaxHealth());
	Attributes->SetHealth(NewHealth);

	if (NewHealth <= 0.f)
	{
		Die();
	}
}

void ARELIQUARYCharacter::Die()
{
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
}
