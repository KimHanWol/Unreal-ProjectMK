// LINK

#include "ProjectMK/Controller/MKPlayerController.h"

#include "ProjectMK/System/MKCheatManager.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "InputCoreTypes.h"
#include "Engine/World.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "ProjectMK/UI/MinimapWidget.h"
#include "ProjectMK/UI/PlaytestWidget.h"
#include "ProjectMK/UI/QuickInventoryWidget.h"

AMKPlayerController::AMKPlayerController()
{
	CheatClass = UMKCheatManager::StaticClass();
	MinimapWidgetClass = UMinimapWidget::StaticClass();
	QuickInventoryWidgetClass = UQuickInventoryWidget::StaticClass();
}

void AMKPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	CreateMinimapWidget();
	CreatePlaytestWidget();
}

void AMKPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (::IsValid(MinimapWidget))
	{
		MinimapWidget->RemoveFromParent();
		MinimapWidget = nullptr;
	}
	if (::IsValid(QuickInventoryWidget))
	{
		QuickInventoryWidget->RemoveFromParent();
		QuickInventoryWidget = nullptr;
	}
	if (::IsValid(PlaytestWidget))
	{
		PlaytestWidget->RemoveFromParent();
		PlaytestWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AMKPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::K, IE_Pressed, this, &AMKPlayerController::ToggleSkillTree);
		InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AMKPlayerController::ToggleCrafting);
		InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AMKPlayerController::TogglePlaytestPanel);
	}
}

void AMKPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CreateMinimapWidget();
	CreatePlaytestWidget();
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

void AMKPlayerController::ToggleCrafting()
{
	UHUDWidget* HUDWidget = GetHUDWidget();
	if (::IsValid(HUDWidget) == false)
	{
		return;
	}

	HUDWidget->ToggleCrafting();
}

bool AMKPlayerController::TryOpenShop()
{
	UHUDWidget* HUDWidget = GetHUDWidget();
	if (::IsValid(HUDWidget) == false)
	{
		return false;
	}

	return HUDWidget->TryOpenShop();
}

void AMKPlayerController::TogglePlaytestPanel()
{
	if (::IsValid(PlaytestWidget))
	{
		PlaytestWidget->TogglePanel();
	}
}

void AMKPlayerController::CreateMinimapWidget()
{
	if (IsLocalController() && GetPawn() != nullptr && ::IsValid(MinimapWidget) == false && ::IsValid(MinimapWidgetClass))
	{
		MinimapWidget = CreateWidget<UMinimapWidget>(this, MinimapWidgetClass);
		if (::IsValid(MinimapWidget))
		{
			MinimapWidget->AddToViewport(5);
		}
	}
}

void AMKPlayerController::CreatePlaytestWidget()
{
#if WITH_EDITOR
	if (IsLocalController() && GetPawn() != nullptr && GetWorld()->WorldType == EWorldType::PIE)
	{
		if (::IsValid(PlaytestWidget) == false)
		{
			PlaytestWidget = CreateWidget<UPlaytestWidget>(this, UPlaytestWidget::StaticClass());
			if (::IsValid(PlaytestWidget))
			{
				PlaytestWidget->AddToViewport(20);
			}
		}
		if (::IsValid(QuickInventoryWidget) == false && ::IsValid(QuickInventoryWidgetClass))
		{
			QuickInventoryWidget = CreateWidget<UQuickInventoryWidget>(this, QuickInventoryWidgetClass);
			if (::IsValid(QuickInventoryWidget))
			{
				QuickInventoryWidget->AddToViewport(10);
			}
		}
	}
#endif
}

UHUDWidget* AMKPlayerController::GetHUDWidget()
{
	TArray<UUserWidget*> FoundWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, FoundWidgets, UHUDWidget::StaticClass(), false);
	return FoundWidgets.Num() > 0 ? Cast<UHUDWidget>(FoundWidgets[0]) : nullptr;
}
