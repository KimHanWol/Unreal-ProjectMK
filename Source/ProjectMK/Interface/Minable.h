// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h" 

#include "Minable.generated.h"

class UAbilitySystemComponent;

UINTERFACE(Blueprintable)
class PROJECTMK_API UMinable : public UInterface
{
	GENERATED_BODY()
};

class IMinable
{
    GENERATED_BODY()

public:
    virtual UAbilitySystemComponent* GetOwnerASC() = 0;
    virtual float GetMiningDamage() = 0;
    virtual float GetMiningDuration() = 0;
};
