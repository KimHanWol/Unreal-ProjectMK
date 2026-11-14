#include "ProjectMK/UI/SkillConnectionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Rendering/DrawElementTypes.h"

TSharedRef<SWidget> USkillConnectionWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		WidgetTree->RootWidget = WidgetTree->ConstructWidget<USizeBox>();
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	return Super::RebuildWidget();
}

int32 USkillConnectionWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& DrawElements, int32 LayerId, const FWidgetStyle& Style, bool bEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, DrawElements, LayerId, Style, bEnabled);
	if (LinePoints.Num() < 2) { return Layer; }
	TArray<FVector2D> ShadowPoints = LinePoints;
	for (FVector2D& Point : ShadowPoints) { Point += FVector2D(1.f, 2.f); }
	const FLinearColor Tint = Style.GetColorAndOpacityTint();
	FSlateDrawElement::MakeLines(DrawElements, Layer + 1, Geometry.ToPaintGeometry(), ShadowPoints,
		ESlateDrawEffect::None, FLinearColor(0.01f, 0.015f, 0.02f, 0.75f) * Tint, true, 9.f);
	FSlateDrawElement::MakeLines(DrawElements, Layer + 2, Geometry.ToPaintGeometry(), LinePoints,
		ESlateDrawEffect::None, FLinearColor(0.035f, 0.05f, 0.06f) * Tint, true, 7.f);
	FSlateDrawElement::MakeLines(DrawElements, Layer + 3, Geometry.ToPaintGeometry(), LinePoints,
		ESlateDrawEffect::None, LineColor * Tint, true, 4.f);
	const float HighlightStrength = FMath::Max3(LineColor.R, LineColor.G, LineColor.B) > 0.3f ? 0.25f : 0.06f;
	const FLinearColor Highlight = FMath::Lerp(LineColor, FLinearColor(0.9f, 0.94f, 0.85f), HighlightStrength);
	FSlateDrawElement::MakeLines(DrawElements, Layer + 4, Geometry.ToPaintGeometry(), LinePoints,
		ESlateDrawEffect::None, Highlight * Tint, true, 1.f);
	return Layer + 4;
}

void USkillConnectionWidget::InitializePath(const TArray<FVector2D>& Points)
{
	LinePoints = Points;
	InvalidateLayoutAndVolatility();
}

void USkillConnectionWidget::SetLineColor(const FLinearColor& Color)
{
	LineColor = Color;
	InvalidateLayoutAndVolatility();
}

const FLinearColor& USkillConnectionWidget::GetLineColor() const
{
	return LineColor;
}
