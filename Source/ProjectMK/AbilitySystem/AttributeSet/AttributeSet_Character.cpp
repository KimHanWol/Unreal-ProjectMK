// LINK

#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"

#include "GameplayEffectExtension.h"

UAttributeSet_Character::UAttributeSet_Character()
{
}

void UAttributeSet_Character::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentOxygenAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxOxygen());
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
}

void UAttributeSet_Character::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentOxygenAttribute())
	{
		SetCurrentOxygen(FMath::Clamp(GetCurrentOxygen(), 0.f, GetMaxOxygen()));
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
}
