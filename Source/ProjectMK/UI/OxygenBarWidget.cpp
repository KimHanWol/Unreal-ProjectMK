// LINK

#include "ProjectMK/UI/OxygenBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"

void UOxygenBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (::IsValid(ProgressBar_Oxygen) == false)
	{
		return;
	}

	CurrentOxygenPercent = FMath::FInterpTo(CurrentOxygenPercent, TargetOxygenPercent, InDeltaTime, ProgressBarInterpSpeed);
	if (FMath::IsNearlyEqual(CurrentOxygenPercent, TargetOxygenPercent, KINDA_SMALL_NUMBER))
	{
		CurrentOxygenPercent = TargetOxygenPercent;
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

	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetMaxOxygenAttribute()).AddUObject(this, &::UOxygenBarWidget::OnMaxOxygenChanged);
	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentOxygenAttribute()).AddUObject(this, &::UOxygenBarWidget::OnCurrentOxygenChanged);

	CurrentOxygenPercent = TargetOxygenPercent = GetOxygenRatio();
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

void UOxygenBarWidget::OnMaxOxygenChanged(const FOnAttributeChangeData& Data)
{
	TargetOxygenPercent = GetOxygenRatio();
}

void UOxygenBarWidget::OnCurrentOxygenChanged(const FOnAttributeChangeData& Data)
{
	TargetOxygenPercent = GetOxygenRatio();
}

float UOxygenBarWidget::GetOxygenRatio() const
{
	if (::IsValid(OwnerASC) == false)
	{
		return 0.f;
	}

	const UAttributeSet_Character* AttributeSet_Character = Cast<UAttributeSet_Character>(OwnerASC->GetAttributeSet(UAttributeSet_Character::StaticClass()));
	if (::IsValid(AttributeSet_Character) == false)
	{
		return 0.f;
	}

	const float MaxOxygen = AttributeSet_Character->GetMaxOxygen();
	return MaxOxygen > 0.f ? (AttributeSet_Character->GetCurrentOxygen() / MaxOxygen) : 0.f;
}

void UOxygenBarWidget::UpdateOxygenProgressBar()
{
	if (::IsValid(ProgressBar_Oxygen) == false)
	{
		return;
	}

	ProgressBar_Oxygen->SetPercent(CurrentOxygenPercent);
}
