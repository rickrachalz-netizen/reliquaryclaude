#include "RLEquipmentComponent.h"
#include "RLGameInstance.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "RLStatLibrary.h"
#include "AbilitySystemComponent.h"

void URLEquipmentComponent::RefreshEquipment(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	ClearEquipment(ASC);

	URLGameInstance* GI = Cast<URLGameInstance>(GetWorld()->GetGameInstance());
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;
	const FRLHeroData* Hero = GI ? GI->GetActiveHero() : nullptr;
	if (!Hero || !Data)
	{
		return;
	}

	TMap<FGameplayTag, int32> SetPieceCounts;

	for (const auto& Pair : Hero->Equipped)
	{
		const FRLItemRow* Item = Data->FindItem(Pair.Value);
		if (!Item)
		{
			continue;
		}

		// Flat stats on the piece.
		FActiveGameplayEffectHandle Handle = URLStatLibrary::ApplyStatBlock(
			ASC, Item->Stats, Pair.Value.ToString());
		if (Handle.IsValid())
		{
			AppliedHandles.Add(Handle);
		}

		// Cantrip: the piece's bespoke passive.
		if (!Item->CantripEffect.IsNull())
		{
			if (UClass* EffectClass = Item->CantripEffect.LoadSynchronous())
			{
				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
				if (Spec.IsValid())
				{
					FActiveGameplayEffectHandle CantripHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
					if (CantripHandle.IsValid())
					{
						AppliedHandles.Add(CantripHandle);
					}
				}
			}
		}

		if (Item->SetTag.IsValid())
		{
			++SetPieceCounts.FindOrAdd(Item->SetTag);
		}
	}

	// Set bonuses: every threshold at or below the worn piece count applies.
	for (const auto& SetPair : SetPieceCounts)
	{
		TArray<const FRLSetBonusRow*> Bonuses;
		Data->GetSetBonuses(SetPair.Key, Bonuses);
		for (const FRLSetBonusRow* Bonus : Bonuses)
		{
			if (SetPair.Value < Bonus->RequiredPieces)
			{
				break;
			}

			FActiveGameplayEffectHandle Handle = URLStatLibrary::ApplyStatBlock(
				ASC, Bonus->Stats, FString::Printf(TEXT("Set_%s_%d"), *SetPair.Key.ToString(), Bonus->RequiredPieces));
			if (Handle.IsValid())
			{
				AppliedHandles.Add(Handle);
			}

			if (!Bonus->BonusEffect.IsNull())
			{
				if (UClass* EffectClass = Bonus->BonusEffect.LoadSynchronous())
				{
					FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
					FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
					if (Spec.IsValid())
					{
						FActiveGameplayEffectHandle BonusHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
						if (BonusHandle.IsValid())
						{
							AppliedHandles.Add(BonusHandle);
						}
					}
				}
			}
		}
	}
}

void URLEquipmentComponent::ClearEquipment(UAbilitySystemComponent* ASC)
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
