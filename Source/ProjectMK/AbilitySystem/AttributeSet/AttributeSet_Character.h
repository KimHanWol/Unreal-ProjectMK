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
};
