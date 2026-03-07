// LINK

#include "ProjectMK/Controller/MKPlayerController.h"

#include "ProjectMK/System/MKCheatManager.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ProjectMK/UI/HUDWidget.h"

AMKPlayerController::AMKPlayerController()
{
	CheatClass = UMKCheatManager::StaticClass();
}

void AMKPlayerController::ToggleShopTestWidget()
{
	TArray<UUserWidget*> FoundWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, FoundWidgets, UHUDWidget::StaticClass(), false);
	UHUDWidget* HUDWidget = FoundWidgets.Num() > 0 ? Cast<UHUDWidget>(FoundWidgets[0]) : nullptr;
	if (::IsValid(HUDWidget) == false)
	{
		return;
	}

	const bool bIsOpened = HUDWidget->ToggleShopWidget();
	if (bIsOpened)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}
