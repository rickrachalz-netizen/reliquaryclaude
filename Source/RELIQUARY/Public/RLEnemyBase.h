// RELIQUARY — base class for every hostile creature. Simple but lethal:
// stats scale with the run's difficulty coefficient, elites hit harder and
// drop Manalith Shards, and every kill feeds XP and excess mana back to the
// hero.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "RLTypes.h"
#include "RLEnemyBase.generated.h"

class URLAbilitySystemComponent;
class URLAttributeSet;
class UUserWidget;
class UWidgetComponent;
class URLDamageNumberWidget;
class ARLEnemyBase;
class ARLEnemyGroup;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRLEnemyKilledSignature, ARLEnemyBase*, Enemy, AActor*, Killer);

/** Orders a coordinating group (wolf pack, goblin gang) pins on a member. */
UENUM()
enum class ERLGroupOrder : uint8
{
	None,		// stand still, no facing requirement
	HoldFacing,	// stand still, turn toward OrderLocation
	MoveTo,		// path to OrderLocation at the given speed scale
	EngageHero	// the built-in chase-and-strike brain, aggro gate skipped
};

UCLASS()
class RELIQUARY_API ARLEnemyBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ARLEnemyBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// --- Baseline stats at difficulty 1.0 (scaled on spawn) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Enemy")
	float BaseHealth = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Enemy")
	float BaseDamage = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Enemy")
	float BaseArmor = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Enemy")
	float BaseMoveSpeed = 450.f;

	/**
	 * Spawn-card row name identifying this enemy's type (Wolf, OrcBruiser...).
	 * The director and challenge altar stamp it on spawn; set it in the
	 * defaults of any Blueprint placed by hand. Drives first-kill essence
	 * shard drops — NAME_None means no essence can drop.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Enemy")
	FName EnemyTypeId = NAME_None;

	// --- Rewards ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Rewards")
	int32 XPReward = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Rewards")
	int32 ExcessManaReward = RLBalance::BaseEnemyManaReward;

	/** Optional material drop (bosses: GodboneSliver, elites: ManalithShard). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Rewards")
	FName DropItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Rewards")
	int32 DropCount = 0;

	// --- Modifiers ---

	/** Elite: 3x health, 1.5x damage, guaranteed Manalith Shard drop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Enemy")
	bool bElite = false;

	/** Set by challenge altars: empowered bosses guard the way forward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Enemy")
	bool bEmpowered = false;

	// --- Built-in AI ---

	/**
	 * Simple chase-and-strike brain, on by default so every enemy works
	 * without a StateTree/BT. Untick on a Blueprint to drive it yourself.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|AI")
	bool bChaseHero = true;

	/** How far away the hero is noticed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|AI")
	float AggroRange = 8000.f;

	/** Touch-strike reach beyond this capsule's radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|AI")
	float AttackRange = 160.f;

	/** Seconds between touch strikes while in reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|AI")
	float AttackInterval = 1.2f;

	/** BP hook: play a swing montage/VFX on each built-in touch strike. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|AI")
	void OnTouchAttack(AActor* Target);

	// --- Health bar (RoR2-style: appears when hurt, hides when left alone) ---

	/** Widget above the head; defaults to the native URLEnemyHealthBarWidget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RELIQUARY|HealthBar")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	/** Seconds after the last hit before the bar hides again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|HealthBar")
	float HealthBarLingerSeconds = 5.f;

	/** Floating damage number spawned per hit; defaults to the native widget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RELIQUARY|Combat")
	TSubclassOf<URLDamageNumberWidget> DamageNumberWidgetClass;

	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Enemy")
	FRLEnemyKilledSignature OnEnemyKilled;

	// --- Group coordination (wolf packs, goblin gangs) ---

	/**
	 * The group steering this enemy. While set (and bChaseHero is on), the
	 * solo brain is suspended and the enemy executes group orders instead.
	 * Pass nullptr to release it back to the solo brain at attribute speed.
	 */
	void SetGroup(ARLEnemyGroup* InGroup);

	ARLEnemyGroup* GetGroup() const { return Group; }

	/**
	 * Latest order from the group; executed every Tick until replaced.
	 * FaceLocation (optional, ZeroVector = none): where to turn while idling
	 * inside a MoveTo order's acceptance ring (goblins face their circle).
	 */
	void SetGroupOrder(ERLGroupOrder InOrder, const FVector& InLocation,
		float InSpeedScale = 1.f, float InAcceptanceRadius = 100.f,
		const FVector& InFaceLocation = FVector::ZeroVector);

	/** One immediate strike (montage hook + damage), bypassing the interval. */
	void PerformTouchStrike(AActor* Target);

	/** Unscaled MoveSpeed attribute — the base group speed scales multiply. */
	float GetBaseMoveSpeed() const;

	/** Touch-strike range measured from the actor center (AttackRange + capsule). */
	float GetStrikeReach() const;

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Enemy")
	bool IsStunned() const;

	/** Basic strike through the shared damage pipeline; call from AI/anim. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Enemy")
	void DealTouchDamage(AActor* Target);

	/** Hard CC: freezes the built-in brain and pathing for Seconds. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Enemy")
	void ApplyStun(float Seconds);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Enemy")
	bool IsDead() const { return bDead; }

	/** BP hook for death VFX/dissolve before the ragdoll is cleaned up. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Enemy")
	void OnDied(AActor* Killer);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URLAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<URLAttributeSet> Attributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> HealthBarComponent;

	FTimerHandle HealthBarTimerHandle;

	/** Updates fraction/tint, shows the bar, and re-arms the linger timer. */
	void RefreshHealthBar();
	void HideHealthBar();

	bool bDead = false;

	float TimeSinceAttack = 0.f;
	float RepathTimer = 0.f;
	bool bNavMovement = false;
	/** Straight-line fallback is only legal on maps with no navmesh at all. */
	bool bBeelineAllowed = true;
	double StunnedUntilSeconds = 0.0;

	// --- Group order state ---

	UPROPERTY()
	TObjectPtr<ARLEnemyGroup> Group;

	ERLGroupOrder GroupOrder = ERLGroupOrder::None;
	FVector GroupOrderLocation = FVector::ZeroVector;
	FVector GroupOrderFace = FVector::ZeroVector;
	float GroupOrderSpeedScale = 1.f;
	float GroupOrderAcceptance = 100.f;

	/**
	 * Anti-stutter latches. Movement decisions brake the character every time
	 * they flip, so both boundaries are sticky: a MoveTo order stops inside
	 * its acceptance ring but only resumes well outside it, and melee holds
	 * on to the target slightly beyond the reach that first captured it.
	 */
	bool bOrderMoving = false;
	bool bInMeleeHold = false;

	/** Runs the current group order for one frame. */
	void ExecuteGroupOrder(float DeltaSeconds);

	/** The classic brain: close in and strike on the interval. No aggro gate. */
	void TickChaseAndStrike(APawn* Hero, float DeltaSeconds);

	/** Nav path toward a point when a navmesh exists, beeline otherwise. */
	void MoveTowardsLocation(const FVector& Target, float AcceptanceRadius, float DeltaSeconds);

	void StopMoving();
	void FaceLocation(const FVector& Target, float DeltaSeconds);

	// --- Stuck recovery (anti-molasses) ---
	// Wedged on an obstacle or another body, a chaser barely moves while
	// pushing at full throttle. Both movement paths funnel through this: it
	// watches actual displacement and breaks a wedge with a short
	// perpendicular sidestep followed by a fresh repath.

	float StuckTimer = 0.f;
	FVector StuckCheckOrigin = FVector::ZeroVector;
	double UnstickUntilSeconds = 0.0;
	FVector UnstickDirection = FVector::ZeroVector;
	bool bUnstickSideRight = false;

	/** Returns true while a sidestep is overriding normal movement. */
	bool TickUnstick(const FVector& Goal, float DeltaSeconds);

	UPROPERTY()
	TObjectPtr<AActor> LastDamager;

	UFUNCTION()
	void HandleDamageTaken(float Damage, AActor* InstigatorActor, bool bCritical);

	/** Pop a floating damage number above the head for this hit. */
	void SpawnDamageNumber(float Damage, bool bCritical);

	UFUNCTION()
	void HandleDeath();

	void GrantRewards(AActor* Killer);
};
