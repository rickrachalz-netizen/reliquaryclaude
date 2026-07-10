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

void ARLResourceNode::BeginPlay()
{
	Super::BeginPlay();
	NodeHealth = MaxNodeHealth;
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
	SetLifeSpan(FMath::Max(ShatterLifeSpan, 0.1f));
}
