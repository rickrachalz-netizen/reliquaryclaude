#include "RLResourcePickup.h"
#include "RELIQUARY.h"
#include "RLRunManagerSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ARLResourcePickup::ARLResourcePickup()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(60.f);
	Collision->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	SetRootComponent(Collision);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARLResourcePickup::BeginPlay()
{
	Super::BeginPlay();
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ARLResourcePickup::HandleOverlap);
}

void ARLResourcePickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Let the drop exist for a beat before it flies to the hero.
	if (GetGameTimeSinceCreation() < MagnetDelaySeconds)
	{
		return;
	}

	APawn* Hero = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Hero)
	{
		return;
	}

	const FVector ToHero = Hero->GetActorLocation() - GetActorLocation();
	const float Distance = ToHero.Size();
	if (Distance < MagnetRadius && Distance > 1.f)
	{
		AddActorWorldOffset(ToHero.GetSafeNormal() * MagnetSpeed * DeltaSeconds);
	}
}

void ARLResourcePickup::GrantTo(AActor* Collector)
{
	if (ItemId == NAME_None)
	{
		// A payload-less grant means someone spawned a pickup without stamping
		// it before overlaps could fire — the drop is silently lost otherwise.
		UE_LOG(LogRELIQUARY, Warning,
			TEXT("%s collected with no ItemId — the spawner never stamped its payload; nothing granted."),
			*GetName());
		return;
	}

	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>() : nullptr)
	{
		RunManager->AddRunResource(ItemId, Count);
	}
}

void ARLResourcePickup::HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	GrantTo(OtherActor);
	OnCollected(OtherActor);
	Destroy();
}
