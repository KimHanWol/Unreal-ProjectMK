#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

class UAbilitySystemComponent;
class UGameplayEffect;

struct FGameplayAbilityUtils
{
	static bool TryApplyGameplayEffectToSelf(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass);
	static bool TryApplyGameplayEffectToSelf(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& SetByCallerTag, float SetByCallerValue);
	static FActiveGameplayEffectHandle ApplyGameplayEffectToSelf(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass);
	static FActiveGameplayEffectHandle ApplyGameplayEffectToSelf(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& SetByCallerTag, float SetByCallerValue);
	static void ApplyDamage(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC, float Damage);
	static void ApplyDamageToDurability(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC, float Damage);
	static void ApplyOxygen(UAbilitySystemComponent* TargetASC, float OxygenDelta);
};
