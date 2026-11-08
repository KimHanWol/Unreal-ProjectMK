// LINK

#include "ProjectMK/UI/CurrencyWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
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
	Panel->SetPadding(FMargin(12.f, 8.f));
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetMinDesiredWidth(132.f);
	Size->SetContent(Panel);
	WidgetTree->RootWidget = Size;
	UTextBlock* RawAmountText = nullptr;
	UHorizontalBox* CurrencyRow = BuildCurrencyAmount(RawAmountText, FText::GetEmpty(), EMKTextRole::HUDMetric, 32.f);
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
