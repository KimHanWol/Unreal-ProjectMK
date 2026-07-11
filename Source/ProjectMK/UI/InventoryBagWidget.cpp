// LINK

#include "ProjectMK/UI/InventoryBagWidget.h"

void UInventoryBagWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Legacy widget kept only so existing UMG assets do not break while the inventory UI moves into UInventoryWidget.
	SetVisibility(ESlateVisibility::Collapsed);
}
