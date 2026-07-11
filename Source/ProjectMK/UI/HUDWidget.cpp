// LINK

#include "ProjectMK/UI/HUDWidget.h"

#include "Components/Widget.h"
#include "ProjectMK/UI/InventoryWidget.h"
#include "ProjectMK/UI/ShopWidget.h"

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (::IsValid(Inventory))
	{
		Inventory->SetVisibility(ESlateVisibility::Visible);
	}
}

bool UHUDWidget::ToggleInventoryWidget()
{
	if (::IsValid(Inventory) == false)
	{
		return false;
	}

	Inventory->SetVisibility(ESlateVisibility::Visible);
	return false;
}

bool UHUDWidget::ToggleShopWidget()
{
	if (::IsValid(Shop) == false)
	{
		return false;
	}

	const bool bWillOpen = Shop->GetVisibility() != ESlateVisibility::Visible;
	Shop->SetVisibility(bWillOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	return bWillOpen;
}

bool UHUDWidget::IsMenuVisible() const
{
	return ::IsValid(Shop) && Shop->GetVisibility() == ESlateVisibility::Visible;
}
