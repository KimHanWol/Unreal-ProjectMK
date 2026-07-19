// LINK

#include "ProjectMK/Controller/MKPlayerController.h"

#include "ProjectMK/System/MKCheatManager.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ProjectMK/UI/HUDWidget.h"

AMKPlayerController::AMKPlayerController()
{
	CheatClass = UMKCheatManager::StaticClass();
}

UHUDWidget* AMKPlayerController::GetHUDWidget()
{
	TArray<UUserWidget*> FoundWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, FoundWidgets, UHUDWidget::StaticClass(), false);
	return FoundWidgets.Num() > 0 ? Cast<UHUDWidget>(FoundWidgets[0]) : nullptr;
}

void AMKPlayerController::ToggleInventoryWidget()
{
	UHUDWidget* HUDWidget = GetHUDWidget();
	if (::IsValid(HUDWidget) == false)
	{
		return;
	}

	HUDWidget->ToggleInventoryWidget();
	SetMenuInputMode(HUDWidget->IsMenuVisible());
}

void AMKPlayerController::ToggleShopTestWidget()
{
	UHUDWidget* HUDWidget = GetHUDWidget();
	if (::IsValid(HUDWidget) == false)
	{
		return;
	}

	HUDWidget->ToggleShopWidget();
	SetMenuInputMode(HUDWidget->IsMenuVisible());
}

void AMKPlayerController::SetMenuInputMode(bool bEnableMenuInput)
{
	if (bEnableMenuInput)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		return;
	}

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}
