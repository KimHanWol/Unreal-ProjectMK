// LINK

#include "ProjectMK/Helper/Utils/DamageableUtil.h"

#include "ProjectMK/Core/Manager/DataManager.h"
#include "AbilitySystemComponent.h"

void FDamageableUtil::ApplyDamage(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC, float Damage)
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

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, TargetASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		FGameplayTag ValueTag = FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Common.Value"));
		SpecHandle.Data->SetSetByCallerMagnitude(ValueTag, -Damage);
		if (::IsValid(SourceASC))
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
		else
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void FDamageableUtil::ApplyDamageToDurability(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC, float Damage)
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

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, TargetASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		FGameplayTag ValueTag = FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Common.Value"));
		SpecHandle.Data->SetSetByCallerMagnitude(ValueTag, -Damage);
		if (::IsValid(SourceASC))
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
		else
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void FDamageableUtil::ApplyOxygen(UAbilitySystemComponent* TargetASC, float OxygenDelta)
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

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, TargetASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		const FGameplayTag ValueTag = FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Common.Value"));
		SpecHandle.Data->SetSetByCallerMagnitude(ValueTag, OxygenDelta);
		TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
