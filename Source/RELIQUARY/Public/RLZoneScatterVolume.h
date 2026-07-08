// RELIQUARY — semi-procedural zone population, in the spirit of Risk of
// Rain 2's stage variation. One volume covers the playable space of a run
// map; on load it scatters resource nodes, upgrade altars, the challenge
// altar, and (every third zone) a banking crate. Placement is driven by the
// run seed locked at embark, so re-entering cannot fish for better layouts.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLXoshiro.h"
#include "RLZoneScatterVolume.generated.h"

class UBoxComponent;
class ARLResourceNode;
class ARLUpgradeAltar;
class ARLChallengeAltar;
class ARLBankingCrate;

/** How a resource type spreads over the map. */
UENUM(BlueprintType)
enum class ERLScatterDistribution : uint8
{
	/** Independent uniform samples over the whole volume (e.g. ore boulders). */
	Uniform,
	/** Grouped into clumps — a few seeded centers, several nodes each (trees). */
	Clustered
};

/**
 * One resource type this map grows, with all of its procedural knobs. Add one
 * entry per material you want to spawn; the list on the volume is the single
 * per-map authority over what appears and how much.
 */
USTRUCT(BlueprintType)
struct FRLResourceScatterEntry
{
	GENERATED_BODY()

	/** DT_Items material row each node yields (e.g. OakheartTimber, DuskironOre). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter")
	FName MaterialItemId;

	/** Node blueprint to spawn (BP_Tree, BP_Stone, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter")
	TSubclassOf<ARLResourceNode> NodeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter")
	ERLScatterDistribution Distribution = ERLScatterDistribution::Uniform;

	/** Uniform: total node count. Clustered: number of clusters. Rolled in [Min,Max]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter", meta = (ClampMin = "0"))
	int32 MinCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter", meta = (ClampMin = "0"))
	int32 MaxCount = 10;

	/** Clustered only: nodes per cluster, rolled in [Min,Max]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter", meta = (ClampMin = "1"))
	int32 MinPerCluster = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter", meta = (ClampMin = "1"))
	int32 MaxPerCluster = 6;

	/** Clustered only: how far a cluster's nodes spread from its center (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter", meta = (ClampMin = "0"))
	float ClusterRadius = 700.f;
};

UCLASS()
class RELIQUARY_API ARLZoneScatterVolume : public AActor
{
	GENERATED_BODY()

public:
	ARLZoneScatterVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Bounds;

	/** Node blueprint per material row name; falls back to DefaultNodeClass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter")
	TMap<FName, TSubclassOf<ARLResourceNode>> NodeClassByMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter")
	TSubclassOf<ARLResourceNode> DefaultNodeClass;

	/**
	 * Per-map resource authoring. When non-empty, this list fully drives resource
	 * scatter for the map (the legacy zone-material path below is skipped). Each
	 * entry names a material, its node blueprint, and its distribution/count knobs.
	 * Defaults to an Oakheart clustered forest + scattered Duskiron ore.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter")
	TArray<FRLResourceScatterEntry> ResourceScatter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter")
	TSubclassOf<ARLUpgradeAltar> UpgradeAltarClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter")
	TSubclassOf<ARLChallengeAltar> ChallengeAltarClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Scatter")
	TSubclassOf<ARLBankingCrate> BankingCrateClass;

	/** Max ground-trace attempts per placed actor. */
	UPROPERTY(EditAnywhere, Category = "RELIQUARY|Scatter")
	int32 MaxPlacementTries = 12;

protected:
	virtual void BeginPlay() override;

	void ScatterZone();

	/** Spawn one resource type's nodes per its distribution/count knobs. */
	void ScatterResourceEntry(const FRLResourceScatterEntry& Entry, FRLXoshiro256& Rng);

	bool FindGroundPoint(FRLXoshiro256& Rng, FVector& OutLocation) const;

	/** Trace to ground near an XY point (for clustered placement). */
	bool FindGroundPointNear(const FVector2D& CenterXY, float Radius, FRLXoshiro256& Rng, FVector& OutLocation) const;

	template <typename ActorType>
	ActorType* SpawnScattered(TSubclassOf<ActorType> ActorClass, FRLXoshiro256& Rng)
	{
		if (!ActorClass)
		{
			return nullptr;
		}
		FVector Location;
		if (!FindGroundPoint(Rng, Location))
		{
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		return GetWorld()->SpawnActor<ActorType>(ActorClass, Location,
			FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), Params);
	}
};
