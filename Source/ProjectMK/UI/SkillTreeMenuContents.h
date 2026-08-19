#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKMenuContentsBase.h"

#include "SkillTreeMenuContents.generated.h"

class UCanvasPanel;

UCLASS()
class PROJECTMK_API USkillTreeMenuContents : public UMKMenuContentsBase
{
	GENERATED_BODY()

public:
	bool BeginSkillTreeDrag(const FVector2D& ScreenSpacePosition);
	bool HasActiveSkillTreeDrag() const;
	bool UpdateSkillTreeDrag(const FVector2D& ScreenSpacePosition);
	void EndSkillTreeDrag();

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	bool UpdateSkillTreeZoom(const FGeometry& InGeometry, const FVector2D& ScreenSpacePosition, float WheelDelta);
	float CalculateMinimumZoomScale() const;
	FVector2D CalculateClampedCanvasPosition(const FVector2D& TargetPosition, float ContentScale) const;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> SkillTreeCanvasPanel;

	UPROPERTY(EditDefaultsOnly, Category = "Skill Tree|Zoom", meta = (ClampMin = "0.01"))
	float ZoomStep = 0.1f;

private:
	UPROPERTY(Transient)
	float CurrentZoomScale = 1.f;

	UPROPERTY(Transient)
	bool bIsDraggingSkillTree = false;

	UPROPERTY(Transient)
	FVector2D DragStartScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	FVector2D DragStartCanvasPosition = FVector2D::ZeroVector;
};
