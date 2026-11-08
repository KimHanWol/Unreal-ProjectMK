#include "ProjectMK/UI/PlaytestQuickSlotWidget.h"

UPlaytestQuickSlotWidget::UPlaytestQuickSlotWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SlotContainer = EItemContainer::QuickSlot;
}

void UPlaytestQuickSlotWidget::SetShowKeyGuide(bool bInShowKeyGuide)
{
	bShowKeyGuide = bInShowKeyGuide;
	UpdateWidget();
}

void UPlaytestQuickSlotWidget::RefreshSlot()
{
	UpdateWidget();
}
