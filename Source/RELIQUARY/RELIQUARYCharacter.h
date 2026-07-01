// RELIQUARY — the player-created hero pawn. Movement stays fast and fluid
// (Risk of Rain 2 feel); everything stat-shaped flows through GAS.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "RELIQUARYCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class URLAbilitySystemComponent;
class URLAttributeSet;
class URLProgressionComponent;
class URLEquipmentComponent;
class URLRunPowerComponent;
struct FInputActionValue;
struct FOnAttributeChangeData;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A persistent hero on an expedition into the realm of the Wild Gods.
 *  Third-person, dynamic camera, four-slot ability kit.
 */
UCLASS(abstract)
class ARELIQUARYCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Ability kit inputs (RoR2-style four slots) */
	UPROPERTY(EditAnywhere, Category="Input|Abilities")
	UInputAction* PrimaryAbilityAction;

	UPROPERTY(EditAnywhere, Category="Input|Abilities")
	UInputAction* SecondaryAbilityAction;

	UPROPERTY(EditAnywhere, Category="Input|Abilities")
	UInputAction* UtilityAbilityAction;

	UPROPERTY(EditAnywhere, Category="Input|Abilities")
	UInputAction* SpecialAbilityAction;

	/** Use altars, crates, forges */
	UPROPERTY(EditAnywhere, Category="Input|Abilities")
	UInputAction* InteractAction;

public:

	/** Constructor */
	ARELIQUARYCharacter();

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void PossessedBy(AController* NewController) override;

	/** Rebuilds permanent power + gear + run boons onto the ASC. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Hero")
	void RefreshHeroBuild();

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Hero")
	URLProgressionComponent* GetProgression() const { return Progression; }

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Hero")
	URLEquipmentComponent* GetEquipment() const { return Equipment; }

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Hero")
	URLRunPowerComponent* GetRunPower() const { return RunPower; }

	/** BP hook for death feedback before respawn at base camp. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Hero")
	void OnHeroDied();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URLAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<URLAttributeSet> Attributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URLProgressionComponent> Progression;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URLEquipmentComponent> Equipment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URLRunPowerComponent> RunPower;

	/** Interaction reach from the camera-facing direction, in cm. */
	UPROPERTY(EditAnywhere, Category = "RELIQUARY|Interaction")
	float InteractRange = 350.f;

	void ActivateKitAbility(FGameplayTag ActionTag);
	void OnPrimaryAbility();
	void OnSecondaryAbility();
	void OnUtilityAbility();
	void OnSpecialAbility();
	void OnInteract();

	UFUNCTION()
	void HandleDeath();

	void HandleMoveSpeedChanged(const FOnAttributeChangeData& Data);
	void FinishDeath();

	FTimerHandle DeathTimerHandle;
	bool bDying = false;

protected:
	FVector2D RawMoveInput;
	FVector2D SmoothedMoveInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float InputSmoothSpeed = 8.0f;


	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called when movement input stops */
	void StopMove(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:
	virtual void Tick(float DeltaSeconds) override;

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
