// RELIQUARY — goblins are brave only in numbers. A gang idles in a ritual
// circle facing its own center and periodically ambles somewhere new; a lone
// goblin wanders the map looking for another gang to join, and gangs that
// meet merge into one. When the hero closes in, the response depends purely
// on headcount: a lone goblin bolts, a small gang (2-3) stands and fights
// near its camp, and a mob of four or more hunts the hero down. Whittle any
// gang to its last goblin and that goblin breaks and runs.

#pragma once

#include "CoreMinimal.h"
#include "RLEnemyGroup.h"
#include "RLGoblinGang.generated.h"

UCLASS()
class RELIQUARY_API ARLGoblinGang : public ARLEnemyGroup
{
	GENERATED_BODY()

public:
	// --- Perception & courage thresholds ---

	/** Hero distance to the nearest goblin that provokes a reaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float AlertRange = 1500.f;

	/** Headcount at which the gang turns from standing its ground to hunting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	int32 HuntThreshold = 4;

	/** Seconds after a hit during which the gang stays angry past any range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float DamageAlertSeconds = 6.f;

	// --- Fight/Hunt leashes ---

	/** Fighting gangs (2-3) give up when the hero gets this far from camp. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float FightLeashRange = 2800.f;

	/** Hunting mobs (4+) give up when the hero shakes every goblin by this much. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float HuntGiveUpRange = 5000.f;

	// --- Fleeing (lone spawn / last survivor) ---

	/** A fleeing goblin calms down once the hero is this far away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float FleeSafeRange = 3200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float FleeSpeedScale = 1.2f;

	/** How far ahead the runner projects each escape point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float FleeStepDistance = 1400.f;

	// --- The circle & roaming ---

	/** Radius of the standing circle (goblins face its center). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float CircleRadius = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float SlotTolerance = 110.f;

	/** Seconds a gang stands in its circle before ambling somewhere new. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float CircleDwellMin = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float CircleDwellMax = 18.f;

	/** Amble speed (× MoveSpeed) — goblins roam slowly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float RoamSpeedScale = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float RoamTargetRangeMin = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float RoamTargetRangeMax = 2800.f;

	// --- Socializing ---

	/** Calm gangs whose centers drift this close merge into one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float MergeRange = 500.f;

	/** How far a lone goblin can sense another gang to wander toward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|GoblinGang")
	float SeekRange = 8000.f;

	bool IsCalm() const { return State == EGangState::Circle || State == EGangState::Roam; }

protected:
	virtual void TickGroup(float DeltaSeconds) override;

	enum class EGangState : uint8
	{
		Circle,	// standing in the ritual circle at the camp anchor
		Roam,	// ambling toward a new camp (a lone goblin: seeking a gang)
		Fight,	// 2-3: engage, but stay leashed to the camp
		Hunt,	// 4+: run the hero down
		Flee	// exactly 1: run away
	};
	EGangState State = EGangState::Circle;

	FVector RoamTarget = FVector::ZeroVector;
	float CircleDwellTimer = 3.f;

	void EnterCircle(bool bReanchor);
	void EnterRoam(int32 AliveCount);
	void EnterFight();
	void EnterHunt();
	void EnterFlee();

	void TickCalm(float DeltaSeconds, const TArray<ARLEnemyBase*>& Alive, bool bThreatened);
	void TickFight(const TArray<ARLEnemyBase*>& Alive);
	void TickHunt(const TArray<ARLEnemyBase*>& Alive);
	void TickFlee(const TArray<ARLEnemyBase*>& Alive);

	/** Nearest other gang with survivors within MaxRange, else nullptr. */
	ARLGoblinGang* FindNearestGang(float MaxRange, float& OutDistance) const;

	/** Absorbs every survivor of Consumed into this gang, then destroys it. */
	void AbsorbGang(ARLGoblinGang* Consumed);

	/**
	 * Merge with any gang in range while calm. Returns true when THIS gang was
	 * consumed by the other side — the caller must stop touching state.
	 */
	bool TryMerge(const TArray<ARLEnemyBase*>& Alive);
};
