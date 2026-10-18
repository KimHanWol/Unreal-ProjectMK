// LINK

#include "ProjectMK/Component/SkillComponent.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Manager/SoundManager.h"
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"
#include "ProjectMK/System/SkillDebugUtils.h"

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
		USoundManager::TryPlaySFX(GetOwner(), ESFXType::Skill_PurchaseFailed);
		MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][Purchase] Failed to purchase skill %s"), *SkillKeyName.ToString());
		return false;
	}

	if (TryApplySkill(AbilitySystemComponent, SkillKeyName, *SkillDataTableRow) == false)
	{
		return false;
	}

	const float NewCoin = FMath::Max(0.f, CharacterAttributeSet->GetCoin() - static_cast<float>(SkillDataTableRow->Price));
	AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetCoinAttribute(), NewCoin);
	PurchasedSkillKeySet.Add(SkillKeyName);
	MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][Purchase] Purchased skill %s Price=%d RemainingCoin=%.2f EffectCount=%d AbilityCount=%d"),
		*SkillKeyName.ToString(),
		SkillDataTableRow->Price,
		NewCoin,
		SkillDataTableRow->GameplayEffectList.Num(),
		SkillDataTableRow->GameplayAbilityList.Num());
	USoundManager::TryPlaySFX(GetOwner(), ESFXType::Skill_Purchase);
	OnSkillPurchasedDelegate.Broadcast(SkillKeyName);
	return true;
}

void USkillComponent::ApplySavedSkills(const TArray<FName>& SavedSkillKeys)
{
	UDataManager* DataManager = UDataManager::Get(this);
	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (::IsValid(DataManager) == false || ::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	for (const FName& SavedSkillKey : SavedSkillKeys)
	{
		if (HasPurchasedSkill(SavedSkillKey))
		{
			continue;
		}

		const FSkillDataTableRow* SkillDataTableRow = DataManager->GetSkillDataTableRow(SavedSkillKey);
		if (SkillDataTableRow == nullptr || TryApplySkill(AbilitySystemComponent, SavedSkillKey, *SkillDataTableRow) == false)
		{
			continue;
		}

		PurchasedSkillKeySet.Add(SavedSkillKey);
	}
}

TArray<FName> USkillComponent::GetPurchasedSkillKeys() const
{
	return PurchasedSkillKeySet.Array();
}

bool USkillComponent::TryApplySkill(UAbilitySystemComponent* AbilitySystemComponent, FName SkillKeyName, const FSkillDataTableRow& SkillDataTableRow) const
{
	if (SkillDataTableRow.GameplayEffectList.IsEmpty() && SkillDataTableRow.GameplayAbilityList.IsEmpty())
	{
		return false;
	}

	for (const FSkillGameplayEffectData& SkillGameplayEffectData : SkillDataTableRow.GameplayEffectList)
	{
		if (TryApplySkillGameplayEffect(AbilitySystemComponent, SkillGameplayEffectData) == false)
		{
			MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][Purchase] Failed to apply gameplay effect for skill %s"), *SkillKeyName.ToString());
			return false;
		}
	}

	for (const FSkillGameplayAbilityData& SkillGameplayAbilityData : SkillDataTableRow.GameplayAbilityList)
	{
		if (TryGrantSkillAbility(AbilitySystemComponent, SkillGameplayAbilityData) == false)
		{
			MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][Purchase] Failed to grant gameplay ability for skill %s"), *SkillKeyName.ToString());
			return false;
		}
	}

	return true;
}

