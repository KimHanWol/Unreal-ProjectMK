// LINK

#include "ProjectMK/UI/HealthBarWidget.h"

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

TSharedRef<SWidget> UHealthBarWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->FindWidget(TEXT("RuntimeHealthBar")) == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

void UHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (::IsValid(ProgressBar_Health) == false)
	{
		return;
	}

	const float LatestHealthPercent = FMath::Clamp(GetHealthRatio(), 0.f, 1.f);
	if (FMath::IsNearlyEqual(TargetHealthPercent, LatestHealthPercent, KINDA_SMALL_NUMBER) == false)
	{
		TargetHealthPercent = LatestHealthPercent;
		bIsInterpolatingHealth = true;
	}

	if (bIsInterpolatingHealth == false)
	{
		return;
	}

	CurrentHealthPercent = FMath::FInterpTo(CurrentHealthPercent, TargetHealthPercent, InDeltaTime, ProgressBarInterpSpeed);
	if (FMath::IsNearlyEqual(CurrentHealthPercent, TargetHealthPercent, KINDA_SMALL_NUMBER))
	{
		CurrentHealthPercent = TargetHealthPercent;
		bIsInterpolatingHealth = false;
	}

	ProgressBar_Health->SetPercent(CurrentHealthPercent);
}

void UHealthBarWidget::BindEvents()
{
	Super::BindEvents();
	ApplyProgressBarStyle();

	if (::IsValid(OwnerASC) == false)
	{
		return;
	}

	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetMaxHealthAttribute()).AddUObject(this, &UHealthBarWidget::OnMaxHealthChanged);
	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).AddUObject(this, &UHealthBarWidget::OnCurrentHealthChanged);

	CurrentHealthPercent = TargetHealthPercent = GetHealthRatio();
	bIsInterpolatingHealth = false;
	UpdateHealthProgressBar();
}

void UHealthBarWidget::UnbindEvents()
{
	Super::UnbindEvents();

	if (::IsValid(OwnerASC) == false)
	{
		return;
	}

	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetMaxHealthAttribute()).RemoveAll(this);
	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).RemoveAll(this);
}

void UHealthBarWidget::BuildDefaultWidgetTree()
{
	UProgressBar* Bar = nullptr;
	WidgetTree->RootWidget = BuildStatusGauge(Bar, TEXT("RuntimeHealthBar"), TEXT("BarFrame"), NSLOCTEXT("HUD", "HealthLabel", "HP"), FLinearColor(0.8f, 0.12f, 0.1f));
	ProgressBar_Health = Bar;
}

void UHealthBarWidget::ApplyProgressBarStyle()
{
	if (::IsValid(ProgressBar_Health) == false)
	{
		return;
	}

	ProgressBar_Health->PercentDelegate.Unbind();
	ProgressBar_Health->FillColorAndOpacityDelegate.Unbind();
	FProgressBarStyle ProgressStyle;
	ProgressStyle.SetBackgroundImage(FSlateColorBrush(FLinearColor(0.055f, 0.06f, 0.07f, 1.f)));
	ProgressStyle.SetFillImage(FSlateColorBrush(FLinearColor::White));
	ProgressBar_Health->SetWidgetStyle(ProgressStyle);
	ProgressBar_Health->SetBarFillType(EProgressBarFillType::LeftToRight);
	ProgressBar_Health->SetBarFillStyle(EProgressBarFillStyle::Scale);
	ProgressBar_Health->SetIsMarquee(false);
	ProgressBar_Health->SetBorderPadding(FVector2D::ZeroVector);
	ProgressBar_Health->SetFillColorAndOpacity(FLinearColor(0.8f, 0.12f, 0.1f, 1.f));
}

float UHealthBarWidget::GetHealthRatio() const
{
	const UAttributeSet_Character* CharacterAttributeSet = GetCharacterAttributeSet();
	if (::IsValid(CharacterAttributeSet) == false)
	{
		return 0.f;
	}

	const float MaxHealth = CharacterAttributeSet->GetMaxHealth();
	if (MaxHealth <= 0.f)
	{
		return 0.f;
	}

	return CharacterAttributeSet->GetCurrentHealth() / MaxHealth;
}

void UHealthBarWidget::StartProgressInterpolation()
{
	if (FMath::IsNearlyEqual(CurrentHealthPercent, TargetHealthPercent, KINDA_SMALL_NUMBER))
	{
		CurrentHealthPercent = TargetHealthPercent;
		bIsInterpolatingHealth = false;
		UpdateHealthProgressBar();
		return;
	}

	bIsInterpolatingHealth = true;
}

void UHealthBarWidget::UpdateHealthProgressBar()
{
	if (::IsValid(ProgressBar_Health) == false)
	{
		return;
	}

	ProgressBar_Health->SetPercent(CurrentHealthPercent);
}

void UHealthBarWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	TargetHealthPercent = GetHealthRatio();
	StartProgressInterpolation();
}

void UHealthBarWidget::OnCurrentHealthChanged(const FOnAttributeChangeData& Data)
{
	TargetHealthPercent = GetHealthRatio();
	StartProgressInterpolation();
}
