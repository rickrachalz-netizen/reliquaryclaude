#include "RLManaOrbPickup.h"
#include "RLRunManagerSubsystem.h"
#include "Engine/World.h"

ARLManaOrbPickup::ARLManaOrbPickup()
{
	// Orbs chase a little harder than materials so mana doesn't litter the floor.
	MagnetRadius = 800.f;
}

void ARLManaOrbPickup::GrantTo(AActor* Collector)
{
	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>() : nullptr)
	{
		RunManager->AddExcessMana(ManaAmount);
	}
}

void ARLManaOrbPickup::SpawnBurst(UWorld* World, const FVector& Origin, int32 TotalMana)
{
	if (!World || TotalMana <= 0)
	{
		return;
	}

	const int32 Unit = FMath::Max(1, RLBalance::ManaOrbUnitValue);
	const int32 OrbCount = FMath::Max(1, FMath::CeilToInt(static_cast<float>(TotalMana) / Unit));

	int32 Remaining = TotalMana;
	for (int32 i = 0; i < OrbCount; ++i)
	{
		// Even split, with the leftover riding the final orb.
		const int32 ThisOrb = (i == OrbCount - 1) ? Remaining : FMath::Min(Unit, Remaining);
		Remaining -= ThisOrb;
		if (ThisOrb <= 0)
		{
			continue;
		}

		const FVector Offset(FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(-90.f, 90.f), 60.f);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ARLManaOrbPickup* Orb = World->SpawnActor<ARLManaOrbPickup>(
			ARLManaOrbPickup::StaticClass(), Origin + Offset, FRotator::ZeroRotator, Params))
		{
			Orb->ManaAmount = ThisOrb;
		}
	}
}
