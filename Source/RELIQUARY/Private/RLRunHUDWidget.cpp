#include "RLRunHUDWidget.h"
#include "RLRunManagerSubsystem.h"
#include "RLGameInstance.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "RLAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

URLRunManagerSubsystem* URLRunHUDWidget::GetRunManager() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
}

URLGameInstance* URLRunHUDWidget::GetRLGameInstance() const
{
	return Cast<URLGameInstance>(GetGameInstance());
}

UAbilitySystemComponent* URLRunHUDWidget::GetHeroASC() const
{
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	return Pawn ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn) : nullptr;
}

FText URLRunHUDWidget::GetRunTimerText() const
{
	const URLRunManagerSubsystem* RunManager = GetRunManager();
	const int32 TotalSeconds = RunManager ? FMath::FloorToInt(RunManager->GetRunSeconds()) : 0;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), TotalSeconds / 60, TotalSeconds % 60));
}

FText URLRunHUDWidget::GetLocationName() const
{
	URLRunManagerSubsystem* RunManager = GetRunManager();
	UGameInstance* GI = GetGameInstance();
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;

	if (RunManager && Data && RunManager->IsOnRun())
	{
		if (const FRLZoneRow* Zone = Data->FindZone(RunManager->GetCurrentZoneIndex()))
		{
			return Zone->DisplayName;
		}
	}
	return NSLOCTEXT("RL", "BaseCamp", "Base Camp — The Fallen God");
}

int32 URLRunHUDWidget::GetExcessMana() const
{
	const URLRunManagerSubsystem* RunManager = GetRunManager();
	return RunManager ? RunManager->GetExcessMana() : 0;
}

int32 URLRunHUDWidget::GetRunResourceTotal() const
{
	const URLRunManagerSubsystem* RunManager = GetRunManager();
	return RunManager ? RunManager->GetRunInventoryTotal() : 0;
}

TArray<FRLItemStack> URLRunHUDWidget::GetRunInventory() const
{
	const URLRunManagerSubsystem* RunManager = GetRunManager();
	return RunManager ? RunManager->GetRunInventory() : TArray<FRLItemStack>();
}

float URLRunHUDWidget::GetHealthFraction() const
{
	if (UAbilitySystemComponent* ASC = GetHeroASC())
	{
		const float MaxHealth = ASC->GetNumericAttribute(URLAttributeSet::GetMaxHealthAttribute());
		if (MaxHealth > 0.f)
		{
			return ASC->GetNumericAttribute(URLAttributeSet::GetHealthAttribute()) / MaxHealth;
		}
	}
	return 0.f;
}

float URLRunHUDWidget::GetManaFraction() const
{
	if (UAbilitySystemComponent* ASC = GetHeroASC())
	{
		const float MaxMana = ASC->GetNumericAttribute(URLAttributeSet::GetMaxManaAttribute());
		if (MaxMana > 0.f)
		{
			return ASC->GetNumericAttribute(URLAttributeSet::GetManaAttribute()) / MaxMana;
		}
	}
	return 0.f;
}

FText URLRunHUDWidget::GetHeroTitle() const
{
	if (URLGameInstance* GI = GetRLGameInstance())
	{
		const FRLHeroData Hero = GI->GetActiveHeroData();
		return FText::Format(NSLOCTEXT("RL", "HeroTitle", "{0} — Level {1} {2}"),
			FText::FromString(Hero.HeroName), FText::AsNumber(Hero.Level), GI->GetDisplayedClassName());
	}
	return FText::GetEmpty();
}

FText URLRunHUDWidget::GetDangerLabel() const
{
	const URLRunManagerSubsystem* RunManager = GetRunManager();
	const float Difficulty = RunManager ? RunManager->GetDifficultyCoefficient() : 1.f;

	if (Difficulty < 1.5f) { return NSLOCTEXT("RL", "Danger1", "Quiet"); }
	if (Difficulty < 2.5f) { return NSLOCTEXT("RL", "Danger2", "Stirring"); }
	if (Difficulty < 4.f)  { return NSLOCTEXT("RL", "Danger3", "Restless"); }
	if (Difficulty < 6.f)  { return NSLOCTEXT("RL", "Danger4", "Hostile"); }
	if (Difficulty < 9.f)  { return NSLOCTEXT("RL", "Danger5", "Furious"); }
	return NSLOCTEXT("RL", "Danger6", "THE REALM WAKES");
}
