// LINK

#include "ProjectMK/UI/HUDWidget.h"

#include "Components/Widget.h"
#include "ProjectMK/UI/InventoryWidget.h"
#include "ProjectMK/UI/MKMenuBase.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"

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

bool UHUDWidget::ToggleSkillTree()
{
	if (::IsValid(Menu) == false)
	{
		return false;
	}

	if (Menu->IsMenuOpen())
	{
		Menu->CloseMenu();
		return false;
	}

	return Menu->OpenMenu(EMenuContentsType::SkillTree);
}

bool UHUDWidget::IsMenuVisible() const
{
	return ::IsValid(Menu) && Menu->IsMenuOpen();
}
