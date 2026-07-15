// RELIQUARY — wolves hunt as a pack. A calm pack is carefree and wild:
// it drifts across big stretches of map like horses on a plain, every wolf
// weaving on its own line inside a loose spread. A pack that finds the hero
// pads sideways into a slowly orbiting ring and darts in for bites — starting
// while the ring is still forming, and more boldly once it has closed. A hero
// who runs gets pursued in that same loose roaming spread, with lunges from
// behind and, every several seconds, one wolf bursting ahead to cut the hero
// off. Break far enough away and the pack goes back to roaming.

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

	/** Slot slack before a wolf strafes back into position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float SlotTolerance = 150.f;

	/** Speed (× MoveSpeed) while circling — above 1 so the ring tracks the hero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float EncircleSpeedScale = 1.25f;

	/**
	 * How far ahead of each wolf its ring slot sits (radians). The wolves
	 * chase slots they never quite catch, so the whole ring slowly orbits
	 * the hero; the orbit direction flips now and then.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float OrbitLead = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float OrbitFlipSecondsMin = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float OrbitFlipSecondsMax = 14.f;

	// --- Lunge (one wolf at a time) ---

	/** Seconds between darts while the ring is still forming up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LungeIntervalMin = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LungeIntervalMax = 3.f;

	/** Seconds between darts once the pack has the hero surrounded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LungeIntervalFormedMin = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LungeIntervalFormedMax = 1.6f;

	/** Sprint speed (× MoveSpeed) of the lunging wolf. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LungeSpeedScale = 1.8f;

	/** A lunge that hasn't connected by now counts as a miss; back to the ring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float LungeMaxSeconds = 2.2f;

	// --- Pursuit (the hero is running) ---

	/** Hero ground speed that reads as fleeing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float PursueTriggerSpeed = 420.f;

	/** Hero must keep that pace this long before the ring breaks into a chase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float PursueTriggerSeconds = 0.8f;

	/** Pack speed (× MoveSpeed) while chasing in the roaming spread. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float PursueSpeedScale = 1.3f;

	/** Hero speed below which the pack settles back into the ring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float PursueCalmSpeed = 330.f;

	/** Every so often one wolf bursts ahead to cut the running hero off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float InterceptIntervalMin = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float InterceptIntervalMax = 12.f;

	/** How far ahead of the hero (seconds of their velocity) the cutoff aims. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float InterceptLeadSeconds = 1.1f;

	/** Burst speed (× MoveSpeed) of the cutoff wolf. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float InterceptSpeedScale = 1.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float InterceptMaxSeconds = 3.f;

	// --- Roam (carefree and wild) ---

	/** Trot speed (× MoveSpeed) of the pack's drift across the map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamSpeedScale = 0.55f;

	/** Spread of the loose pack: each wolf weaves inside this radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamSpreadRadius = 750.f;

	/** Roam legs are long — open-plain wandering, not tight patrol loops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamTargetRangeMin = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamTargetRangeMax = 6000.f;

	/** Brief milling about between legs; wolves rarely sit still. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamDwellMin = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|WolfPack")
	float RoamDwellMax = 4.f;

	virtual void NotifyMemberDamaged(ARLEnemyBase* Member, AActor* InstigatorActor) override;

protected:
	virtual void TickGroup(float DeltaSeconds) override;

	enum class EPackState : uint8
	{
		Roam,
		Encircle,
		Pursue
	};
	EPackState State = EPackState::Roam;

	/** Per-wolf flavor: a drifting offset in the spread and a gait of its own. */
	struct FWolfStyle
	{
		FVector Offset = FVector::ZeroVector;
		float SpeedJitter = 1.f;
		double NextRerollSeconds = 0.0;
	};
	TMap<TWeakObjectPtr<ARLEnemyBase>, FWolfStyle> Styles;

	FVector RoamTarget = FVector::ZeroVector;
	float RoamDwellTimer = 0.f;
	float LungeTimer = 0.f;
	float LungeElapsed = 0.f;
	float LoseHeroTimer = 0.f;
	float OrbitFlipTimer = 0.f;
	float OrbitSign = 1.f;
	float PursueScoreSeconds = 0.f;
	float CalmScoreSeconds = 0.f;
	float InterceptTimer = 0.f;
	float InterceptElapsed = 0.f;
	bool bRingFormed = false;
	int32 NextLungerIndex = -1;
	TWeakObjectPtr<ARLEnemyBase> Lunger;
	TWeakObjectPtr<ARLEnemyBase> Interceptor;

	void EnterRoam();
	void EnterEncircle();
	void EnterPursue();
	void TickRoam(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive);
	void TickEncircle(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive);
	void TickPursue(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive);

	/** True when the hero is gone or shaken for long enough; enters Roam. */
	bool TickLoseHero(float DeltaSeconds, APawn* Hero);

	/** The loose weaving spread, centered anywhere (roam anchor / running hero). */
	void OrderLoosePack(const TArray<ARLEnemyBase*>& Alive, const FVector& Center,
		float SpreadScale, float BaseSpeedScale,
		ARLEnemyBase* ExcludeA = nullptr, ARLEnemyBase* ExcludeB = nullptr);

	/** Runs the take-turns dart-and-bite token against the hero. */
	void TickLungeToken(float DeltaSeconds, APawn* Hero, const TArray<ARLEnemyBase*>& Alive,
		float IntervalMin, float IntervalMax);
};
