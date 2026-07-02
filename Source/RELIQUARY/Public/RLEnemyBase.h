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

	/** Attack-power coefficient for the basic touch strike. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Enemy")
	float TouchAttackCoefficient = 0.5f;

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

	UPROPERTY(BlueprintAssignable, Category = "RELIQUARY|Enemy")
	FRLEnemyKilledSignature OnEnemyKilled;

	/** Basic strike through the shared damage pipeline; call from AI/anim. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Enemy")
	void DealTouchDamage(AActor* Target);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Enemy")
	bool IsDead() const { return bDead; }

	/** RoR2 ambient monster level this enemy spawned at. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Enemy")
	float GetScaledLevel() const { return ScaledLevel; }

	/** BP hook for death VFX/dissolve before the ragdoll is cleaned up. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Enemy")
	void OnDied(AActor* Killer);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URLAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<URLAttributeSet> Attributes;

	bool bDead = false;
	float ScaledLevel = 1.f;

	UPROPERTY()
	TObjectPtr<AActor> LastDamager;

	UFUNCTION()
	void HandleDamageTaken(float Damage, AActor* InstigatorActor);

	UFUNCTION()
	void HandleDeath();

	void GrantRewards(AActor* Killer);
};
