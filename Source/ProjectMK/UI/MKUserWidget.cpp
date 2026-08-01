// LINK

#include "ProjectMK/UI/MKUserWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/UI/HUDWidget.h"

void UMKUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LocalPlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (::IsValid(LocalPlayerCharacter))
	{
		OwnerASC = LocalPlayerCharacter->GetAbilitySystemComponent();
	}

	BindEvents();
	UpdateWidget();
}

void UMKUserWidget::NativeDestruct()
{
	Super::NativeDestruct();

	UnbindEvents();
}

AMKCharacter* UMKUserWidget::GetLocalPlayerCharacter() const
{
	return LocalPlayerCharacter.Get();
}

UHUDWidget* UMKUserWidget::GetHUDWidget() const
{
	TArray<UUserWidget*> FoundWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(const_cast<UMKUserWidget*>(this), FoundWidgets, UHUDWidget::StaticClass(), false);
	return FoundWidgets.Num() > 0 ? Cast<UHUDWidget>(FoundWidgets[0]) : nullptr;
}

UAbilitySystemComponent* UMKUserWidget::GetOwnerAbilitySystemComponent() const
{
	return OwnerASC.Get();
}

UInventoryComponent* UMKUserWidget::GetLocalInventoryComponent() const
{
	if (::IsValid(LocalPlayerCharacter) == false)
	{
		return nullptr;
	}

	return LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>();
}

USkillComponent* UMKUserWidget::GetLocalSkillComponent() const
{
	if (::IsValid(LocalPlayerCharacter) == false)
	{
		return nullptr;
	}

	return LocalPlayerCharacter->GetComponentByClass<USkillComponent>();
}

const UAttributeSet_Character* UMKUserWidget::GetCharacterAttributeSet() const
{
	if (::IsValid(OwnerASC) == false)
	{
		return nullptr;
	}

	return Cast<UAttributeSet_Character>(OwnerASC->GetAttributeSet(UAttributeSet_Character::StaticClass()));
}

bool UMKUserWidget::IsVisible() const
{
	return GetVisibility() != ESlateVisibility::Collapsed;
}

void UMKUserWidget::SetVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
