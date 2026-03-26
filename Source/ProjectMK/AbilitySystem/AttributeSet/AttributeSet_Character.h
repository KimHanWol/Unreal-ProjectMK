#pragma once

#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "ProjectMK/AbilitySystem/AttributeSet/MKAttributeSetBase.h"
#include "AttributeSet_Character.generated.h"

UCLASS()
class PROJECTMK_API UAttributeSet_Character : public UMKAttributeSetBase
{
	GENERATED_BODY()

public:
	UAttributeSet_Character();
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData ItemCollectRange;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, ItemCollectRange)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData CurrentHealth;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, CurrentHealth)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, MaxHealth)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData HealthRegenRate;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, HealthRegenRate)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, MoveSpeed)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData FlyingSpeed;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, FlyingSpeed)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData CurrentOxygen;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, CurrentOxygen)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData MaxOxygen;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, MaxOxygen)

	//Drilling
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData DrillingPeriod;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, DrillingPeriod)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData DrillingPower;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, DrillingPower)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData DrillingDistance;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, DrillingDistance)
	//~Drilling

	//Currency
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData Coin;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, Coin)
	//~Currency
};
