// LINK

#include "ProjectMK/UI/MKSlotBase.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Input/Reply.h"
#include "PaperSprite.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "ProjectMK/UI/MKTooltipBase.h"

TSharedRef<SWidget> UMKSlotBase::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

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

void UMKSlotBase::SetSlotIcon(UPaperSprite* IconSprite)
{
	if (::IsValid(IconImage) == false)
	{
		return;
	}

	IconImage->SetBrushResourceObject(IconSprite);
	IconImage->SetVisibility(::IsValid(IconSprite) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UMKSlotBase::SetSlotIcon(UTexture2D* IconTexture)
{
	if (::IsValid(IconImage) == false)
	{
		return;
	}

	IconImage->SetBrushFromTexture(IconTexture, true);
	IconImage->SetVisibility(::IsValid(IconTexture) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UMKSlotBase::SetSlotCount(const int32 Count, const bool bShowCount)
{
	if (::IsValid(CountText))
	{
		CountText->SetText(bShowCount ? FText::AsNumber(Count) : FText::GetEmpty());
		CountText->SetVisibility(bShowCount ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UMKSlotBase::SetKeyGuide(const FText& KeyGuide)
{
	if (::IsValid(KeyGuideText))
	{
		KeyGuideText->SetText(KeyGuide);
	}
	if (::IsValid(KeyGuideBorder))
	{
		KeyGuideBorder->SetVisibility(KeyGuide.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UMKSlotBase::SetSelected(const bool bSelected)
{
	if (::IsValid(SelectionText))
	{
		SelectionText->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UMKSlotBase::ClearSlotIcon()
{
	if (::IsValid(IconImage))
	{
		IconImage->SetBrush(FSlateBrush());
		IconImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMKSlotBase::BuildDefaultWidgetTree()
{
	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>();
	RootSize->SetWidthOverride(DefaultSlotSize.X);
	RootSize->SetHeightOverride(DefaultSlotSize.Y);
	WidgetTree->RootWidget = RootSize;

	UBorder* FrameBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Slot"));
	FrameBorder->SetPadding(FMargin(FMKUILayout::SpaceXS));
	RootSize->SetContent(FrameBorder);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
	FrameBorder->SetContent(Overlay);

	IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
	IconImage->SetDesiredSizeOverride(FVector2D(40.f));
	IconImage->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(IconImage);
	IconSlot->SetHorizontalAlignment(HAlign_Center);
	IconSlot->SetVerticalAlignment(VAlign_Center);

	FocusImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectedSlot"));
	FocusImage->SetVisibility(ESlateVisibility::Collapsed);
	Overlay->AddChildToOverlay(FocusImage);

	CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountText"));
	ApplyTextRole(CountText, EMKTextRole::Caption);
	CountText->SetJustification(ETextJustify::Right);
	CountText->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* CountSlot = Overlay->AddChildToOverlay(CountText);
	CountSlot->SetHorizontalAlignment(HAlign_Right);
	CountSlot->SetVerticalAlignment(VAlign_Bottom);
	CountSlot->SetPadding(FMargin(0.f, 0.f, 1.f, 1.f));

	KeyGuideText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KeyGuideText"));
	ApplyTextRole(KeyGuideText, EMKTextRole::Caption);
	KeyGuideText->SetJustification(ETextJustify::Center);
	KeyGuideBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Keycap"));
	KeyGuideBorder->SetPadding(FMargin(FMKUILayout::SpaceXS, 0.f));
	KeyGuideBorder->SetContent(KeyGuideText);
	KeyGuideBorder->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* KeySlot = Overlay->AddChildToOverlay(KeyGuideBorder);
	KeySlot->SetHorizontalAlignment(HAlign_Center);
	KeySlot->SetVerticalAlignment(VAlign_Bottom);
	KeySlot->SetPadding(FMargin(0.f, 0.f, 0.f, -5.f));

	SelectionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectionText"));
	SelectionText->SetText(FText::FromString(TEXT("✓")));
	ApplyTextRole(SelectionText, EMKTextRole::Label);
	SelectionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 1.f, 0.72f)));
	SelectionText->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* SelectionSlot = Overlay->AddChildToOverlay(SelectionText);
	SelectionSlot->SetHorizontalAlignment(HAlign_Right);
	SelectionSlot->SetVerticalAlignment(VAlign_Top);
}

void UMKSlotBase::UpdateFocusImageVisibility()
{
	if (::IsValid(FocusImage) == false)
	{
		return;
	}

	FocusImage->SetVisibility(bHasSlotFocus ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
