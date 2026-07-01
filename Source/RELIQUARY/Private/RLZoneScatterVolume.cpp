#include "RLZoneScatterVolume.h"
#include "RLResourceNode.h"
#include "RLUpgradeAltar.h"
#include "RLChallengeAltar.h"
#include "RLBankingCrate.h"
#include "RLRunManagerSubsystem.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogRLScatter, Log, All);

ARLZoneScatterVolume::ARLZoneScatterVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetBoxExtent(FVector(5000.f, 5000.f, 2000.f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(Bounds);

	DefaultNodeClass = ARLResourceNode::StaticClass();
	UpgradeAltarClass = ARLUpgradeAltar::StaticClass();
	ChallengeAltarClass = ARLChallengeAltar::StaticClass();
	BankingCrateClass = ARLBankingCrate::StaticClass();
}

void ARLZoneScatterVolume::BeginPlay()
{
	Super::BeginPlay();
	ScatterZone();
}

void ARLZoneScatterVolume::ScatterZone()
{
	UGameInstance* GI = GetGameInstance();
	URLRunManagerSubsystem* RunManager = GI ? GI->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;
	if (!RunManager || !Data || !RunManager->IsOnRun())
	{
		return;
	}

	const FRLZoneRow* Zone = Data->FindZone(RunManager->GetCurrentZoneIndex());
	if (!Zone)
	{
		UE_LOG(LogRLScatter, Warning, TEXT("No zone row for index %d; scattering nothing."),
			RunManager->GetCurrentZoneIndex());
		return;
	}

	FRandomStream Rng = RunManager->MakeZoneRandomStream();

	// Resource nodes: each zone reliably grows its planned materials.
	TArray<FName> Materials;
	Zone->GetMaterials(Materials);
	if (Materials.Num() > 0)
	{
		for (int32 i = 0; i < Zone->ResourceNodeCount; ++i)
		{
			const FName MaterialId = Materials[Rng.RandRange(0, Materials.Num() - 1)];
			TSubclassOf<ARLResourceNode> NodeClass = DefaultNodeClass;
			if (const TSubclassOf<ARLResourceNode>* Mapped = NodeClassByMaterial.Find(MaterialId))
			{
				if (*Mapped)
				{
					NodeClass = *Mapped;
				}
			}
			if (ARLResourceNode* Node = SpawnScattered<ARLResourceNode>(NodeClass, Rng))
			{
				Node->MaterialItemId = MaterialId;
			}
		}
	}

	// Upgrade altars: a fixed number per zone, offers rolled from the seed.
	for (int32 i = 0; i < Zone->UpgradeAltarCount; ++i)
	{
		if (ARLUpgradeAltar* Altar = SpawnScattered<ARLUpgradeAltar>(UpgradeAltarClass, Rng))
		{
			Altar->SetOfferSeed(Rng.RandRange(1, MAX_int32 - 1));
		}
	}

	// Exactly one challenge altar guards the way onward.
	SpawnScattered<ARLChallengeAltar>(ChallengeAltarClass, Rng);

	// A crate home every third cleared zone.
	if (RunManager->ShouldOfferBankingCrate())
	{
		SpawnScattered<ARLBankingCrate>(BankingCrateClass, Rng);
	}
}

bool ARLZoneScatterVolume::FindGroundPoint(FRandomStream& Rng, FVector& OutLocation) const
{
	const FVector Origin = Bounds->GetComponentLocation();
	const FVector Extent = Bounds->GetScaledBoxExtent();

	for (int32 Try = 0; Try < MaxPlacementTries; ++Try)
	{
		const FVector Start(
			Origin.X + Rng.FRandRange(-Extent.X, Extent.X),
			Origin.Y + Rng.FRandRange(-Extent.Y, Extent.Y),
			Origin.Z + Extent.Z);
		const FVector End = Start - FVector(0.f, 0.f, Extent.Z * 2.f);

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(RLScatterTrace), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params) && Hit.bBlockingHit)
		{
			OutLocation = Hit.ImpactPoint + FVector(0.f, 0.f, 10.f);
			return true;
		}
	}
	return false;
}
