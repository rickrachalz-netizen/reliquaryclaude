// RELIQUARY — the full hero/enemy stat suite.
//
// Vitals:    Health, MaxHealth, HealthRegen, Mana, MaxMana, ManaRegen
// Primary:   Strength (Warrior), Agility (Rogue), Intellect (Mage)
// Secondary: CritChance, CritDamage, Haste, Armor, MoveSpeed, Adaptability
// Meta:      IncomingDamage (transient pipeline value, never persisted)
//
// Adaptability (per the production map): increases the damage of abilities
// and attacks by X% when they aren't a repeat of your last action, stacking
// up to five times. The stack count lives on URLAbilitySystemComponent; this
// attribute is the "X% per stack" magnitude.

#pragma once
#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "RLAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class RELIQUARY_API URLAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    // --- Vitals ---

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Vitals")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, Health)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Vitals")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, MaxHealth)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HealthRegen, Category = "Vitals")
    FGameplayAttributeData HealthRegen;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, HealthRegen)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Vitals")
    FGameplayAttributeData Mana;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, Mana)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "Vitals")
    FGameplayAttributeData MaxMana;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, MaxMana)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ManaRegen, Category = "Vitals")
    FGameplayAttributeData ManaRegen;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, ManaRegen)

    // --- Primary ---

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "Primary")
    FGameplayAttributeData Strength;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, Strength)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Agility, Category = "Primary")
    FGameplayAttributeData Agility;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, Agility)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Intellect, Category = "Primary")
    FGameplayAttributeData Intellect;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, Intellect)

    // --- Secondary ---

    /** Chance in [0,1] for attacks to crit. */
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritChance, Category = "Secondary")
    FGameplayAttributeData CritChance;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, CritChance)

    /** Crit damage multiplier (2.0 = double damage). */
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritDamage, Category = "Secondary")
    FGameplayAttributeData CritDamage;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, CritDamage)

    /** Cooldown/attack-speed rate multiplier (0.10 = 10% faster). */
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Haste, Category = "Secondary")
    FGameplayAttributeData Haste;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, Haste)

    /** Flat mitigation rating; converted to % reduction in the damage execution. */
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "Secondary")
    FGameplayAttributeData Armor;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, Armor)

    /** Ground speed in cm/s; the character applies this to movement each change. */
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Secondary")
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, MoveSpeed)

    /** Bonus damage fraction per Adaptability stack (0.03 = +3% per stack, max 5 stacks). */
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Adaptability, Category = "Secondary")
    FGameplayAttributeData Adaptability;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, Adaptability)

    // --- Meta (not replicated; consumed inside PostGameplayEffectExecute) ---

    UPROPERTY(BlueprintReadOnly, Category = "Meta")
    FGameplayAttributeData IncomingDamage;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, IncomingDamage)

protected:
    UFUNCTION() void OnRep_Health(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_HealthRegen(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Mana(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_MaxMana(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_ManaRegen(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Strength(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Agility(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Intellect(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_CritChance(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_CritDamage(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Haste(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Armor(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_MoveSpeed(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Adaptability(const FGameplayAttributeData& Old);
};
