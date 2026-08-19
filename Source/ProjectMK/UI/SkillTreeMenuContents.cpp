// LINK

#include "ProjectMK/UI/SkillTreeMenuContents.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Input/Reply.h"

bool USkillTreeMenuContents::BeginSkillTreeDrag(const FVector2D& ScreenSpacePosition)
{
	UCanvasPanelSlot* CanvasPanelSlot = ::IsValid(SkillTreeCanvasPanel) ? Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot) : nullptr;
	if (::IsValid(CanvasPanelSlot) == false)
	{
		return false;
	}

	DragStartScreenPosition = ScreenSpacePosition;
	DragStartCanvasPosition = CanvasPanelSlot->GetPosition();
	bIsDraggingSkillTree = true;
	return true;
}

bool USkillTreeMenuContents::HasActiveSkillTreeDrag() const
{
	return bIsDraggingSkillTree;
}

bool USkillTreeMenuContents::UpdateSkillTreeDrag(const FVector2D& ScreenSpacePosition)
{
	UCanvasPanelSlot* CanvasPanelSlot = ::IsValid(SkillTreeCanvasPanel) ? Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot) : nullptr;
	if (bIsDraggingSkillTree == false || ::IsValid(CanvasPanelSlot) == false)
	{
		return false;
	}

	const FVector2D DragDelta = ScreenSpacePosition - DragStartScreenPosition;
	const FVector2D TargetPosition = DragStartCanvasPosition + DragDelta;
	CanvasPanelSlot->SetPosition(CalculateClampedCanvasPosition(TargetPosition, CurrentZoomScale));
	return true;
}

void USkillTreeMenuContents::EndSkillTreeDrag()
{
	bIsDraggingSkillTree = false;
}

