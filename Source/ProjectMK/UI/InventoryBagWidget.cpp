// LINK

#include "ProjectMK/UI/InventoryBagWidget.h"

void UInventoryBagWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Keep this widget only to preserve existing UMG references during the inventory UI cleanup.
	// Remove it after every dependent widget blueprint has been moved to UInventoryWidget.
	SetVisibility(ESlateVisibility::Collapsed);
}
