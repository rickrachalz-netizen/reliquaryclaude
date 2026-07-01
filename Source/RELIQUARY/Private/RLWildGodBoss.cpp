#include "RLWildGodBoss.h"
#include "RLAbilitySystemComponent.h"
#include "RLGameplayTags.h"
#include "RLRunManagerSubsystem.h"
#include "RLGameInstance.h"

ARLWildGodBoss::ARLWildGodBoss()
{
	// Tuned against a well-geared level-30 hero: with full Godbone gear,
	// deep talents, and ranked essences a hero fields roughly 200+ primary
	// stat and 1500+ health. These numbers make everyone else bounce off.
	BaseHealth = 25000.f;
	BaseDamage = 120.f;
	BaseArmor = 200.f;
	BaseMoveSpeed = 520.f;

	XPReward = 5000;
	ExcessManaReward = 0;
	DropItemId = FName(TEXT("WildGodIchor"));
	DropCount = 5;
}

void ARLWildGodBoss::BeginPlay()
{
	Super::BeginPlay();

	if (URLAbilitySystemComponent* RLASC = Cast<URLAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		RLASC->AddLooseGameplayTag(RLTags::Enemy_WildGod);
		RLASC->AddLooseGameplayTag(RLTags::Enemy_Boss);
	}

	OnEnemyKilled.AddDynamic(this, &ARLWildGodBoss::HandleWildGodKilled);
}

void ARLWildGodBoss::HandleWildGodKilled(ARLEnemyBase* Enemy, AActor* Killer)
{
	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>() : nullptr)
	{
		RunManager->HandleFinalBossKilled(/*bWildGod=*/true);
	}
	if (URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance()))
	{
		GI->SaveToDisk();	// the victory is permanent the moment it happens
	}
	OnWildGodDefeated(Killer);
}