FReply USkillTreeMenuContents::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && BeginSkillTreeDrag(InMouseEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USkillTreeMenuContents::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingSkillTree && UpdateSkillTreeDrag(InMouseEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply USkillTreeMenuContents::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDraggingSkillTree)
	{
		EndSkillTreeDrag();
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply USkillTreeMenuContents::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (UpdateSkillTreeZoom(InGeometry, InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

bool USkillTreeMenuContents::UpdateSkillTreeZoom(const FGeometry& InGeometry, const FVector2D& ScreenSpacePosition, const float WheelDelta)
{
	UCanvasPanelSlot* CanvasPanelSlot = ::IsValid(SkillTreeCanvasPanel) ? Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot) : nullptr;
	if (::IsValid(CanvasPanelSlot) == false || FMath::IsNearlyZero(WheelDelta) || ZoomStep <= 0.f)
	{
		return false;
	}

	const float MinimumZoomScale = CalculateMinimumZoomScale();
	const float NewZoomScale = FMath::Clamp(CurrentZoomScale + (WheelDelta * ZoomStep), MinimumZoomScale, 1.f);
	if (FMath::IsNearlyEqual(NewZoomScale, CurrentZoomScale))
	{
		return true;
	}

	const FVector2D CursorPosition = InGeometry.AbsoluteToLocal(ScreenSpacePosition);
	const FVector2D CurrentContentPosition = InGeometry.AbsoluteToLocal(SkillTreeCanvasPanel->GetCachedGeometry().GetAbsolutePosition());
	const FVector2D ContentPositionUnderCursor = (CursorPosition - CurrentContentPosition) / CurrentZoomScale;
	const FVector2D TargetContentPosition = CursorPosition - (ContentPositionUnderCursor * NewZoomScale);
	const FVector2D TargetCanvasPosition = CanvasPanelSlot->GetPosition() + (TargetContentPosition - CurrentContentPosition);

	SkillTreeCanvasPanel->SetRenderTransformPivot(FVector2D::ZeroVector);
	SkillTreeCanvasPanel->SetRenderScale(FVector2D(NewZoomScale));
	CurrentZoomScale = NewZoomScale;
	CanvasPanelSlot->SetPosition(CalculateClampedCanvasPosition(TargetCanvasPosition, CurrentZoomScale));

	if (bIsDraggingSkillTree)
	{
		DragStartScreenPosition = ScreenSpacePosition;
		DragStartCanvasPosition = CanvasPanelSlot->GetPosition();
	}

	return true;
}

float USkillTreeMenuContents::CalculateMinimumZoomScale() const
{
	if (::IsValid(SkillTreeCanvasPanel) == false)
	{
		return 1.f;
	}

	const FVector2D ViewSize = GetCachedGeometry().GetLocalSize();
	FVector2D ContentSize = SkillTreeCanvasPanel->GetCachedGeometry().GetLocalSize();
	if (ContentSize.IsNearlyZero())
	{
		ContentSize = SkillTreeCanvasPanel->GetDesiredSize();
	}

	if (ContentSize.IsNearlyZero())
	{
		const UCanvasPanelSlot* CanvasPanelSlot = Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot);
		ContentSize = ::IsValid(CanvasPanelSlot) ? CanvasPanelSlot->GetSize() : FVector2D::ZeroVector;
	}

	if (ViewSize.IsNearlyZero() || ContentSize.IsNearlyZero())
	{
		return 1.f;
	}

	const float FitScale = FMath::Min(ViewSize.X / ContentSize.X, ViewSize.Y / ContentSize.Y);
	return FMath::Clamp(FitScale, UE_KINDA_SMALL_NUMBER, 1.f);
}

FVector2D USkillTreeMenuContents::CalculateClampedCanvasPosition(const FVector2D& TargetPosition, const float ContentScale) const
{
	UCanvasPanelSlot* CanvasPanelSlot = ::IsValid(SkillTreeCanvasPanel) ? Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot) : nullptr;
	if (::IsValid(SkillTreeCanvasPanel) == false || ::IsValid(CanvasPanelSlot) == false)
	{
		return TargetPosition;
	}

	const FVector2D ViewSize = GetCachedGeometry().GetLocalSize();
	FVector2D ContentSize = SkillTreeCanvasPanel->GetCachedGeometry().GetLocalSize();
	if (ContentSize.IsNearlyZero())
	{
		ContentSize = SkillTreeCanvasPanel->GetDesiredSize();
	}

	if (ContentSize.IsNearlyZero())
	{
		ContentSize = CanvasPanelSlot->GetSize();
	}
	ContentSize *= ContentScale;

	const FVector2D CurrentPosition = CanvasPanelSlot->GetPosition();
	const FVector2D PositionDelta = TargetPosition - CurrentPosition;
	const FVector2D CurrentContentPosition = GetCachedGeometry().AbsoluteToLocal(SkillTreeCanvasPanel->GetCachedGeometry().GetAbsolutePosition());
	const FVector2D TargetContentPosition = CurrentContentPosition + PositionDelta;

	FVector2D MinContentPosition = CurrentContentPosition;
	FVector2D MaxContentPosition = CurrentContentPosition;

	if (ContentSize.X > ViewSize.X)
	{
		MinContentPosition.X = ViewSize.X - ContentSize.X;
		MaxContentPosition.X = 0.f;
	}
	else
	{
		MinContentPosition.X = (ViewSize.X - ContentSize.X) * 0.5f;
		MaxContentPosition.X = MinContentPosition.X;
	}

	if (ContentSize.Y > ViewSize.Y)
	{
		MinContentPosition.Y = ViewSize.Y - ContentSize.Y;
		MaxContentPosition.Y = 0.f;
	}
	else
	{
		MinContentPosition.Y = (ViewSize.Y - ContentSize.Y) * 0.5f;
		MaxContentPosition.Y = MinContentPosition.Y;
	}

	const FVector2D ClampedContentPosition(
		FMath::Clamp(TargetContentPosition.X, MinContentPosition.X, MaxContentPosition.X),
		FMath::Clamp(TargetContentPosition.Y, MinContentPosition.Y, MaxContentPosition.Y)
	);

	return TargetPosition + (ClampedContentPosition - TargetContentPosition);
}
