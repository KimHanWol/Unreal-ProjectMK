// LINK

#include "ProjectMK/UI/MKUserWidget.h"

void UMKUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindEvents();
}

void UMKUserWidget::NativeDestruct()
{
	Super::NativeDestruct();

	UnbindEvents();
}
