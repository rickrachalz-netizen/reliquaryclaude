// RELIQUARY — a gameplay effect context that carries a critical-hit flag.
// The damage execution rolls crit internally; without somewhere to stash the
// result it would be lost before the target reacts. This context rides along
// with the damage spec so the hit target (and its floating damage number)
// knows whether the blow crit.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "RLGameplayEffectContext.generated.h"

USTRUCT()
struct RELIQUARY_API FRLGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:
	bool IsCriticalHit() const { return bIsCriticalHit; }
	void SetIsCriticalHit(bool bInCritical) { bIsCriticalHit = bInCritical; }

	/** Return our type so GAS keeps the extra data through copies/replication. */
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FRLGameplayEffectContext::StaticStruct();
	}

	virtual FRLGameplayEffectContext* Duplicate() const override
	{
		FRLGameplayEffectContext* NewContext = new FRLGameplayEffectContext(*this);
		if (GetHitResult())
		{
			// Deep copy the hit result so the clone owns its own.
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override
	{
		FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);
		uint8 CritByte = bIsCriticalHit ? 1 : 0;
		Ar << CritByte;
		bIsCriticalHit = (CritByte != 0);
		return true;
	}

protected:
	UPROPERTY()
	bool bIsCriticalHit = false;
};

template<>
struct TStructOpsTypeTraits<FRLGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FRLGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
