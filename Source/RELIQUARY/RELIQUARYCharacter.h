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
class URLCharacterPanelWidget;
class URLPauseMenuWidget;
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

	/** Fifth slot: the active granted by a major-slot essence (default Q). */
	UPROPERTY(EditAnywhere, Category="Input|Abilities")
	UInputAction* EssenceAbilityAction;

	/** Use altars, crates, forges */
	UPROPERTY(EditAnywhere, Category="Input|Abilities")
	UInputAction* InteractAction;

	/** Toggle the character sheet (default C). */
	UPROPERTY(EditAnywhere, Category="Input|UI")
	UInputAction* ToggleCharacterPanelAction;

	/**
	 * Open/close the pause menu (default Esc, plus P for PIE). The Input
	 * Action asset must have "Trigger When Paused" enabled to close again.
	 */
	UPROPERTY(EditAnywhere, Category="Input|UI")
	UInputAction* PauseMenuAction;

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

	/**
	 * When false, the next primary press fires OnWeaponDrawRequested instead
	 * of attacking. Implement that event in BP to play the draw montage,
	 * attach the weapon, and set this true; sheathing sets it false again.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Weapon")
	bool bWeaponDrawn = true;

	/** BP hook: the player pressed attack while the weapon was sheathed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Weapon")
	void OnWeaponDrawRequested();

	// --- Battle Trance (warrior passive; upgraded by the War_K3 keystone) ---

	/**
	 * 0..1: how deep into Battle Trance the hero is. Zero above the health
	 * threshold, ramping toward 1 as health approaches death. Drive UI
	 * feedback (vignette, glow) from this.
	 */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|BattleTrance")
	float GetBattleTranceIntensity() const;

	float GetTranceDamageReduction() const;
	float GetTranceCooldownReduction() const;
	float GetTranceHasteBonus() const;
	float GetTranceHealingBonus() const;
	float GetTranceSpeedBonus() const;

	// --- Sprint ---

	/**
	 * True while the sprint key is held (set from BP input). Tick folds it
	 * into MaxWalkSpeed together with the MoveSpeed attribute and Battle
	 * Trance, so nothing stomps anything.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Movement")
	bool bSprinting = false;

	/** Sprint speed as a multiple of base move speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Movement")
	float SprintSpeedMultiplier = 2.f;

	/**
	 * Short-lived walk-speed multiplier (ability recoils, debuff slows).
	 * Folded into the same tick-driven formula as sprint and Battle Trance,
	 * so nothing stomps anything. Reapplying overwrites the previous one.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Movement")
	void ApplyTemporarySpeedMultiplier(float Multiplier, float Seconds);

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
	void ReleaseKitAbility(FGameplayTag ActionTag);
	void OnPrimaryAbility();
	void OnSecondaryAbility();
	void OnUtilityAbility();
	void OnSpecialAbility();
	void OnEssenceAbility();
	void OnPrimaryAbilityReleased();
	void OnSecondaryAbilityReleased();
	void OnUtilityAbilityReleased();
	void OnSpecialAbilityReleased();
	void OnEssenceAbilityReleased();
	void OnInteract();
	void OnToggleCharacterPanel();
	void OnTogglePauseMenu();

	/** Character sheet widget (created lazily on first toggle). */
	UPROPERTY(EditAnywhere, Category="RELIQUARY|UI")
	TSubclassOf<URLCharacterPanelWidget> CharacterPanelWidgetClass;

	UPROPERTY()
	TObjectPtr<URLCharacterPanelWidget> CharacterPanel;

	bool bCharacterPanelOpen = false;

	/** Pause menu widget (created lazily on first open). */
	UPROPERTY(EditAnywhere, Category="RELIQUARY|UI")
	TSubclassOf<URLPauseMenuWidget> PauseMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<URLPauseMenuWidget> PauseMenu;

	UFUNCTION()
	void HandleDeath();

	void HandleMoveSpeedChanged(const FOnAttributeChangeData& Data);
	void FinishDeath();

	FTimerHandle DeathTimerHandle;
	bool bDying = false;

	// Cached from the active hero on RefreshHeroBuild.
	bool bHasBattleTrance = false;
	bool bImprovedTrance = false;

	// Temporary slow/boost state consumed by Tick's walk-speed formula.
	float TempSpeedMultiplier = 1.f;
	double TempSpeedUntilSeconds = 0.0;

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
