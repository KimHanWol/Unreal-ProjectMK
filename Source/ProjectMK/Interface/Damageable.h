// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h" 

#include "Damageable.generated.h"

class UAbilitySystemComponent;

UINTERFACE(Blueprintable)
class PROJECTMK_API UDamageable : public UInterface
{
	GENERATED_BODY()
};

class IDamageable
{
    GENERATED_BODY()

public:
    virtual UAbilitySystemComponent* GetOwnerASC() = 0;

protected:
    virtual bool CheckIsDestroyed() = 0;
    virtual void OnDestroyed() = 0;
};
