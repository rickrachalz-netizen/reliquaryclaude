#include "RLMortalStrikeAbility.h"
#include "RLAttributeSet.h"
#include "RLEnemyBase.h"
#include "RLGameInstance.h"
#include "RLGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"

URLMortalStrikeAbility::URLMortalStrikeAbility()
{
	ActionTag = RLTags::Ability_Secondary;
	BaseDamage.Value = 36.f;

	// Narrower and heavier than the primary, on a real cooldown.
	Range = 200.f;
	Radius = 90.f;
	ComboCooldownSeconds = 8.f;
	FinisherMultiplier = 1.f;
	HitStopSeconds = 0.07f;
	KnockbackStrength = 150.f;
}

float URLMortalStrikeAbility::GetVictimDamageMultiplier(AActor* Victim, float BaseMultiplier)
{
	float Multiplier = Super::GetVictimDamageMultiplier(Victim, BaseMultiplier);

	if (UAbilitySystemComponent* VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim))
	{
		// Consume Exposed stacks from the primary finisher.
		const int32 Stacks = VictimASC->GetTagCount(RLTags::State_Exposed);
		if (Stacks > 0)
		{
			Multiplier *= 1.f + ExposedBonusPerStack * Stacks;
			VictimASC->RemoveLooseGameplayTag(RLTags::State_Exposed, Stacks);
		}

		// Execute the nearly-dead.
		const float MaxHealth = VictimASC->GetNumericAttribute(URLAttributeSet::GetMaxHealthAttribute());
		if (MaxHealth > 0.f &&
			VictimASC->GetNumericAttribute(URLAttributeSet::GetHealthAttribute()) / MaxHealth < ExecuteThresholdFraction)
		{
			Multiplier *= ExecuteMultiplier;
		}
	}

	// Bladestorm's final tick empowers the next Mortal Strike.
	if (UAbilitySystemComponent* SelfASC = GetAbilitySystemComponentFromActorInfo())
	{
		const int32 Empowered = SelfASC->GetTagCount(RLTags::State_BladestormEmpowered);
		if (Empowered > 0)
		{
			Multiplier *= EmpoweredMultiplier;
			SelfASC->RemoveLooseGameplayTag(RLTags::State_BladestormEmpowered, Empowered);
		}
	}

	return Multiplier;
}

void URLMortalStrikeAbility::NotifyVictimHit(AActor* Victim)
{
	Super::NotifyVictimHit(Victim);

	if (!Victim)
	{
		return;
	}

	// Mortal Wounds: healing-received cut, consumed by healing systems.
	if (UAbilitySystemComponent* VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim))
	{
		VictimASC->AddLooseGameplayTag(RLTags::State_MortalWounds);

		FTimerHandle Handle;
		TWeakObjectPtr<UAbilitySystemComponent> WeakASC = VictimASC;
		Victim->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakASC]()
		{
			if (WeakASC.IsValid())
			{
				WeakASC->RemoveLooseGameplayTag(RLTags::State_MortalWounds);
			}
		}), MortalWoundsSeconds, /*bLoop=*/false);
	}

	// Second Wind (War_T3A): the strike also stuns.
	if (StunSeconds > 0.f)
	{
		if (ARLEnemyBase* Enemy = Cast<ARLEnemyBase>(Victim))
		{
			const UWorld* World = GetWorld();
			URLGameInstance* GI = World ? Cast<URLGameInstance>(World->GetGameInstance()) : nullptr;
			if (GI && GI->GetTalentRank(FName(TEXT("War_T3A"))) > 0)
			{
				Enemy->ApplyStun(StunSeconds);
			}
		}
	}
}
