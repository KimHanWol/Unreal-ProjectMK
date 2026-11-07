// LINK

#include "ProjectMK/UI/CurrencyWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"

TSharedRef<SWidget> UCurrencyWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->FindWidget(TEXT("CurrencyPanel")) == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

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

void UCurrencyWidget::BuildDefaultWidgetTree()
{
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CurrencyPanel"));
	Panel->SetPadding(FMargin(FMKUILayout::SpaceS, FMKUILayout::SpaceXS));
	WidgetTree->RootWidget = Panel;
	UTextBlock* RawAmountText = nullptr;
	UHorizontalBox* CurrencyRow = BuildCurrencyAmount(RawAmountText, FText::GetEmpty());
	Text_Coin = RawAmountText;
	Panel->SetContent(CurrencyRow);
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
