// LINK

#include "ProjectMK/UI/HUDWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"

void UHUDWidget::BindEvents()
{
	Super::BindEvents();

	if (::IsValid(OwnerASC) == false)
	{
		return;
	}

	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetMaxHealthAttribute()).AddUObject(this, &::UHUDWidget::OnMaxHealthChanged);
	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).AddUObject(this, &::UHUDWidget::OnCurrentHealthChanged);
}

void UHUDWidget::UnbindEvents()
{
	Super::UnbindEvents();

	if (::IsValid(OwnerASC) == false)
	{
		return;
	}

	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetMaxHealthAttribute()).RemoveAll(this);
	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).RemoveAll(this);
}

void UHUDWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	UpdateHealthProgressBar();
}

void UHUDWidget::OnCurrentHealthChanged(const FOnAttributeChangeData& Data)
{
	UpdateHealthProgressBar();
}

void UHUDWidget::UpdateHealthProgressBar()
{
	if (::IsValid(ProgressBar_Health) == false)
	{
		return;
	}

	if (::IsValid(OwnerASC) == false)
	{
		return;
	}

	const UAttributeSet_Character* AttributeSet_Character = Cast<UAttributeSet_Character>(OwnerASC->GetAttributeSet(UAttributeSet_Character::StaticClass()));
	if (::IsValid(AttributeSet_Character) == false)
	{
		return;
	}

	float HealthRatio = AttributeSet_Character->GetCurrentHealth() / AttributeSet_Character->GetMaxHealth();
	ProgressBar_Health->SetPercent(HealthRatio);
}
