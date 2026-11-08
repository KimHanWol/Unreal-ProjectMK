#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKMenuContentsBase.h"

#include "SkillTreeMenuContents.generated.h"

class UCanvasPanel;
class USkillSlotWidget;

USTRUCT()
struct FSkillConnectionVisual
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	FName TargetSkillKey;

	UPROPERTY(Transient)
	FName SourceSkillKey;

	UPROPERTY(Transient)
	TArray<FVector2D> RoutePoints;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> Elements;
};

UCLASS()
class PROJECTMK_API USkillTreeMenuContents : public UMKMenuContentsBase
{
	GENERATED_BODY()

public:
	USkillTreeMenuContents(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& Event) override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

public:
	bool BeginSkillTreeDrag(const FVector2D& ScreenSpacePosition);
	bool HasActiveSkillTreeDrag() const;
	bool UpdateSkillTreeDrag(const FVector2D& ScreenSpacePosition);
	void EndSkillTreeDrag();

private:
	void BuildDefaultWidgetTree();
	void RefreshSkillTree();
	void AddSkillConnection(const FVector2D& FromPosition, const FVector2D& ToPosition, FName SourceSkillKey, FName TargetSkillKey, const TMap<FName, FVector2D>& NodePositions);
	void UpdateConnectionStates();
	TArray<FVector2D> CalculateConnectionPath(const FVector2D& FromPosition, const FVector2D& ToPosition, const TMap<FName, FVector2D>& NodePositions);
	bool UpdateSkillTreeZoom(const FGeometry& InGeometry, const FVector2D& ScreenSpacePosition, float WheelDelta);
	float CalculateMinimumZoomScale() const;
	FVector2D CalculateClampedCanvasPosition(const FVector2D& TargetPosition, float ContentScale) const;
	void OnSkillPurchased(FName SkillKey);
	UFUNCTION()
	void OnMenuVisibilityChanged(ESlateVisibility InVisibility);

protected:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> SkillTreeViewportPanel;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> SkillTreeCanvasPanel;

	UPROPERTY(EditDefaultsOnly, Category = "Skill Tree")
	TSubclassOf<USkillSlotWidget> SkillSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Skill Tree|Zoom", meta = (ClampMin = "0.01"))
	float ZoomStep = 0.1f;

private:
	UPROPERTY(Transient)
	TArray<FSkillConnectionVisual> Connections;
	TSet<FIntPoint> OccupiedConnectionCells;

	bool bNeedsCentering = true;
	FVector2D CentralPlayerPosition;

	UPROPERTY(Transient)
	float CurrentZoomScale = 1.f;

	UPROPERTY(Transient)
	bool bIsDraggingSkillTree = false;

	UPROPERTY(Transient)
	FVector2D DragStartScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	FVector2D DragStartCanvasPosition = FVector2D::ZeroVector;
};
