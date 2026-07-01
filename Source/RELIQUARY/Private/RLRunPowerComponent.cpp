#include "RLRunPowerComponent.h"
#include "RLRunManagerSubsystem.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "RLStatLibrary.h"
#include "AbilitySystemComponent.h"

bool URLRunPowerComponent::ApplyBoon(UAbilitySystemComponent* ASC, FName BoonId)
{
	return ApplyBoonInternal(ASC, BoonId, /*bRecord=*/true);
}

void URLRunPowerComponent::RestoreFromRunManager(UAbilitySystemComponent* ASC)
{
	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	URLRunManagerSubsystem* RunManager = GI ? GI->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
	if (!RunManager)
	{
		return;
	}

	ClearAll(ASC);
	for (const auto& Pair : RunManager->GetAllBoonStacks())
	{
		for (int32 Stack = 0; Stack < Pair.Value; ++Stack)
		{
			ApplyBoonInternal(ASC, Pair.Key, /*bRecord=*/false);
		}
	}
}

void URLRunPowerComponent::ClearAll(UAbilitySystemComponent* ASC)
{
	if (ASC)
	{
		for (const FActiveGameplayEffectHandle& Handle : AppliedHandles)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}
	AppliedHandles.Reset();
}

bool URLRunPowerComponent::ApplyBoonInternal(UAbilitySystemComponent* ASC, FName BoonId, bool bRecord)
{
	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;
	URLRunManagerSubsystem* RunManager = GI ? GI->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
	const FRLBoonRow* Boon = Data ? Data->FindBoon(BoonId) : nullptr;
	if (!ASC || !Boon || !RunManager)
	{
		return false;
	}

	FActiveGameplayEffectHandle Handle = URLStatLibrary::ApplyStatBlock(
		ASC, Boon->Stats, FString::Printf(TEXT("Boon_%s"), *BoonId.ToString()));
	if (Handle.IsValid())
	{
		AppliedHandles.Add(Handle);
	}

	if (!Boon->GrantedEffect.IsNull())
	{
		if (UClass* EffectClass = Boon->GrantedEffect.LoadSynchronous())
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
			if (Spec.IsValid())
			{
				FActiveGameplayEffectHandle EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				if (EffectHandle.IsValid())
				{
					AppliedHandles.Add(EffectHandle);
				}
			}
		}
	}

	if (bRecord)
	{
		RunManager->RecordBoonStack(BoonId);
	}
	return true;
}
