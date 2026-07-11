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

namespace
{
	/**
	 * The orb class bursts default to: /Game/Resources/BP_ManaOrb if the
	 * project authored one (that's where the visuals live), else the native
	 * class. Cached so a missing Blueprint doesn't re-attempt (and re-log)
	 * the load on every kill.
	 */
	UClass* ResolveDefaultOrbClass()
	{
		static TWeakObjectPtr<UClass> Cached;
		static bool bLoadFailed = false;

		if (Cached.IsValid())
		{
			return Cached.Get();
		}
		if (bLoadFailed)
		{
			return ARLManaOrbPickup::StaticClass();
		}

		UClass* Loaded = StaticLoadClass(ARLManaOrbPickup::StaticClass(), nullptr,
			TEXT("/Game/Resources/BP_ManaOrb.BP_ManaOrb_C"));
		Cached = Loaded;
		bLoadFailed = (Loaded == nullptr);
		return Loaded ? Loaded : ARLManaOrbPickup::StaticClass();
	}
}

void ARLManaOrbPickup::SpawnBurst(UWorld* World, const FVector& Origin, int32 TotalMana,
	TSubclassOf<ARLManaOrbPickup> OrbClass)
{
	if (!World || TotalMana <= 0)
	{
		return;
	}

	UClass* ClassToSpawn = OrbClass ? *OrbClass : ResolveDefaultOrbClass();

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
		// Deferred spawn: stamp the payload before overlaps can fire (an orb
		// spawning inside the hero is collected during the spawn call).
		const FTransform SpawnTransform(FRotator::ZeroRotator, Origin + Offset);
		if (ARLManaOrbPickup* Orb = World->SpawnActorDeferred<ARLManaOrbPickup>(
			ClassToSpawn, SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Orb->ManaAmount = ThisOrb;
			Orb->FinishSpawning(SpawnTransform);
		}
	}
}
