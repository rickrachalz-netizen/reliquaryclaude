// RELIQUARY — coordinated enemy behavior ("personality" AI, RoR2 style).
// A group is an invisible coordinator actor the director spawns alongside a
// pack spawn card's members. It owns the pack-level state machine (roam,
// circle up, surround, flee...) and steers members by pinning simple orders
// (move here / hold facing / engage the hero) that ARLEnemyBase executes in
// its own Tick. Enemies without a group keep the classic solo brain.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLXoshiro.h"
#include "RLEnemyGroup.generated.h"

class ARLEnemyBase;

UCLASS(Abstract)
class RELIQUARY_API ARLEnemyGroup : public AActor
{
	GENERATED_BODY()

public:
	ARLEnemyGroup();

	/** Decorrelates this group's behavior rolls (roam points, lunge jitter). */
	void InitSeed(uint64 Seed) { Rng.SeedFrom(Seed); }

	/** Adds a member and takes over its brain. Also used when gangs merge. */
	virtual void RegisterMember(ARLEnemyBase* Enemy);

	/** Called by a member taking damage: wakes the group past any aggro range. */
	virtual void NotifyMemberDamaged(ARLEnemyBase* Member, AActor* InstigatorActor);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Group")
	int32 GetAliveCount() const;

	/** Draw anchors, ring slots, and roam targets while tuning in PIE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Group")
	bool bDrawDebug = false;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Per-archetype state machine. Runs only while at least one member lives. */
	virtual void TickGroup(float DeltaSeconds) {}

	UPROPERTY()
	TArray<TObjectPtr<ARLEnemyBase>> Members;

	/** Behavior stream, seeded by the spawner — never FMath::Rand (project rule). */
	FRLXoshiro256 Rng;

	/** World seconds of the last hit any member took; drives damage alerts. */
	double LastDamagedSeconds = -1000.0;

	/** The point the group drifts around while calm (camp / virtual leader). */
	FVector AnchorLocation = FVector::ZeroVector;

	// --- Shared helpers ---

	void GetAliveMembers(TArray<ARLEnemyBase*>& Out) const;
	APawn* GetHero() const;

	/** Average of living members' locations; AnchorLocation when none. */
	FVector GetCentroid() const;

	/** Distance from the hero to the nearest living member; MAX_flt without either. */
	float GetMinHeroDistance() const;

	bool WasDamagedWithin(float Seconds) const;

	/** Random point Min..Max cm from Origin, snapped to navmesh or ground. */
	FVector PickWanderPoint(const FVector& Origin, float MinRange, float MaxRange);

	/** Snap a point to the navmesh, else trace to ground; input on failure. */
	FVector ProjectPoint(const FVector& Point) const;

	/**
	 * Even ring of slots around Center. Slot assignment is PERSISTENT: it is
	 * derived from bearings once (so nobody crosses anybody) and then kept
	 * until the roster actually changes — a death, a merge, or the excluded
	 * member (a lunging wolf) switching. Stable slots are what keep members
	 * from stuttering after goalposts recomputed every frame.
	 *
	 * PhaseDeltaRadians rotates the whole ring by that much this tick (pass
	 * OrbitSpeed * DeltaSeconds for a ring that circles Center). Members
	 * idling on their slot turn toward Center when bFaceCenterAtSlot.
	 */
	void OrderRing(const TArray<ARLEnemyBase*>& Alive, const FVector& Center, float Radius,
		float SpeedScale, float Tolerance, ARLEnemyBase* Exclude = nullptr,
		float PhaseDeltaRadians = 0.f, bool bFaceCenterAtSlot = true);

	/** Current ring assignment (see OrderRing). */
	TArray<TWeakObjectPtr<ARLEnemyBase>> RingMembers;
	TWeakObjectPtr<ARLEnemyBase> RingExclude;
	float RingPhase = 0.f;

	bool RingMatches(const TArray<ARLEnemyBase*>& Alive, ARLEnemyBase* Exclude) const;
	void AssignRing(const TArray<ARLEnemyBase*>& Alive, const FVector& Center, ARLEnemyBase* Exclude);

	/** Call on state changes that displace everyone; the next ring reassigns fresh. */
	void InvalidateRing()
	{
		RingMembers.Reset();
		RingExclude = nullptr;
	}

	void PruneMembers();
};
