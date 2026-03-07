// LINK

#include "ProjectMK/UI/HUDWidget.h"

#include "Components/Widget.h"
#include "ProjectMK/UI/ShopWidget.h"

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
