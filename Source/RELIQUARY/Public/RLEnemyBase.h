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
class ARLEnemyBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRLEnemyKilledSignature, ARLEnemyBase*, Enemy, AActor*, Killer);

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

	// --- Rewards ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Rewards")
	int32 XPReward = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Rewards")
	int32 ExcessManaReward = 5;

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

	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Enemy")
	FRLEnemyKilledSignature OnEnemyKilled;

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

	bool bDead = false;

	float TimeSinceAttack = 0.f;
	float RepathTimer = 0.f;
	bool bNavMovement = false;
	double StunnedUntilSeconds = 0.0;

	UPROPERTY()
	TObjectPtr<AActor> LastDamager;

	UFUNCTION()
	void HandleDamageTaken(float Damage, AActor* InstigatorActor);

	UFUNCTION()
	void HandleDeath();

	void GrantRewards(AActor* Killer);
};
