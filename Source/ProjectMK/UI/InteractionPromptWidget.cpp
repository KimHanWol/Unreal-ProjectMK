// LINK

#include "ProjectMK/UI/InteractionPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Rendering/DrawElements.h"

TSharedRef<SWidget> UInteractionPromptWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

int32 UInteractionPromptWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& DrawElements, int32 LayerId, const FWidgetStyle& Style, bool bEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, DrawElements, LayerId, Style, bEnabled);
	if (RecoveryProgress <= 0.f) { return Layer; }
	const FVector2D Center(Geometry.GetLocalSize().X * 0.5f, 18.f);
	const TArray<FVector2D> Track = { Center + FVector2D(0.f,-16.f), Center + FVector2D(16.f,-16.f), Center + FVector2D(16.f,16.f), Center + FVector2D(-16.f,16.f), Center + FVector2D(-16.f,-16.f), Center + FVector2D(0.f,-16.f) };
	TArray<FVector2D> Progress = { Track[0] };
	float Remaining = RecoveryProgress * 128.f;
	for (int32 Index = 1; Index < Track.Num() && Remaining > 0.f; ++Index)
	{
		const float Length = FVector2D::Distance(Track[Index-1],Track[Index]);
		Progress.Add(FMath::Lerp(Track[Index-1],Track[Index],FMath::Min(1.f,Remaining/Length)));
		Remaining -= Length;
	}
	FSlateDrawElement::MakeLines(DrawElements, Layer + 1, Geometry.ToPaintGeometry(), Track, ESlateDrawEffect::None, FLinearColor(0.08f,0.055f,0.025f), false, 4.f);
	if (Progress.Num() > 1) { FSlateDrawElement::MakeLines(DrawElements, Layer + 2, Geometry.ToPaintGeometry(), Progress, ESlateDrawEffect::None, FLinearColor(0.94f,0.68f,0.26f), false, 2.f); }
	return Layer + 2;
}

void UInteractionPromptWidget::SetRecoveryState(float Progress, bool bAvailable, const FText& BlockedReason)
{
	RecoveryProgress = bAvailable ? FMath::Clamp(Progress, 0.f, 1.f) : 0.f;
	if (RecoveryHint != nullptr)
	{
		RecoveryHint->SetText(FText::GetEmpty());
		RecoveryHint->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInteractionPromptWidget::BuildDefaultWidgetTree()
{
	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InteractionPrompt"));
	RootSize->SetWidthOverride(144.f);
	RootSize->SetHeightOverride(56.f);
	WidgetTree->RootWidget = RootSize;

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
	RootSize->SetContent(Stack);
	USizeBox* KeySize = WidgetTree->ConstructWidget<USizeBox>();
	KeySize->SetWidthOverride(36.f);
	KeySize->SetHeightOverride(36.f);
	Stack->AddChildToVerticalBox(KeySize)->SetHorizontalAlignment(HAlign_Center);
	UBorder* Keycap = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Keycap"));
	Keycap->SetPadding(FMargin(FMKUILayout::SpaceXS));
	KeySize->SetContent(Keycap);

	UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KeyText"));
	KeyText->SetText(FText::FromString(TEXT("F")));
	KeyText->SetJustification(ETextJustify::Center);
	ApplyTextRole(KeyText, EMKTextRole::Button);
	Keycap->SetContent(KeyText);
	RecoveryHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RecoveryHint"));
	ApplyTextRole(RecoveryHint, EMKTextRole::Caption);
	FSlateFontInfo Font = RecoveryHint->GetFont();
	Font.Size = 10;
	RecoveryHint->SetFont(Font);
	RecoveryHint->SetJustification(ETextJustify::Center);
	RecoveryHint->SetShadowOffset(FVector2D(1.f, 1.f));
	RecoveryHint->SetShadowColorAndOpacity(FLinearColor::Black);
	RecoveryHint->SetVisibility(ESlateVisibility::Collapsed);
	Stack->AddChildToVerticalBox(RecoveryHint)->SetHorizontalAlignment(HAlign_Center);
}
