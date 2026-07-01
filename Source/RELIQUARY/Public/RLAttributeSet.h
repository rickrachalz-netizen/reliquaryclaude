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

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Vitals")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, Health)

        UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Vitals")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, MaxHealth)

        UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "Primary")
    FGameplayAttributeData Strength;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, Strength)

        UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Intellect, Category = "Primary")
    FGameplayAttributeData Intellect;
    ATTRIBUTE_ACCESSORS(URLAttributeSet, Intellect)

protected:
    UFUNCTION() void OnRep_Health(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Strength(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Intellect(const FGameplayAttributeData& Old);
};