// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "ProjectMK/AbilitySystem/AttributeSet/MKAttributeSetBase.h"
#include "AttributeSet_Block.generated.h"

UCLASS()
class PROJECTMK_API UAttributeSet_Block : public UMKAttributeSetBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    FGameplayAttributeData Durability;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Block, Durability)
};
