#include "ProjectMK/UI/DrillReachWidget.h"

#include "Blueprint/WidgetTree.h"
#include "PaperSprite.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"

TSharedRef<SWidget> UDrillReachWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget == nullptr)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(144.f); Size->SetHeightOverride(48.f); WidgetTree->RootWidget = Size;
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(); Size->SetContent(Canvas);
		Shaft = WidgetTree->ConstructWidget<USizeBox>(); Shaft->SetHeightOverride(10.f);
		auto* ShaftSlot = Canvas->AddChildToCanvas(Shaft); ShaftSlot->SetPosition(FVector2D(4.f,13.f)); ShaftSlot->SetAutoSize(true);
		UBorder* Metal = WidgetTree->ConstructWidget<UBorder>(); ApplyBorderStyle(Metal,TEXT("ButtonDisabled")); Metal->SetPadding(FMargin(2.f)); Shaft->SetContent(Metal);
		DrillTip = WidgetTree->ConstructWidget<UImage>();
		const auto* Style = UDataManager::Get(this)->GetVisualStyleDataAsset();
		DrillTip->SetBrushResourceObject(Style != nullptr ? Style->TownSprites.FindRef(TEXT("DrillReach")) : nullptr); DrillTip->SetRenderTransformAngle(90.f);
		auto* TipSlot = Canvas->AddChildToCanvas(DrillTip); TipSlot->SetSize(FVector2D(36.f)); TipSlot->SetPosition(FVector2D(16.f,0.f));
		Readout = WidgetTree->ConstructWidget<UTextBlock>(); ApplyTextRole(Readout,EMKTextRole::Caption); Readout->SetJustification(ETextJustify::Center);
		auto* TextSlot = Canvas->AddChildToCanvas(Readout); TextSlot->SetPosition(FVector2D(0.f,32.f)); TextSlot->SetSize(FVector2D(100.f,16.f));
	}
	return Super::RebuildWidget();
}

void UDrillReachWidget::SetReach(int32 Cells, int32 Maximum)
{
	SetVisibility(Maximum > 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Shaft == nullptr) { return; }
	const float Length = 12.f + 12.f*FMath::Min(Cells,7);
	Shaft->SetWidthOverride(Length);
	CastChecked<UCanvasPanelSlot>(DrillTip->Slot)->SetPosition(FVector2D(Length-12.f,0.f));
	Readout->SetText(FText::Format(NSLOCTEXT("Drill","ReachCount","{0} / {1}"),FText::AsNumber(Cells),FText::AsNumber(Maximum)));
}