bool USkillComponent::TryApplySkillGameplayEffect(UAbilitySystemComponent* AbilitySystemComponent, const FSkillGameplayEffectData& SkillGameplayEffectData) const
{
	if (::IsValid(AbilitySystemComponent) == false || SkillGameplayEffectData.GameplayEffect.IsNull())
	{
		return false;
	}

	UClass* LoadedEffectClass = SkillGameplayEffectData.GameplayEffect.LoadSynchronous();
	const TSubclassOf<UGameplayEffect> EffectClass = LoadedEffectClass;
	if (::IsValid(EffectClass) == false)
	{
		return false;
	}

	if (SkillGameplayEffectData.SetByCallerDataList.IsEmpty())
	{
		MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][Purchase] Apply effect %s without SetByCaller"), *GetNameSafe(EffectClass));
		return FGameplayAbilityUtils::TryApplyGameplayEffectToSelf(AbilitySystemComponent, EffectClass);
	}

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, AbilitySystemComponent->MakeEffectContext());
	if (SpecHandle.IsValid() == false)
	{
		return false;
	}

	for (const FSkillSetByCallerData& SetByCallerData : SkillGameplayEffectData.SetByCallerDataList)
	{
		if (SetByCallerData.SetByCallerTag.IsValid() == false)
		{
			continue;
		}

		SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerData.SetByCallerTag, SetByCallerData.SetByCallerValue);
		MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][Purchase] Apply effect %s SetByCaller %s=%.3f"),
			*GetNameSafe(EffectClass),
			*SetByCallerData.SetByCallerTag.ToString(),
			SetByCallerData.SetByCallerValue);
	}

	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return true;
}

bool USkillComponent::TryGrantSkillAbility(UAbilitySystemComponent* AbilitySystemComponent, const FSkillGameplayAbilityData& SkillGameplayAbilityData) const
{
	if (::IsValid(AbilitySystemComponent) == false || SkillGameplayAbilityData.GameplayAbility.IsNull())
	{
		return false;
	}

	UClass* LoadedAbilityClass = SkillGameplayAbilityData.GameplayAbility.LoadSynchronous();
	const TSubclassOf<UGameplayAbility> AbilityClass = LoadedAbilityClass;
	if (::IsValid(AbilityClass) == false)
	{
		return false;
	}

	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass));
	MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][Purchase] Granted ability %s"), *GetNameSafe(AbilityClass));
	return true;
}

bool USkillComponent::TryAddCoin(int32 CoinAmount, bool bApplyCoinGainMultiplier)
{
	if (CoinAmount <= 0)
	{
		return false;
	}

	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	const UAttributeSet_Character* CharacterAttributeSet = GetCharacterAttributeSet();
	if (::IsValid(AbilitySystemComponent) == false || ::IsValid(CharacterAttributeSet) == false)
	{
		return false;
	}

	const int32 AdjustedCoinAmount = bApplyCoinGainMultiplier ? CalculateAdjustedCoinAmount(CoinAmount) : CoinAmount;
	if (AdjustedCoinAmount <= 0)
	{
		return false;
	}

	const float NewCoin = CharacterAttributeSet->GetCoin() + static_cast<float>(AdjustedCoinAmount);
	AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetCoinAttribute(), NewCoin);
	USoundManager::TryPlaySFX(GetOwner(), ESFXType::Coin_Gain);
	MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][Recycler] Coin reward applied Base=%d Adjusted=%d CurrentCoin=%.2f"), CoinAmount, AdjustedCoinAmount, NewCoin);
	return true;
}

bool USkillComponent::TryAddOxygen(int32 OxygenAmount)
{
	if (OxygenAmount <= 0)
	{
		return false;
	}

	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return false;
	}

	FGameplayAbilityUtils::ApplyOxygen(AbilitySystemComponent, static_cast<float>(OxygenAmount));
	MK_SKILL_DEBUG_LOG(Warning, TEXT("[ItemReward] Oxygen reward applied Amount=%d"), OxygenAmount);
	return true;
}

int32 USkillComponent::CalculateAdjustedCoinAmount(int32 CoinAmount) const
{
	if (CoinAmount <= 0)
	{
		return 0;
	}

	const UAttributeSet_Character* CharacterAttributeSet = GetCharacterAttributeSet();
	if (::IsValid(CharacterAttributeSet) == false)
	{
		return CoinAmount;
	}

	const float CoinGainMultiplier = CharacterAttributeSet->GetCoinGainMultiplier();
	const int32 AdjustedCoinAmount = FMath::Max(0, FMath::RoundToInt(static_cast<float>(CoinAmount) * FMath::Max(0.f, CoinGainMultiplier)));
	MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][CoinSense] BaseCoin=%d Multiplier=%.3f AdjustedCoin=%d BonusCoin=%d"),
		CoinAmount,
		CoinGainMultiplier,
		AdjustedCoinAmount,
		FMath::Max(0, AdjustedCoinAmount - CoinAmount));
	return AdjustedCoinAmount;
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
