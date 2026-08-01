// LINK

#include "ProjectMK/UI/MKSlotBase.h"

#include "Components/Image.h"
#include "InputCoreTypes.h"
#include "Input/Reply.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "ProjectMK/UI/MKTooltipBase.h"

void UMKSlotBase::UpdateWidget()
{
	UpdateFocusImageVisibility();
}

FReply UMKSlotBase::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return HandleSlotClicked(InGeometry, InMouseEvent);
}

FReply UMKSlotBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return HandleSlotClicked(InGeometry, InMouseEvent);
}

void UMKSlotBase::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	bHasSlotFocus = true;
	OnSlotHovered();
}

void UMKSlotBase::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	bHasSlotFocus = false;
	OnSlotUnhovered();
}

FReply UMKSlotBase::HandleSlotClicked(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const bool bIsLeftMouseButton = InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
	if (bIsLeftMouseButton == false || CanClickSlot() == false)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	OnSlotClicked();
	return FReply::Handled();
}

bool UMKSlotBase::CanClickSlot() const
{
	return true;
}

void UMKSlotBase::OnSlotClicked()
{
}

void UMKSlotBase::OnSlotHovered()
{
	UpdateFocusImageVisibility();

	if (UHUDWidget* HUDWidget = GetHUDWidget())
	{
		HUDWidget->SetHoveredSlot(this);
	}
}

void UMKSlotBase::OnSlotUnhovered()
{
	UpdateFocusImageVisibility();

	if (UHUDWidget* HUDWidget = GetHUDWidget())
	{
		HUDWidget->ClearHoveredSlot(this);
	}
}

void UMKSlotBase::UpdateTooltipWidget(UMKTooltipBase* TooltipWidget) const
{
}

void UMKSlotBase::UpdateFocusImageVisibility()
{
	if (::IsValid(FocusImage) == false)
	{
		return;
	}

	FocusImage->SetVisibility(bHasSlotFocus ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
