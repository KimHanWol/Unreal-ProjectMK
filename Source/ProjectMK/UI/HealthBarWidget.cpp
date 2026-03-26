// LINK

#include "ProjectMK/UI/HealthBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"

void UHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (::IsValid(ProgressBar_Health) == false)
	{
		return;
	}

	CurrentHealthPercent = FMath::FInterpTo(CurrentHealthPercent, TargetHealthPercent, InDeltaTime, ProgressBarInterpSpeed);
	if (FMath::IsNearlyEqual(CurrentHealthPercent, TargetHealthPercent, KINDA_SMALL_NUMBER))
	{
		CurrentHealthPercent = TargetHealthPercent;
	}

	ProgressBar_Health->SetPercent(CurrentHealthPercent);
}

void UHealthBarWidget::BindEvents()
{
	Super::BindEvents();

	if (::IsValid(OwnerASC) == false)
	{
		return;
	}

	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetMaxHealthAttribute()).AddUObject(this, &::UHealthBarWidget::OnMaxHealthChanged);
	OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).AddUObject(this, &::UHealthBarWidget::OnCurrentHealthChanged);

	CurrentHealthPercent = TargetHealthPercent = GetHealthRatio();
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

void UHealthBarWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	TargetHealthPercent = GetHealthRatio();
}

void UHealthBarWidget::OnCurrentHealthChanged(const FOnAttributeChangeData& Data)
{
	TargetHealthPercent = GetHealthRatio();
}

float UHealthBarWidget::GetHealthRatio() const
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

	const float MaxHealth = AttributeSet_Character->GetMaxHealth();
	if (MaxHealth <= 0.f)
	{
		return 0.f;
	}

	return AttributeSet_Character->GetCurrentHealth() / MaxHealth;
}

void UHealthBarWidget::UpdateHealthProgressBar()
{
	if (::IsValid(ProgressBar_Health) == false)
	{
		return;
	}

	ProgressBar_Health->SetPercent(CurrentHealthPercent);
}
