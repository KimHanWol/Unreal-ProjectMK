#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "SkillConnectionWidget.generated.h"

UCLASS()
class PROJECTMK_API USkillConnectionWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& DrawElements, int32 LayerId, const FWidgetStyle& Style, bool bEnabled) const override;

public:
	void InitializePath(const TArray<FVector2D>& Points);
	void SetLineColor(const FLinearColor& Color);
	const FLinearColor& GetLineColor() const;

private:
	TArray<FVector2D> LinePoints;
	FLinearColor LineColor = FLinearColor::White;
};
