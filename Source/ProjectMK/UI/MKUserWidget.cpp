// LINK

#include "ProjectMK/UI/MKUserWidget.h"

#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"

void UMKUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LocalPlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (::IsValid(LocalPlayerCharacter))
	{
		OwnerASC = LocalPlayerCharacter->GetAbilitySystemComponent();
	}

	BindEvents();
}

void UMKUserWidget::NativeDestruct()
{
	Super::NativeDestruct();

	UnbindEvents();
}

UInventoryComponent* UMKUserWidget::GetLocalInventoryComponent() const
{
	if (::IsValid(LocalPlayerCharacter) == false)
	{
		return nullptr;
	}

	return LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>();
}

const UAttributeSet_Character* UMKUserWidget::GetCharacterAttributeSet() const
{
	if (::IsValid(OwnerASC) == false)
	{
		return nullptr;
	}

	return Cast<UAttributeSet_Character>(OwnerASC->GetAttributeSet(UAttributeSet_Character::StaticClass()));
}
