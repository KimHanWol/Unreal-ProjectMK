// LINK

#include "ProjectMK/UI/CurrencyWidget.h"

#include "AbilitySystemComponent.h"
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
	RefreshCoinText();
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

void UCurrencyWidget::RefreshCoinText()
{
	const UAttributeSet_Character* CharacterAttributeSet = GetCharacterAttributeSet();
	if (::IsValid(Text_Coin) && ::IsValid(CharacterAttributeSet))
	{
		Text_Coin->SetText(FText::AsNumber(CharacterAttributeSet->GetCoin()));
	}
}

void UCurrencyWidget::OnCoinChanged(const FOnAttributeChangeData& Data)
{
	RefreshCoinText();
}
