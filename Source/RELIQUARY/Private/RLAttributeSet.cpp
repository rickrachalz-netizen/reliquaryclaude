#include "RLAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

void URLAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Strength, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Intellect, COND_None, REPNOTIFY_Always);
}

void URLAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    if (Attribute == GetHealthAttribute())
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
}

void URLAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
        SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
}

void URLAttributeSet::OnRep_Health(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Health, Old); }
void URLAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, MaxHealth, Old); }
void URLAttributeSet::OnRep_Strength(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Strength, Old); }
void URLAttributeSet::OnRep_Intellect(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Intellect, Old); }