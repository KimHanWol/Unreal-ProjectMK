// LINK

#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"

#include "GameplayEffectExtension.h"

UAttributeSet_Character::UAttributeSet_Character()
{
	InitInventorySlotCount(8.f);
	InitOxygenDrainMultiplier(1.f);
	InitOreBonusDropChance(0.f);
	InitCoinGainMultiplier(1.f);
	InitDeathCoinLossRatio(0.5f);
	InitDeathItemLossRatio(0.5f);
	InitInventoryBurdenMoveSpeedPenaltyRate(0.f);
	InitSurroundDamageRatio(0.f);
	InitDrillPeriodPerOxygenIncreaseRate(0.f);
}

void UAttributeSet_Character::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentOxygenAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxOxygen());
	}
	else if (Attribute == GetInventorySlotCountAttribute())
	{
		NewValue = FMath::Max(1.f, NewValue);
	}
	else if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetHealthRegenRateAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetMaxOxygenAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetOxygenDrainMultiplierAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetOreBonusDropChanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	}
	else if (Attribute == GetCoinGainMultiplierAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetDeathCoinLossRatioAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	}
	else if (Attribute == GetDeathItemLossRatioAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	}
	else if (Attribute == GetInventoryBurdenMoveSpeedPenaltyRateAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetSurroundDamageRatioAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
}

void UAttributeSet_Character::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentOxygenAttribute())
	{
		SetCurrentOxygen(FMath::Clamp(GetCurrentOxygen(), 0.f, GetMaxOxygen()));
	}
	else if (Data.EvaluatedData.Attribute == GetInventorySlotCountAttribute())
	{
		SetInventorySlotCount(FMath::Max(1.f, GetInventorySlotCount()));
	}
	else if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(0.f, GetMaxHealth()));
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetHealthRegenRateAttribute())
	{
		SetHealthRegenRate(FMath::Max(0.f, GetHealthRegenRate()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxOxygenAttribute())
	{
		SetMaxOxygen(FMath::Max(0.f, GetMaxOxygen()));
		SetCurrentOxygen(FMath::Clamp(GetCurrentOxygen(), 0.f, GetMaxOxygen()));
	}
	else if (Data.EvaluatedData.Attribute == GetOxygenDrainMultiplierAttribute())
	{
		SetOxygenDrainMultiplier(FMath::Max(0.f, GetOxygenDrainMultiplier()));
	}
	else if (Data.EvaluatedData.Attribute == GetOreBonusDropChanceAttribute())
	{
		SetOreBonusDropChance(FMath::Clamp(GetOreBonusDropChance(), 0.f, 1.f));
	}
	else if (Data.EvaluatedData.Attribute == GetCoinGainMultiplierAttribute())
	{
		SetCoinGainMultiplier(FMath::Max(0.f, GetCoinGainMultiplier()));
	}
	else if (Data.EvaluatedData.Attribute == GetDeathCoinLossRatioAttribute())
	{
		SetDeathCoinLossRatio(FMath::Clamp(GetDeathCoinLossRatio(), 0.f, 1.f));
	}
	else if (Data.EvaluatedData.Attribute == GetDeathItemLossRatioAttribute())
	{
		SetDeathItemLossRatio(FMath::Clamp(GetDeathItemLossRatio(), 0.f, 1.f));
	}
	else if (Data.EvaluatedData.Attribute == GetInventoryBurdenMoveSpeedPenaltyRateAttribute())
	{
		SetInventoryBurdenMoveSpeedPenaltyRate(FMath::Max(0.f, GetInventoryBurdenMoveSpeedPenaltyRate()));
	}
	else if (Data.EvaluatedData.Attribute == GetSurroundDamageRatioAttribute())
	{
		SetSurroundDamageRatio(FMath::Max(0.f, GetSurroundDamageRatio()));
	}
}

float UAttributeSet_Character::CalculateEffectiveDrillingPeriod() const
{
	const float MaxOxygenValue = GetMaxOxygen();
	const float OxygenRatio = MaxOxygenValue > 0.f
		? FMath::Clamp(GetCurrentOxygen() / MaxOxygenValue, 0.f, 1.f)
		: 0.f;
	const float OxygenDepletionRatio = 1.f - OxygenRatio;
	const float AppliedDrillingPeriodIncreaseRate = GetDrillPeriodPerOxygenIncreaseRate() * OxygenDepletionRatio;

	return GetDrillingPeriod() * FMath::Max(1.f + AppliedDrillingPeriodIncreaseRate, KINDA_SMALL_NUMBER);
}
