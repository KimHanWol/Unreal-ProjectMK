// LINK

#include "ProjectMK/UI/OxygenBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"

void UOxygenBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (::IsValid(ProgressBar_Oxygen) == false || bIsInterpolatingOxygen == false)
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
