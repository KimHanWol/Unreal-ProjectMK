// LINK

#include "ProjectMK/Controller/MKPlayerController.h"

#include "ProjectMK/System/MKCheatManager.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "InputCoreTypes.h"
#include "ProjectMK/UI/HUDWidget.h"

AMKPlayerController::AMKPlayerController()
{
	CheatClass = UMKCheatManager::StaticClass();
}

void AMKPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AMKPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::K, IE_Pressed, this, &AMKPlayerController::ToggleSkillTree);
	}
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
}

void AMKPlayerController::ToggleSkillTree()
{
	UHUDWidget* HUDWidget = GetHUDWidget();
	if (::IsValid(HUDWidget) == false)
	{
		return;
	}

	HUDWidget->ToggleSkillTree();
}
