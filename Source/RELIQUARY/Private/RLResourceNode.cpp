#include "RLResourceNode.h"
#include "RELIQUARY.h"
#include "RLResourcePickup.h"
#include "RLManaOrbPickup.h"
#include "RLRunManagerSubsystem.h"
#include "RLTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"

ARLResourceNode::ARLResourceNode()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Mesh);

	PickupClass = ARLResourcePickup::StaticClass();
}

namespace
{
	/**
	 * Nodes left on the native PickupClass default get upgraded to the
	 * project's BP_ResourcePickup (the one with a visible mesh) when it
	 * exists, mirroring the BP_ManaOrb convention. Cached so a missing
	 * Blueprint doesn't re-attempt (and re-log) the load per node.
	 */
	UClass* ResolveDefaultPickupClass()
	{
		static TWeakObjectPtr<UClass> Cached;
		static bool bLoadFailed = false;

		if (Cached.IsValid())
		{
			return Cached.Get();
		}
		if (bLoadFailed)
		{
			return ARLResourcePickup::StaticClass();
		}

		UClass* Loaded = StaticLoadClass(ARLResourcePickup::StaticClass(), nullptr,
			TEXT("/Game/Resources/BP_ResourcePickup.BP_ResourcePickup_C"));
		Cached = Loaded;
		bLoadFailed = (Loaded == nullptr);
		return Loaded ? Loaded : ARLResourcePickup::StaticClass();
	}
}

void ARLResourceNode::BeginPlay()
{
	Super::BeginPlay();
	NodeHealth = MaxNodeHealth;

	// The native pickup is invisible (no mesh). If nobody chose a pickup
	// Blueprint explicitly, prefer the project's BP_ResourcePickup so drops
	// can actually be seen; an explicit choice always wins.
	if (PickupClass == ARLResourcePickup::StaticClass())
	{
		PickupClass = ResolveDefaultPickupClass();
	}
}

void ARLResourceNode::Destroyed()
{
	// Shatter() is the only sanctioned way out — it drops materials and mana
	// and sets bShattered before the lifespan removes the actor. Anything else
	// (a Blueprint DestroyActor, leftover prototype logic) silently eats the
	// yield, so shout about it.
	if (!bShattered && HasActorBegunPlay())
	{
		UE_LOG(LogRELIQUARY, Warning,
			TEXT("%s (%s) was destroyed without shattering — external logic (Blueprint DestroyActor?) removed it; no materials or mana dropped."),
			*GetName(), *GetClass()->GetName());
	}
	Super::Destroyed();
}

float ARLResourceNode::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (bShattered || DamageAmount <= 0.f)
	{
		return Applied;
	}

	NodeHealth -= DamageAmount;
	if (NodeHealth <= 0.f)
	{
		Shatter(DamageCauser);
	}
	else
	{
		OnChipped(NodeHealth / MaxNodeHealth);
	}
	return Applied;
}

void ARLResourceNode::Shatter(AActor* Breaker)
{
	bShattered = true;

	// Stop blocking immediately; the actor itself lingers for the shatter
	// presentation below.
	if (Mesh)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	SetActorEnableCollision(false);

	UWorld* World = GetWorld();
	if (World && (MaterialItemId == NAME_None || !PickupClass))
	{
		// A node that drops nothing is a wiring bug, not a design choice —
		// say so instead of failing silently.
		UE_LOG(LogRELIQUARY, Warning,
			TEXT("%s shattered but drops no materials (MaterialItemId=%s, PickupClass=%s)."),
			*GetName(), *MaterialItemId.ToString(), *GetNameSafe(*PickupClass));
	}
	if (World && MaterialItemId != NAME_None && PickupClass)
	{
		const int32 Yield = FMath::RandRange(MinYield, MaxYield);
		for (int32 i = 0; i < Yield; ++i)
		{
			const FVector Offset(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), 40.f);
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ARLResourcePickup* Pickup = World->SpawnActor<ARLResourcePickup>(
				PickupClass, GetActorLocation() + Offset, FRotator::ZeroRotator, Params))
			{
				Pickup->ItemId = MaterialItemId;
				Pickup->Count = 1;
			}
		}
	}

	// The realm feeds you: shattering the world bursts into mana orbs,
	// worth more the deeper and later the run has run.
	if (World && ExcessManaYield > 0)
	{
		URLRunManagerSubsystem* RunManager =
			World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
		const float Difficulty = RunManager ? RunManager->GetDifficultyCoefficient() : 1.f;
		ARLManaOrbPickup::SpawnBurst(World, GetActorLocation(),
			RLBalance::ScaledManaReward(ExcessManaYield, Difficulty));
	}

	OnShattered(Breaker);

	// The stock presentation spawns replacement props (falling trunk, stump,
	// gravel) from OnShattered, so the intact original disappears now — the
	// job the prototype's DestroyActor used to do.
	if (bHideMeshOnShatter)
	{
		SetActorHiddenInGame(true);
	}

	SetLifeSpan(FMath::Max(ShatterLifeSpan, 0.1f));
}
