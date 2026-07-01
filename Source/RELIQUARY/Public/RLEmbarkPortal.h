// RELIQUARY — portals at base camp. One leads to the realm's path (a fresh
// run); another leads straight to the Wild God that bars the way home,
// challengeable at any time — surviving it is another matter.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLInteractable.h"
#include "RLEmbarkPortal.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ERLPortalDestination : uint8
{
	RealmPath,	// begin a resource run at zone 1
	WildGod		// the game win condition encounter
};

UCLASS()
class RELIQUARY_API ARLEmbarkPortal : public AActor, public IRLInteractable
{
	GENERATED_BODY()

public:
	ARLEmbarkPortal();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Portal")
	ERLPortalDestination Destination = ERLPortalDestination::RealmPath;

	/** Arena map for the Wild God encounter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Portal")
	FName WildGodArenaMap = TEXT("L_WildGodArena");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// --- IRLInteractable ---
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
};
