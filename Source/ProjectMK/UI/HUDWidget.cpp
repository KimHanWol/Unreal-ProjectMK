// LINK

#include "ProjectMK/UI/HUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Widget.h"
#include "ProjectMK/UI/InventoryWidget.h"
#include "ProjectMK/UI/MKMenuBase.h"
#include "ProjectMK/UI/MKSlotBase.h"
#include "ProjectMK/UI/MKTooltipBase.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (::IsValid(Inventory))
	{
		Inventory->SetVisibility(ESlateVisibility::Visible);
	}

	if (::IsValid(Tooltip))
	{
		Tooltip->HideTooltip();
	}
}

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (::IsValid(HoveredSlot) == false || ::IsValid(Tooltip) == false || Tooltip->IsVisible() == false)
	{
		return;
	}

	UpdateTooltipPosition();
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

UMKTooltipBase* UHUDWidget::GetTooltipWidget() const
{
	return Tooltip.Get();
}

void UHUDWidget::SetHoveredSlot(UMKSlotBase* InHoveredSlot)
{
	HoveredSlot = InHoveredSlot;
	UpdateTooltipWidget();
	UpdateTooltipPosition();
}

void UHUDWidget::ClearHoveredSlot(UMKSlotBase* InHoveredSlot)
{
	if (HoveredSlot != InHoveredSlot)
	{
		return;
	}

	HoveredSlot = nullptr;

	if (::IsValid(Tooltip))
	{
		Tooltip->HideTooltip();
	}
}

void UHUDWidget::UpdateTooltipWidget()
{
	if (::IsValid(Tooltip) == false)
	{
		return;
	}

	Tooltip->HideTooltip();

	if (::IsValid(HoveredSlot) == false)
	{
		return;
	}

	HoveredSlot->UpdateTooltipWidget(Tooltip);
}

void UHUDWidget::UpdateTooltipPosition()
{
	if (::IsValid(Tooltip) == false)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (::IsValid(PlayerController) == false)
	{
		return;
	}

	float MousePositionX = 0.f;
	float MousePositionY = 0.f;
	if (UWidgetLayoutLibrary::GetMousePositionScaledByDPI(PlayerController, MousePositionX, MousePositionY) == false)
	{
		return;
	}

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D RawViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const FVector2D ViewportSize = ViewportScale > 0.f ? (RawViewportSize / ViewportScale) : RawViewportSize;
	const FVector2D MousePosition(MousePositionX, MousePositionY);

	Tooltip->ForceLayoutPrepass();
	const FVector2D TooltipSize = Tooltip->GetDesiredSize();

	FVector2D TooltipPosition = MousePosition + TooltipOffset;

	if (TooltipPosition.X + TooltipSize.X > ViewportSize.X)
	{
		TooltipPosition.X = MousePosition.X - TooltipSize.X - TooltipOffset.X;
	}

	if (TooltipPosition.Y + TooltipSize.Y > ViewportSize.Y)
	{
		TooltipPosition.Y = MousePosition.Y - TooltipSize.Y - TooltipOffset.Y;
	}

	TooltipPosition.X = FMath::Clamp(TooltipPosition.X, 0.f, FMath::Max(0.f, ViewportSize.X - TooltipSize.X));
	TooltipPosition.Y = FMath::Clamp(TooltipPosition.Y, 0.f, FMath::Max(0.f, ViewportSize.Y - TooltipSize.Y));

	Tooltip->SetRenderTranslation(TooltipPosition);
}
