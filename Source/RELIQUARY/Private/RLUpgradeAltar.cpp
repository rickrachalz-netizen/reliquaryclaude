#include "RLUpgradeAltar.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "RLRunManagerSubsystem.h"
#include "RLRunPowerComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/StaticMeshComponent.h"

ARLUpgradeAltar::ARLUpgradeAltar()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Mesh);
}

void ARLUpgradeAltar::BeginPlay()
{
	Super::BeginPlay();
	if (OfferSeed == 0)
	{
		OfferSeed = static_cast<int32>(GetUniqueID());	// hand-placed altars still get offers
	}
}

void ARLUpgradeAltar::RollOffers()
{
	UGameInstance* GI = GetGameInstance();
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;
	URLRunManagerSubsystem* RunManager = GI ? GI->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
	if (!Data || !RunManager)
	{
		return;
	}

	FRandomStream Rng(OfferSeed);
	Data->DrawRandomBoons(ChoiceCount, RunManager->GetAllBoonStacks(), Rng, OfferedBoons);
	bOffersRolled = true;
}

bool ARLUpgradeAltar::CanInteract_Implementation(AActor* Interactor) const
{
	return !bConsumed;
}

FText ARLUpgradeAltar::GetInteractionPrompt_Implementation() const
{
	return bConsumed
		? NSLOCTEXT("RL", "AltarSpent", "The altar is spent.")
		: NSLOCTEXT("RL", "AltarOffer", "Commune with the Altar");
}

void ARLUpgradeAltar::Interact_Implementation(AActor* Interactor)
{
	if (bConsumed)
	{
		return;
	}
	if (!bOffersRolled)
	{
		RollOffers();
	}
	OnBoonsOffered(OfferedBoons);
}

bool ARLUpgradeAltar::PurchaseBoon(AActor* Purchaser, int32 ChoiceIndex)
{
	if (bConsumed || !Purchaser || !OfferedBoons.IsValidIndex(ChoiceIndex))
	{
		return false;
	}

	UGameInstance* GI = GetGameInstance();
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;
	URLRunManagerSubsystem* RunManager = GI ? GI->GetSubsystem<URLRunManagerSubsystem>() : nullptr;

	const FName BoonId = OfferedBoons[ChoiceIndex];
	const FRLBoonRow* Boon = Data ? Data->FindBoon(BoonId) : nullptr;
	URLRunPowerComponent* RunPower = Purchaser->FindComponentByClass<URLRunPowerComponent>();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Purchaser);
	if (!Boon || !RunPower || !ASC || !RunManager)
	{
		return false;
	}

	if (!RunManager->SpendExcessMana(Boon->ManaCost))
	{
		return false;
	}

	if (!RunPower->ApplyBoon(ASC, BoonId))
	{
		// Refund on the (unlikely) application failure.
		RunManager->AddExcessMana(Boon->ManaCost);
		return false;
	}

	bConsumed = true;
	OnBoonPurchased(BoonId);
	return true;
}
