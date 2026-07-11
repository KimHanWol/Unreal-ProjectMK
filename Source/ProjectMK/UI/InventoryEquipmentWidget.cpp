// LINK

#include "ProjectMK/UI/InventoryEquipmentWidget.h"

void UInventoryEquipmentWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Characters no longer equip items, so keep the legacy panel hidden without touching the UMG asset graph.
	SetVisibility(ESlateVisibility::Collapsed);
}
