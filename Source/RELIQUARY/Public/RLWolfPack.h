// RELIQUARY — wolves hunt as a pack. A calm pack lopes around the map in a
// loose formation behind a virtual leader. A pack that finds the hero fans
// out into a surrounding ring and takes turns lunging in for a bite — each
// striker peels back out and slots into whatever gap the ring has left. If
// the hero runs, the ring simply travels with them (the lunges keep coming);
// a hero who genuinely breaks away sends the pack back to roaming.

#pragma once

#include "CoreMinimal.h"
#include "RLEnemyGroup.h"
#include "RLWolfPack.generated.h"

UCLASS()
class RELIQUARY_API ARLWolfPack : public ARLEnemyGroup
{
	GENERATED_BODY()

public:
	// --- Perception ---

	/** Hero distance to the nearest wolf that wakes the pack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float AggroRange = 2600.f;

	/** The pack loses the hero once every wolf is at least this far away... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LoseHeroRange = 4500.f;

	/** ...for this many unbroken seconds; then it returns to roaming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LoseHeroSeconds = 4.f;

	// --- Surround ---

	/** Radius of the ring the pack holds around the hero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float CircleRadius = 520.f;

	/** How far a wolf may drift from its slot before strafing back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float SlotTolerance = 150.f;

	/** Speed (× MoveSpeed) while circling — above 1 so the ring tracks a running hero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float EncircleSpeedScale = 1.25f;

	// --- Lunge (one wolf at a time) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LungeIntervalMin = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LungeIntervalMax = 3.f;

	/** Sprint speed (× MoveSpeed) of the lunging wolf. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LungeSpeedScale = 1.8f;

	/** A lunge that hasn't connected by now counts as a miss; back to the ring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LungeMaxSeconds = 2.2f;

	// --- Roam ---

	/** Trot speed (× MoveSpeed) while roaming the map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamSpeedScale = 0.5f;

	/** Formation spread around the virtual leader while traveling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamRingRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamTargetRangeMin = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamTargetRangeMax = 3500.f;

	/** Seconds the pack mills around after reaching a roam target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamDwellMin = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamDwellMax = 6.f;

	virtual void NotifyMemberDamaged(ARLEnemyBase* Member, AActor* InstigatorActor) override;

protected:
	virtual void TickGroup(float DeltaSeconds) override;

	enum class EPackState : uint8
	{
		Roam,
		Encircle
	};
	EPackState State = EPackState::Roam;

	FVector RoamTarget = FVector::ZeroVector;
	float RoamDwellTimer = 0.f;
	float LungeTimer = 0.f;
	float LungeElapsed = 0.f;
	float LoseHeroTimer = 0.f;
	int32 NextLungerIndex = -1;
	TWeakObjectPtr<ARLEnemyBase> Lunger;

	void EnterRoam();
	void EnterEncircle();
	void TickRoam(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive);
	void TickEncircle(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive);
};
