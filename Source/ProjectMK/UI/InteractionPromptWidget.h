#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "InteractionPromptWidget.generated.h"

UCLASS()
class PROJECTMK_API UInteractionPromptWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& DrawElements, int32 LayerId, const FWidgetStyle& Style, bool bEnabled) const override;
	void SetRecoveryState(float Progress, bool bAvailable, const FText& BlockedReason);

private:
	void BuildDefaultWidgetTree();

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> RecoveryHint;
	float RecoveryProgress = 0.f;
};
