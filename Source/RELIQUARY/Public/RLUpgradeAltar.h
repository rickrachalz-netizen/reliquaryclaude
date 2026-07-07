// RELIQUARY — an upgrade altar. Spend excess mana on one of three offered
// boons, each pushing power in a different direction. Only a set number of
// these appear per zone, and everything they grant dies at base camp.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLInteractable.h"
#include "RLUpgradeAltar.generated.h"

class UStaticMeshComponent;

UCLASS()
class RELIQUARY_API ARLUpgradeAltar : public AActor, public IRLInteractable
{
	GENERATED_BODY()

public:
	ARLUpgradeAltar();

	/** How many distinct boons this altar offers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Altar")
	int32 ChoiceCount = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// --- IRLInteractable ---
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** The boon row names currently on offer (rolled from the run seed). */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Altar")
	const TArray<FName>& GetOfferedBoons() const { return OfferedBoons; }

	/**
	 * Excess-mana price of the boon at ChoiceIndex — the boon's base cost
	 * scaled by the run's difficulty when this altar rolled, so the boon UI
	 * shows what PurchaseBoon will actually charge. -1 for an invalid index.
	 */
	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Altar")
	int32 GetOfferedPrice(int32 ChoiceIndex) const;

	/**
	 * Buys the boon at ChoiceIndex for the interacting hero. Spends excess
	 * mana; fails silently if it can't be afforded. Consumes the altar.
	 */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Altar")
	bool PurchaseBoon(AActor* Purchaser, int32 ChoiceIndex);

	UFUNCTION(BlueprintPure, Category = "RELIQUARY|Altar")
	bool IsConsumed() const { return bConsumed; }

	/** BP hook: open the three-choice widget. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Altar")
	void OnBoonsOffered(const TArray<FName>& BoonIds);

	/** BP hook: purchase feedback. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RELIQUARY|Altar")
	void OnBoonPurchased(FName BoonId);

	/** Seed used to roll offers; set by the zone scatter for determinism. */
	void SetOfferSeed(int32 Seed) { OfferSeed = Seed; }

protected:
	virtual void BeginPlay() override;

	TArray<FName> OfferedBoons;
	/** Per-offer price, difficulty-scaled at roll time (parallel to OfferedBoons). */
	TArray<int32> OfferedPrices;
	int32 OfferSeed = 0;
	bool bConsumed = false;
	bool bOffersRolled = false;

	void RollOffers();
};
