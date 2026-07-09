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

	// Out-of-the-box authoring for L_Run: a clustered Oakheart forest and
	// scattered Duskiron ore. Assign each entry's NodeClass to your BP_Tree /
	// BP_Stone and tune the counts on the placed volume.
	FRLResourceScatterEntry Trees;
	Trees.MaterialItemId = FName(TEXT("OakheartTimber"));
	Trees.Distribution = ERLScatterDistribution::Clustered;
	Trees.MinCount = 4;
	Trees.MaxCount = 7;
	Trees.MinPerCluster = 3;
	Trees.MaxPerCluster = 6;
	Trees.ClusterRadius = 700.f;
	ResourceScatter.Add(Trees);

	FRLResourceScatterEntry Ore;
	Ore.MaterialItemId = FName(TEXT("DuskironOre"));
	Ore.Distribution = ERLScatterDistribution::Uniform;
	Ore.MinCount = 6;
	Ore.MaxCount = 10;
	ResourceScatter.Add(Ore);
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

	FRLXoshiro256 Rng = RunManager->MakeZoneRng();

	// Resource nodes. When the map authors a ResourceScatter list, it is the
	// sole authority over what grows and how; otherwise fall back to the legacy
	// per-zone material roll so other maps/zones are unchanged.
	if (ResourceScatter.Num() > 0)
	{
		for (const FRLResourceScatterEntry& Entry : ResourceScatter)
		{
			ScatterResourceEntry(Entry, Rng);
		}
	}
	else
	{
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

void ARLZoneScatterVolume::ScatterResourceEntry(const FRLResourceScatterEntry& Entry, FRLXoshiro256& Rng)
{
	// Resolve the node class like the legacy path: an explicit entry class
	// wins, then the per-material map, then the volume default — so a fresh
	// volume still spawns harvestable nodes before any BPs are assigned.
	TSubclassOf<ARLResourceNode> NodeClass = Entry.NodeClass;
	if (!NodeClass)
	{
		if (const TSubclassOf<ARLResourceNode>* Mapped = NodeClassByMaterial.Find(Entry.MaterialItemId))
		{
			NodeClass = *Mapped;
		}
	}
	if (!NodeClass)
	{
		NodeClass = DefaultNodeClass;
	}
	if (!NodeClass)
	{
		return;
	}

	const int32 Count = Rng.RandRange(FMath::Min(Entry.MinCount, Entry.MaxCount),
		FMath::Max(Entry.MinCount, Entry.MaxCount));

	if (Entry.Distribution == ERLScatterDistribution::Uniform)
	{
		for (int32 i = 0; i < Count; ++i)
		{
			if (ARLResourceNode* Node = SpawnScattered<ARLResourceNode>(NodeClass, Rng))
			{
				Node->MaterialItemId = Entry.MaterialItemId;
			}
		}
		return;
	}

	// Clustered: seed a few cluster centers, then clump several nodes around each.
	for (int32 c = 0; c < Count; ++c)
	{
		FVector Center;
		if (!FindGroundPoint(Rng, Center))
		{
			continue;
		}

		const int32 PerCluster = Rng.RandRange(FMath::Min(Entry.MinPerCluster, Entry.MaxPerCluster),
			FMath::Max(Entry.MinPerCluster, Entry.MaxPerCluster));

		for (int32 n = 0; n < PerCluster; ++n)
		{
			FVector Location;
			if (!FindGroundPointNear(FVector2D(Center.X, Center.Y), Entry.ClusterRadius, Rng, Location))
			{
				continue;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			if (ARLResourceNode* Node = GetWorld()->SpawnActor<ARLResourceNode>(NodeClass, Location,
				FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), Params))
			{
				Node->MaterialItemId = Entry.MaterialItemId;
			}
		}
	}
}

bool ARLZoneScatterVolume::FindGroundPoint(FRLXoshiro256& Rng, FVector& OutLocation) const
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

bool ARLZoneScatterVolume::FindGroundPointNear(const FVector2D& CenterXY, float Radius, FRLXoshiro256& Rng, FVector& OutLocation) const
{
	const FVector Origin = Bounds->GetComponentLocation();
	const FVector Extent = Bounds->GetScaledBoxExtent();

	for (int32 Try = 0; Try < MaxPlacementTries; ++Try)
	{
		// Polar offset around the cluster center (same shape as the enemy director).
		const float Angle = Rng.FRandRange(0.f, 2.f * PI);
		const float Dist = Rng.FRandRange(0.f, Radius);
		const FVector Start(
			CenterXY.X + FMath::Cos(Angle) * Dist,
			CenterXY.Y + FMath::Sin(Angle) * Dist,
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
