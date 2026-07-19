// LINK

#include "ProjectMK/UI/InventoryEquipmentWidget.h"

void UInventoryEquipmentWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Characters no longer use this panel, but existing widget blueprints may still reference it.
	// Keep it collapsed until those references are removed from the UMG asset graph.
	SetVisibility(ESlateVisibility::Collapsed);
}
