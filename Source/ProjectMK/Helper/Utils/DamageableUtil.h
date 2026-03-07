// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UAbilitySystemComponent;

struct FDamageableUtil
{
    static void ApplyDamage(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC, float Damage);
    static void ApplyDamageToDurability(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC, float Damage);
    static void ApplyOxygen(UAbilitySystemComponent* TargetASC, float OxygenDelta);
};
