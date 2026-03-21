// LINK

#include "ProjectMK/UI/HUDWidget.h"

#include "Components/Widget.h"
#include "ProjectMK/UI/InventoryWidget.h"
#include "ProjectMK/UI/ShopWidget.h"

bool UHUDWidget::ToggleInventoryWidget()
{
    if (::IsValid(Inventory) == false)
    {
        return false;
    }

    const bool bWillOpen = Inventory->GetVisibility() != ESlateVisibility::Visible;
    Inventory->SetVisibility(bWillOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    return bWillOpen;
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
    return (::IsValid(Inventory) && Inventory->GetVisibility() == ESlateVisibility::Visible)
        || (::IsValid(Shop) && Shop->GetVisibility() == ESlateVisibility::Visible);
}
