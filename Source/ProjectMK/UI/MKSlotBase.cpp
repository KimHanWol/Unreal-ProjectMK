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

void UMKSlotBase::SetSlotLocked(const bool bLocked)
{
	bIsSlotLocked = bLocked;
	if (::IsValid(LockImage))
	{
		LockImage->SetVisibility(bLocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	SetRenderOpacity(bLocked ? 0.45f : 1.f);
}

void UMKSlotBase::SetRankBadge(const FText& Rank)
{
	if (::IsValid(RankText)) { RankText->SetText(Rank); }
	if (::IsValid(RankBorder)) { RankBorder->SetVisibility(Rank.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible); }
}

void UMKSlotBase::SetAcquired(bool bAcquired)
{
	const ESlateVisibility State = bAcquired ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (::IsValid(AcquiredImage)) { AcquiredImage->SetVisibility(State); }
	if (::IsValid(AcquiredSeal)) { AcquiredSeal->SetVisibility(State); }
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

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
	RootSize->SetContent(Overlay);
	SetVisibility(ESlateVisibility::Visible);
	Overlay->SetClipping(EWidgetClipping::Inherit);
	UBorder* Backing = WidgetTree->ConstructWidget<UBorder>();
	Backing->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.055f, 1.f));
	Backing->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* BackingSlot = Overlay->AddChildToOverlay(Backing);
	BackingSlot->SetHorizontalAlignment(HAlign_Fill);
	BackingSlot->SetVerticalAlignment(VAlign_Fill);
	UBorder* FrameBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Slot"));
	FrameBorder->SetPadding(FMargin(0.f));
	UOverlaySlot* FrameSlot = Overlay->AddChildToOverlay(FrameBorder);
	FrameSlot->SetHorizontalAlignment(HAlign_Fill);
	FrameSlot->SetVerticalAlignment(VAlign_Fill);
	FrameBorder->SetVisibility(ESlateVisibility::HitTestInvisible);

	FocusImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectedSlot"));
	FocusImage->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* FocusSlot = Overlay->AddChildToOverlay(FocusImage);
	FocusSlot->SetHorizontalAlignment(HAlign_Fill);
	FocusSlot->SetVerticalAlignment(VAlign_Fill);
	AcquiredImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AcquiredSlot"));
	AcquiredImage->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* AcquiredSlot = Overlay->AddChildToOverlay(AcquiredImage);
	AcquiredSlot->SetHorizontalAlignment(HAlign_Fill);
	AcquiredSlot->SetVerticalAlignment(VAlign_Fill);

	IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
	IconSize->SetWidthOverride(DefaultSlotSize.X * 0.625f);
	IconSize->SetHeightOverride(DefaultSlotSize.Y * 0.625f);
	IconSize->SetContent(IconImage);
	IconImage->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(IconSize);
	IconSlot->SetHorizontalAlignment(HAlign_Center);
	IconSlot->SetVerticalAlignment(VAlign_Center);

	CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountText"));
	ApplyTextRole(CountText, EMKTextRole::Caption);
	CountText->SetJustification(ETextJustify::Right);
	CountText->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* CountSlot = Overlay->AddChildToOverlay(CountText);
	CountSlot->SetHorizontalAlignment(HAlign_Right);
	CountSlot->SetVerticalAlignment(VAlign_Bottom);
	CountSlot->SetPadding(FMargin(0.f, 0.f, 7.f, 7.f));

	KeyGuideText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KeyGuideText"));
	ApplyTextRole(KeyGuideText, EMKTextRole::Caption);
	KeyGuideText->SetJustification(ETextJustify::Center);
	KeyGuideBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Keycap"));
	KeyGuideBorder->SetPadding(FMargin(6.f, 2.f));
	KeyGuideBorder->SetContent(KeyGuideText);
	KeyGuideBorder->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* KeySlot = Overlay->AddChildToOverlay(KeyGuideBorder);
	KeySlot->SetHorizontalAlignment(HAlign_Center);
	KeySlot->SetVerticalAlignment(VAlign_Bottom);
	KeyGuideBorder->SetRenderTranslation(FVector2D(0.f, 10.f));

	SelectionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectionText"));
	SelectionText->SetText(FText::FromString(TEXT("✓")));
	ApplyTextRole(SelectionText, EMKTextRole::Label);
	SelectionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 1.f, 0.72f)));
	SelectionText->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* SelectionSlot = Overlay->AddChildToOverlay(SelectionText);
	SelectionSlot->SetHorizontalAlignment(HAlign_Right);
	SelectionSlot->SetVerticalAlignment(VAlign_Top);

	LockImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Lock"));
	LockImage->SetDesiredSizeOverride(FVector2D(24.f));
	LockImage->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* LockSlot = Overlay->AddChildToOverlay(LockImage);
	LockSlot->SetHorizontalAlignment(HAlign_Center);
	LockSlot->SetVerticalAlignment(VAlign_Center);

	RankBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RankBadge"));
	RankBorder->SetPadding(FMargin(5.f, 2.f));
	RankBorder->SetVisibility(ESlateVisibility::Collapsed);
	RankText = WidgetTree->ConstructWidget<UTextBlock>();
	ApplyTextRole(RankText, EMKTextRole::Numeric);
	RankText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.83f, 0.45f)));
	RankBorder->SetContent(RankText);
	RankText->SetJustification(ETextJustify::Center);
	USizeBox* RankSize = WidgetTree->ConstructWidget<USizeBox>();
	RankSize->SetMinDesiredWidth(28.f);
	RankSize->SetMinDesiredHeight(28.f);
	RankSize->SetContent(RankBorder);
	UOverlaySlot* RankSlot = Overlay->AddChildToOverlay(RankSize);
	RankSlot->SetHorizontalAlignment(HAlign_Right);
	RankSlot->SetVerticalAlignment(VAlign_Bottom);
	RankSlot->SetPadding(FMargin(0.f, 0.f, 5.f, 5.f));

	AcquiredSeal = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Sparkle"));
	AcquiredSeal->SetDesiredSizeOverride(FVector2D(20.f));
	AcquiredSeal->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* SealSlot = Overlay->AddChildToOverlay(AcquiredSeal);
	SealSlot->SetPadding(FMargin(4.f));
}

void UMKSlotBase::UpdateFocusImageVisibility()
{
	if (::IsValid(FocusImage) == false)
	{
		return;
	}

	FocusImage->SetVisibility(bHasSlotFocus ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
