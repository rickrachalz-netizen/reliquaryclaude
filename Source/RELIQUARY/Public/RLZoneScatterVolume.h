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
	bool FindGroundPoint(FRLXoshiro256& Rng, FVector& OutLocation) const;

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
