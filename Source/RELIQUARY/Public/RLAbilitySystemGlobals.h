// RELIQUARY — project AbilitySystemGlobals. Its one job: allocate the
// crit-carrying FRLGameplayEffectContext for every effect context in the game.
// GAS builds contexts through UAbilitySystemGlobals::AllocGameplayEffectContext
// (both UGameplayAbility::MakeEffectContext and UAbilitySystemComponent::
// MakeEffectContext route here), so this override is what actually puts our
// context type on damage specs. Registered in DefaultGame.ini via
// AbilitySystemGlobalsClassName.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "RLAbilitySystemGlobals.generated.h"

UCLASS()
class RELIQUARY_API URLAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
