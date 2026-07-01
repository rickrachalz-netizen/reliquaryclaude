// RELIQUARY — the challenge altar, spiritual sibling of the Risk of Rain 2
// teleporter. Activate it, survive the empowered boss while standing in the
// charge radius, then choose: extract home with the haul, or push deeper
// along the realm's path.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLInteractable.h"
#include "RLChallengeAltar.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class ARLEnemyBase;

UENUM(BlueprintType)
enum class ERLAltarState : uint8
{
	Dormant,	// waiting for the hero to begin the challenge
	Charging,	// boss alive and/or charge below 100%
	Charged,	// boss dead, fully charged: choice available
	Consumed	// choice made
};

UCLASS()
class RELIQUARY_API ARLChallengeAltar : public AActor, public IRLInteractable
{
	GENERATED_BODY()

public:
	ARLChallengeAltar();

	/** Seconds of in-radius standing to fully charge (before difficulty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Altar")
	float ChargeDuration = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Altar")
	float ChargeRadius = 1400.f;

	/** Fallback boss when the zone's boss card can't be loaded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Altar")
	TSubclassOf<ARLEnemyBase> FallbackBossClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> ChargeZone;

	// --- IRLInteractable ---
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	// --- Choice (also callable straight from the choice widget) ---
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Altar")
	void ChooseExtract();

	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Altar")
	void ChooseContinue();

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Altar")
	ERLAltarState GetAltarState() const { return AltarState; }

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Altar")
	float GetChargeFraction() const { return ChargeFraction; }

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Altar")
	bool IsBossAlive() const;

	// --- BP hooks ---
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Altar")
	void OnChallengeStarted(ARLEnemyBase* Boss);

	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Altar")
	void OnFullyCharged();

	/** Fired when the charged altar is used — open the extract/continue UI. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Altar")
	void OnChoiceRequested();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	ERLAltarState AltarState = ERLAltarState::Dormant;
	float ChargeFraction = 0.f;

	UPROPERTY()
	TObjectPtr<ARLEnemyBase> ChallengeBoss;

	void BeginChallenge(AActor* Interactor);
	void SpawnChallengeBoss(AActor* Interactor);

	UFUNCTION()
	void HandleBossKilled(ARLEnemyBase* Enemy, AActor* Killer);
};
