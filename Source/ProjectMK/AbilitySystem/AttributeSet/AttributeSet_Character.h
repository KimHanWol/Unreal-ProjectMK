// Fill out your copyright notice in the Description page of Project Settings.

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
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, MoveSpeed)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData FlyingSpeed;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, FlyingSpeed)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData DrillingRate;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, DrillingRate)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData DrillingPower;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, DrillingPower)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData Coin;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Character, Coin)
};
