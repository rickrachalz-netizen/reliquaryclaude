#include "RLAbilitySystemGlobals.h"
#include "RLGameplayEffectContext.h"

FGameplayEffectContext* URLAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FRLGameplayEffectContext();
}
