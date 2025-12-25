// LINK

#include "ProjectMK/UI/CurrencyWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"

void UCurrencyWidget::BindEvents()
{
	Super::BindEvents();

	if (::IsValid(OwnerASC) == false)
	{
		return;
	}

	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCoinAttribute()).AddUObject(this, &UCurrencyWidget::OnCoinChanged);
}

void UCurrencyWidget::UnbindEvents()
{
	Super::UnbindEvents();

	if (::IsValid(OwnerASC) == false)
	{
		return;
	}

	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCoinAttribute()).RemoveAll(this);
}

void UCurrencyWidget::OnCoinChanged(const FOnAttributeChangeData& Data)
{
	if (::IsValid(Text_Coin) == false)
	{
		Text_Coin->SetText(FText::AsNumber(Data.NewValue));
	}
}