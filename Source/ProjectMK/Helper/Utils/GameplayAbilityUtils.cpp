// LINK

#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "ProjectMK/Core/Manager/DataManager.h"

namespace
{
	const FGameplayTag CommonValueTag = FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Common.Value"));
}

bool FGameplayAbilityUtils::TryApplyGameplayEffectToSelf(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass)
{
	if (::IsValid(TargetASC) == false || ::IsValid(EffectClass) == false)
	{
		return false;
	}

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, TargetASC->MakeEffectContext());
	if (SpecHandle.IsValid() == false)
	{
		return false;
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return true;
}

bool FGameplayAbilityUtils::TryApplyGameplayEffectToSelf(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& SetByCallerTag, float SetByCallerValue)
{
	if (::IsValid(TargetASC) == false || ::IsValid(EffectClass) == false || SetByCallerTag.IsValid() == false)
	{
		return false;
	}

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, TargetASC->MakeEffectContext());
	if (SpecHandle.IsValid() == false)
	{
		return false;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, SetByCallerValue);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return true;
}

FActiveGameplayEffectHandle FGameplayAbilityUtils::ApplyGameplayEffectToSelf(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass)
{
	if (::IsValid(TargetASC) == false || ::IsValid(EffectClass) == false)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, TargetASC->MakeEffectContext());
	if (SpecHandle.IsValid() == false)
	{
		return FActiveGameplayEffectHandle();
	}

	return TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

FActiveGameplayEffectHandle FGameplayAbilityUtils::ApplyGameplayEffectToSelf(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& SetByCallerTag, float SetByCallerValue)
{
	if (::IsValid(TargetASC) == false || ::IsValid(EffectClass) == false || SetByCallerTag.IsValid() == false)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, TargetASC->MakeEffectContext());
	if (SpecHandle.IsValid() == false)
	{
		return FActiveGameplayEffectHandle();
	}

	SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, SetByCallerValue);
	return TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void FGameplayAbilityUtils::ApplyDamage(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC, float Damage)
{
	if (::IsValid(TargetASC) == false)
	{
		return;
	}

	const UDataManager* DataManager = UDataManager::Get(TargetASC);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	TSubclassOf<UGameplayEffect> EffectClass = DataManager->GetGameplayEffect(EGameplayEffectType::CurrentHealth_Add);
	if (::IsValid(EffectClass) == false)
	{
		return;
	}

	if (::IsValid(SourceASC) == false)
	{
		ApplyGameplayEffectToSelf(TargetASC, EffectClass, CommonValueTag, -Damage);
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, TargetASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(CommonValueTag, -Damage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void FGameplayAbilityUtils::ApplyDamageToDurability(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC, float Damage)
{
	if (::IsValid(TargetASC) == false)
	{
		return;
	}

	const UDataManager* DataManager = UDataManager::Get(TargetASC);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	TSubclassOf<UGameplayEffect> EffectClass = DataManager->GetGameplayEffect(EGameplayEffectType::Durability_Add);
	if (::IsValid(EffectClass) == false)
	{
		return;
	}

	if (::IsValid(SourceASC) == false)
	{
		ApplyGameplayEffectToSelf(TargetASC, EffectClass, CommonValueTag, -Damage);
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, TargetASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(CommonValueTag, -Damage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void FGameplayAbilityUtils::ApplyOxygen(UAbilitySystemComponent* TargetASC, float OxygenDelta)
{
	if (::IsValid(TargetASC) == false || FMath::IsNearlyZero(OxygenDelta))
	{
		return;
	}

	const UDataManager* DataManager = UDataManager::Get(TargetASC);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	TSubclassOf<UGameplayEffect> EffectClass = DataManager->GetGameplayEffect(EGameplayEffectType::CurrentOxygen_Add);
	if (::IsValid(EffectClass) == false)
	{
		return;
	}

	ApplyGameplayEffectToSelf(TargetASC, EffectClass, CommonValueTag, OxygenDelta);
}
