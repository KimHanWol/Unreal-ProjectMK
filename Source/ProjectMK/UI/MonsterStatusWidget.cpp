#include "ProjectMK/UI/MonsterStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UMonsterStatusWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>();
		Frame->SetWidthOverride(112.f);
		WidgetTree->RootWidget = Frame;
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Frame->SetContent(Column);
		StatusText = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = StatusText->GetFont();
		Font.Size = 10;
		StatusText->SetFont(Font);
		StatusText->SetJustification(ETextJustify::Center);
		StatusText->SetShadowOffset(FVector2D(1.f, 1.f));
		StatusText->SetShadowColorAndOpacity(FLinearColor::Black);
		Column->AddChildToVerticalBox(StatusText);
		USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>();
		BarSize->SetHeightOverride(4.f);
		Column->AddChildToVerticalBox(BarSize)->SetPadding(FMargin(18.f, 2.f, 18.f, 0.f));
		HealthBar = WidgetTree->ConstructWidget<UProgressBar>();
		BarSize->SetContent(HealthBar);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	return Super::RebuildWidget();
}

void UMonsterStatusWidget::UpdateStatus(const FText& Text, float HealthRatio, const FLinearColor& Color)
{
	TakeWidget();
	if (::IsValid(StatusText) && ::IsValid(HealthBar))
	{
		StatusText->SetText(Text);
		StatusText->SetColorAndOpacity(FSlateColor(Color));
		HealthBar->SetPercent(FMath::Clamp(HealthRatio, 0.f, 1.f));
		HealthBar->SetFillColorAndOpacity(Color);
	}
}
