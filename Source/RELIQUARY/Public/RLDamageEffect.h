// RELIQUARY — a preconfigured instant damage GameplayEffect.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "RLDamageEffect.generated.h"

/**
 * Instant effect wired to URLDamageExecution. Abilities fill in the
 * SetByCaller.Damage magnitude; nothing needs to be configured in the editor.
 * Blueprint GEs can still subclass this to add on-hit extras (slows, burns).
 */
UCLASS()
class RELIQUARY_API URLDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	URLDamageEffect();
};
