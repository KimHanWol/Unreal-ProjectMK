// LINK

#include "ProjectMK/Component/SkillComponent.h"

#include "AbilitySystemComponent.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"

bool USkillComponent::HasPurchasedSkill(FName SkillKeyName) const
{
	return SkillKeyName.IsNone() == false && PurchasedSkillKeySet.Contains(SkillKeyName);
}

bool USkillComponent::CanPurchaseSkill(FName SkillKeyName) const
{
	if (SkillKeyName.IsNone())
	{
		return false;
	}

	const UDataManager* DataManager = UDataManager::Get(const_cast<USkillComponent*>(this));
	if (::IsValid(DataManager) == false)
	{
		return false;
	}

	const FSkillDataTableRow* SkillDataTableRow = DataManager->GetSkillDataTableRow(SkillKeyName);
	if (SkillDataTableRow == nullptr)
	{
		return false;
	}

	return CanPurchaseSkill(SkillKeyName, *SkillDataTableRow);
}

bool USkillComponent::TryPurchaseSkill(FName SkillKeyName)
{
	if (SkillKeyName.IsNone())
	{
		return false;
	}

	UDataManager* DataManager = UDataManager::Get(this);
	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	const UAttributeSet_Character* CharacterAttributeSet = GetCharacterAttributeSet();
	if (::IsValid(DataManager) == false || ::IsValid(AbilitySystemComponent) == false || ::IsValid(CharacterAttributeSet) == false)
	{
		return false;
	}

	const FSkillDataTableRow* SkillDataTableRow = DataManager->GetSkillDataTableRow(SkillKeyName);
	if (SkillDataTableRow == nullptr || CanPurchaseSkill(SkillKeyName, *SkillDataTableRow) == false)
	{
		return false;
	}

	UClass* LoadedEffectClass = SkillDataTableRow->GameplayEffect.LoadSynchronous();
	const TSubclassOf<UGameplayEffect> EffectClass = LoadedEffectClass;
	const bool bAppliedSkillEffect = SkillDataTableRow->SetByCallerTag.IsValid()
		? FGameplayAbilityUtils::TryApplyGameplayEffectToSelf(AbilitySystemComponent, EffectClass, SkillDataTableRow->SetByCallerTag, SkillDataTableRow->SetByCallerValue)
		: FGameplayAbilityUtils::TryApplyGameplayEffectToSelf(AbilitySystemComponent, EffectClass);
	if (bAppliedSkillEffect == false)
	{
		return false;
	}

	const float NewCoin = FMath::Max(0.f, CharacterAttributeSet->GetCoin() - static_cast<float>(SkillDataTableRow->Price));
	AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetCoinAttribute(), NewCoin);
	PurchasedSkillKeySet.Add(SkillKeyName);
	return true;
}

bool USkillComponent::AreRequiredSkillsPurchased(const TArray<FName>& RequiredSkillKeys) const
{
	for (const FName& RequiredSkillKey : RequiredSkillKeys)
	{
		if (HasPurchasedSkill(RequiredSkillKey) == false)
		{
			return false;
		}
	}

	return true;
}

bool USkillComponent::CanPurchaseSkill(FName SkillKeyName, const FSkillDataTableRow& SkillDataTableRow) const
{
	const UAttributeSet_Character* CharacterAttributeSet = GetCharacterAttributeSet();
	if (SkillKeyName.IsNone() || HasPurchasedSkill(SkillKeyName) || ::IsValid(CharacterAttributeSet) == false)
	{
		return false;
	}

	if (CharacterAttributeSet->GetCoin() < static_cast<float>(SkillDataTableRow.Price))
	{
		return false;
	}

	return AreRequiredSkillsPurchased(SkillDataTableRow.RequiredSkill);
}

UAbilitySystemComponent* USkillComponent::GetOwnerAbilitySystemComponent() const
{
	const AMKCharacter* OwnerCharacter = Cast<AMKCharacter>(GetOwner());
	if (::IsValid(OwnerCharacter) == false)
	{
		return nullptr;
	}

	return OwnerCharacter->GetAbilitySystemComponent();
}

const UAttributeSet_Character* USkillComponent::GetCharacterAttributeSet() const
{
	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return nullptr;
	}

	return Cast<UAttributeSet_Character>(AbilitySystemComponent->GetAttributeSet(UAttributeSet_Character::StaticClass()));
}
