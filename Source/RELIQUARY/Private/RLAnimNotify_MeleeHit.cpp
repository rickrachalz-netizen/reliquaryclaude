#include "RLAnimNotify_MeleeHit.h"
#include "RLGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"

void URLAnimNotify_MeleeHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		FGameplayEventData Payload;
		Payload.EventTag = RLTags::Event_Melee_Hit;
		Payload.Instigator = Owner;
		ASC->HandleGameplayEvent(RLTags::Event_Melee_Hit, &Payload);
	}
}
