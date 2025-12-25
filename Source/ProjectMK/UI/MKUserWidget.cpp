// LINK

#include "ProjectMK/UI/MKUserWidget.h"

#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"

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
