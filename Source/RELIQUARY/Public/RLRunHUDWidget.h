// RELIQUARY — C++ base for the run HUD. Minimalist by design: run timer,
// vitals, location, excess mana, and gathered resources. Reparent
// WBP_RunHUD to this class and bind the getters.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLTypes.h"
#include "RLRunHUDWidget.generated.h"

UCLASS(Abstract)
class RELIQUARY_API URLRunHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Run timer as MM:SS. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|HUD")
	FText GetRunTimerText() const;

	/** Current zone display name ("The Weeping Glade"), or Base Camp. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|HUD")
	FText GetLocationName() const;

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|HUD")
	int32 GetExcessMana() const;

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|HUD")
	int32 GetRunResourceTotal() const;

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|HUD")
	TArray<FRLItemStack> GetRunInventory() const;

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|HUD")
	float GetHealthFraction() const;

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|HUD")
	float GetManaFraction() const;

	/** Hero level and (possibly evolved) class title for the nameplate. */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|HUD")
	FText GetHeroTitle() const;

	/** RoR2-style danger readout ("Easy" ... "HAHAHAHA"). */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|HUD")
	FText GetDangerLabel() const;

protected:
	class URLRunManagerSubsystem* GetRunManager() const;
	class URLGameInstance* GetRLGameInstance() const;
	class UAbilitySystemComponent* GetHeroASC() const;
};
