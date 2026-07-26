// LINK

#include "ProjectMK/UI/MKSlotBase.h"

#include "InputCoreTypes.h"
#include "Input/Reply.h"

FReply UMKSlotBase::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return HandleSlotClicked(InGeometry, InMouseEvent);
}

FReply UMKSlotBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return HandleSlotClicked(InGeometry, InMouseEvent);
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
