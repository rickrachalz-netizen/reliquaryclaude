#include "RLAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "RLAbilitySystemComponent.h"

void URLAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, HealthRegen, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Mana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, ManaRegen, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Strength, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Agility, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Intellect, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, CritDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Haste, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Armor, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Adaptability, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Multistrike, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Hatred, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Sanguination, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Force, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Synergy, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(URLAttributeSet, Frenzy, COND_None, REPNOTIFY_Always);
}

void URLAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    }
    else if (Attribute == GetManaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
    }
    else if (Attribute == GetCritChanceAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
    }
    else if (Attribute == GetMoveSpeedAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 100.f, 2000.f);
    }
}

void URLAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
    {
        // Consume the meta attribute and route it into Health.
        const float Damage = GetIncomingDamage();
        SetIncomingDamage(0.f);

        if (Damage > 0.f)
        {
            const float NewHealth = FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth());
            SetHealth(NewHealth);

            if (URLAbilitySystemComponent* RLASC = Cast<URLAbilitySystemComponent>(GetOwningAbilitySystemComponent()))
            {
                RLASC->HandleDamageTaken(Damage, Data.EffectSpec.GetContext(), NewHealth <= 0.f);
            }
        }
    }
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
    }
    else if (Data.EvaluatedData.Attribute == GetManaAttribute())
    {
        SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
    }
}

void URLAttributeSet::OnRep_Health(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Health, Old); }
void URLAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, MaxHealth, Old); }
void URLAttributeSet::OnRep_HealthRegen(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, HealthRegen, Old); }
void URLAttributeSet::OnRep_Mana(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Mana, Old); }
void URLAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, MaxMana, Old); }
void URLAttributeSet::OnRep_ManaRegen(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, ManaRegen, Old); }
void URLAttributeSet::OnRep_Strength(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Strength, Old); }
void URLAttributeSet::OnRep_Agility(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Agility, Old); }
void URLAttributeSet::OnRep_Intellect(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Intellect, Old); }
void URLAttributeSet::OnRep_CritChance(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, CritChance, Old); }
void URLAttributeSet::OnRep_CritDamage(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, CritDamage, Old); }
void URLAttributeSet::OnRep_Haste(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Haste, Old); }
void URLAttributeSet::OnRep_Armor(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Armor, Old); }
void URLAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, MoveSpeed, Old); }
void URLAttributeSet::OnRep_Adaptability(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Adaptability, Old); }
void URLAttributeSet::OnRep_Multistrike(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Multistrike, Old); }
void URLAttributeSet::OnRep_Hatred(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Hatred, Old); }
void URLAttributeSet::OnRep_Sanguination(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Sanguination, Old); }
void URLAttributeSet::OnRep_Force(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Force, Old); }
void URLAttributeSet::OnRep_Synergy(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Synergy, Old); }
void URLAttributeSet::OnRep_Frenzy(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(URLAttributeSet, Frenzy, Old); }
