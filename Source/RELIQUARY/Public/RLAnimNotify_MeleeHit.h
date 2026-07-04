// RELIQUARY — anim notify marking the impact frame of a melee swing.
// Drop one on each attack montage where the blade crosses the target;
// URLMeleeAttackAbility waits for it before sweeping for damage.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "RLAnimNotify_MeleeHit.generated.h"

UCLASS(meta = (DisplayName = "RL Melee Hit"))
class RELIQUARY_API URLAnimNotify_MeleeHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("RL Melee Hit"); }
};
