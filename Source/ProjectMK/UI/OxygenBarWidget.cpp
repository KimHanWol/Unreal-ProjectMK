// LINK

#include "ProjectMK/UI/OxygenBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"

TSharedRef<SWidget> UOxygenBarWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->FindWidget(TEXT("RuntimeOxygenBar")) == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

void UOxygenBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (::IsValid(ProgressBar_Oxygen) == false)
	{
		return;
	}

	const float LatestOxygenPercent = FMath::Clamp(GetOxygenRatio(), 0.f, 1.f);
	if (FMath::IsNearlyEqual(TargetOxygenPercent, LatestOxygenPercent, KINDA_SMALL_NUMBER) == false)
	{
		TargetOxygenPercent = LatestOxygenPercent;
		bIsInterpolatingOxygen = true;
	}

	if (bIsInterpolatingOxygen == false)
	{
		return;
	}

	CurrentOxygenPercent = FMath::FInterpTo(CurrentOxygenPercent, TargetOxygenPercent, InDeltaTime, ProgressBarInterpSpeed);
	if (FMath::IsNearlyEqual(CurrentOxygenPercent, TargetOxygenPercent, KINDA_SMALL_NUMBER))
	{
		CurrentOxygenPercent = TargetOxygenPercent;
		bIsInterpolatingOxygen = false;
	}

	ProgressBar_Oxygen->SetPercent(CurrentOxygenPercent);
}

void UOxygenBarWidget::BindEvents()
{
	Super::BindEvents();
	ApplyProgressBarStyle();

	if (::IsValid(OwnerASC) == false)
	{
		return;
	}

	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetMaxOxygenAttribute()).AddUObject(this, &UOxygenBarWidget::OnMaxOxygenChanged);
	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentOxygenAttribute()).AddUObject(this, &UOxygenBarWidget::OnCurrentOxygenChanged);

	CurrentOxygenPercent = TargetOxygenPercent = GetOxygenRatio();
	bIsInterpolatingOxygen = false;
	UpdateOxygenProgressBar();
}

void UOxygenBarWidget::UnbindEvents()
{
	Super::UnbindEvents();

	if (::IsValid(OwnerASC) == false)
	{
		return;
	}

	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetMaxOxygenAttribute()).RemoveAll(this);
	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentOxygenAttribute()).RemoveAll(this);
}

void UOxygenBarWidget::BuildDefaultWidgetTree()
{
	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RuntimeOxygenBar"));
	RootSize->SetWidthOverride(260.f);
	RootSize->SetHeightOverride(32.f);
	WidgetTree->RootWidget = RootSize;
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OxygenFrame"));
	Frame->SetPadding(FMargin(40.f, 9.f, 10.f, 9.f));
	RootSize->SetContent(Frame);
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
	Frame->SetContent(Overlay);

	ProgressBar_Oxygen = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ProgressBar_Oxygen"));
	FProgressBarStyle ProgressStyle;
	ProgressStyle.SetBackgroundImage(FSlateColorBrush(FLinearColor(0.055f, 0.06f, 0.07f, 1.f)));
	ProgressStyle.SetFillImage(FSlateColorBrush(FLinearColor::White));
	ProgressBar_Oxygen->SetWidgetStyle(ProgressStyle);
	ProgressBar_Oxygen->SetFillColorAndOpacity(FLinearColor(0.08f, 0.65f, 0.82f, 1.f));
	UOverlaySlot* FillSlot = Overlay->AddChildToOverlay(ProgressBar_Oxygen);
	FillSlot->SetHorizontalAlignment(HAlign_Fill);
	FillSlot->SetVerticalAlignment(VAlign_Fill);
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(NSLOCTEXT("HUD", "OxygenLabel", "O₂"));
	ApplyTextRole(Label, EMKTextRole::Caption);
	UOverlaySlot* LabelSlot = Overlay->AddChildToOverlay(Label);
	LabelSlot->SetHorizontalAlignment(HAlign_Left);
	LabelSlot->SetVerticalAlignment(VAlign_Center);
	LabelSlot->SetPadding(FMargin(0.f));
}

void UOxygenBarWidget::ApplyProgressBarStyle()
{
	if (::IsValid(ProgressBar_Oxygen) == false)
	{
		return;
	}

	ProgressBar_Oxygen->PercentDelegate.Unbind();
	ProgressBar_Oxygen->FillColorAndOpacityDelegate.Unbind();
	FProgressBarStyle ProgressStyle;
	ProgressStyle.SetBackgroundImage(FSlateColorBrush(FLinearColor(0.055f, 0.06f, 0.07f, 1.f)));
	ProgressStyle.SetFillImage(FSlateColorBrush(FLinearColor::White));
	ProgressBar_Oxygen->SetWidgetStyle(ProgressStyle);
	ProgressBar_Oxygen->SetBarFillType(EProgressBarFillType::LeftToRight);
	ProgressBar_Oxygen->SetBarFillStyle(EProgressBarFillStyle::Scale);
	ProgressBar_Oxygen->SetIsMarquee(false);
	ProgressBar_Oxygen->SetBorderPadding(FVector2D::ZeroVector);
	ProgressBar_Oxygen->SetFillColorAndOpacity(FLinearColor(0.08f, 0.65f, 0.82f, 1.f));
}

float UOxygenBarWidget::GetOxygenRatio() const
{
	const UAttributeSet_Character* CharacterAttributeSet = GetCharacterAttributeSet();
	if (::IsValid(CharacterAttributeSet) == false)
	{
		return 0.f;
	}

	const float MaxOxygen = CharacterAttributeSet->GetMaxOxygen();
	return MaxOxygen > 0.f ? (CharacterAttributeSet->GetCurrentOxygen() / MaxOxygen) : 0.f;
}

void UOxygenBarWidget::StartProgressInterpolation()
{
	if (FMath::IsNearlyEqual(CurrentOxygenPercent, TargetOxygenPercent, KINDA_SMALL_NUMBER))
	{
		CurrentOxygenPercent = TargetOxygenPercent;
		bIsInterpolatingOxygen = false;
		UpdateOxygenProgressBar();
		return;
	}

	bIsInterpolatingOxygen = true;
}

void UOxygenBarWidget::UpdateOxygenProgressBar()
{
	if (::IsValid(ProgressBar_Oxygen) == false)
	{
		return;
	}

	ProgressBar_Oxygen->SetPercent(CurrentOxygenPercent);
}

void UOxygenBarWidget::OnMaxOxygenChanged(const FOnAttributeChangeData& Data)
{
	TargetOxygenPercent = GetOxygenRatio();
	StartProgressInterpolation();
}

void UOxygenBarWidget::OnCurrentOxygenChanged(const FOnAttributeChangeData& Data)
{
	TargetOxygenPercent = GetOxygenRatio();
	StartProgressInterpolation();
}
