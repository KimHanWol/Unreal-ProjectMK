// LINK

#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/System/SkillDebugUtils.h"

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
		if (ABlockBase* TargetBlock = Cast<ABlockBase>(TargetASC->GetOwner()))
		{
			TargetBlock->SetLastDamageInstigatorASC(SourceASC);
		}

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
		if (ABlockBase* TargetBlock = Cast<ABlockBase>(TargetASC->GetOwner()))
		{
			TargetBlock->SetLastDamageInstigatorASC(SourceASC);
		}

		SpecHandle.Data->SetSetByCallerMagnitude(CommonValueTag, -Damage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

int32 FGameplayAbilityUtils::CalculateAdjustedItemCount(UObject* WorldContextObject, UAbilitySystemComponent* InstigatorASC, FName ItemUID, int32 ItemCount)
{
	if (ItemUID.IsNone() || ItemCount <= 0)
	{
		return 0;
	}

	const UDataManager* DataManager = UDataManager::Get(WorldContextObject);
	if (::IsValid(DataManager) == false)
	{
		return ItemCount;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemUID);
	if (ItemDataTableRow == nullptr || ItemDataTableRow->bIsOre == false)
	{
		return ItemCount;
	}

	if (::IsValid(InstigatorASC) == false)
	{
		return ItemCount;
	}

	const UAttributeSet_Character* CharacterAttributeSet = Cast<UAttributeSet_Character>(InstigatorASC->GetAttributeSet(UAttributeSet_Character::StaticClass()));
	if (::IsValid(CharacterAttributeSet) == false)
	{
		return ItemCount;
	}

	const float OreBonusDropChance = CharacterAttributeSet->GetOreBonusDropChance();
	if (OreBonusDropChance <= 0.f)
	{
		MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][LuckyOre] Item=%s BaseCount=%d BonusChance=%.3f AdjustedCount=%d"), *ItemUID.ToString(), ItemCount, OreBonusDropChance, ItemCount);
		return ItemCount;
	}

	int32 AdjustedItemCount = ItemCount;
	int32 BonusItemCount = 0;
	for (int32 ItemIndex = 0; ItemIndex < ItemCount; ++ItemIndex)
	{
		const float Roll = FMath::FRand();
		if (Roll <= OreBonusDropChance)
		{
			AdjustedItemCount++;
			BonusItemCount++;
		}

		MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][LuckyOre] Item=%s RollIndex=%d Roll=%.3f BonusChance=%.3f GrantedBonus=%s"), *ItemUID.ToString(), ItemIndex, Roll, OreBonusDropChance, Roll <= OreBonusDropChance ? TEXT("true") : TEXT("false"));
	}

	MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][LuckyOre] Item=%s BaseCount=%d BonusCount=%d AdjustedCount=%d"), *ItemUID.ToString(), ItemCount, BonusItemCount, AdjustedItemCount);
	return AdjustedItemCount;
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
