// LINK

#include "ProjectMK/UI/HUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Widget.h"
#include "Blueprint/WidgetTree.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/UI/ItemQuantityDialog.h"
#include "ProjectMK/UI/ItemSlotDragDropOperation.h"
#include "ProjectMK/Core/Manager/SoundManager.h"
#include "ProjectMK/UI/InventoryWidget.h"
#include "ProjectMK/UI/MKMenuBase.h"
#include "ProjectMK/UI/MKSlotBase.h"
#include "ProjectMK/UI/MKTooltipBase.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);
	DropQuantityDialog = CreateWidget<UItemQuantityDialog>(this);
	DropQuantityDialog->AddToViewport(200);
	DropQuantityDialog->SetVisibility(ESlateVisibility::Collapsed);
	DropQuantityDialog->OnQuantityConfirmed.AddUObject(this, &UHUDWidget::OnDropConfirmed);
	DropQuantityDialog->OnQuantityCancelled.AddUObject(this, &UHUDWidget::OnDropCancelled);

	if (::IsValid(Inventory))
	{
		Inventory->SetVisibility(ESlateVisibility::Visible);
	}

	if (::IsValid(Tooltip))
	{
		Tooltip->HideTooltip();
	}
}

void UHUDWidget::NativeDestruct()
{
	if (::IsValid(DropQuantityDialog))
	{
		DropQuantityDialog->OnQuantityConfirmed.RemoveAll(this);
		DropQuantityDialog->OnQuantityCancelled.RemoveAll(this);
		DropQuantityDialog->RemoveFromParent();
	}
	Super::NativeDestruct();
}

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (::IsValid(HoveredTooltipSource) == false || ::IsValid(Tooltip) == false || Tooltip->IsVisible() == false)
	{
		return;
	}

	UpdateTooltipPosition();
}

bool UHUDWidget::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	const UItemSlotDragDropOperation* Item = Cast<UItemSlotDragDropOperation>(Operation);
	UInventoryComponent* Items = GetLocalInventoryComponent();
	if (::IsValid(Item) == false || ::IsValid(Items) == false) { return Super::NativeOnDrop(Geometry, Event, Operation); }
	if (::IsValid(Inventory) && Inventory->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition())) { return true; }
	UWidget* MenuFrame = ::IsValid(Menu) ? Menu->WidgetTree->FindWidget(TEXT("BG")) : nullptr;
	if (IsMenuVisible() && ::IsValid(MenuFrame) && MenuFrame->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition())) { return true; }
	const TArray<FInventorySlotData>& Slots = Items->GetItemSlots(Item->SourceContainer);
	if (Slots.IsValidIndex(Item->SourceSlotIndex) == false || Slots[Item->SourceSlotIndex].ItemUID != Item->ItemKey) { return true; }
	PendingDropContainer = Item->SourceContainer;
	PendingDropIndex = Item->SourceSlotIndex;
	PendingDropKey = Item->ItemKey;
	DropQuantityDialog->ShowQuantity(Item->ItemKey, Slots[Item->SourceSlotIndex].ItemCount, true);
	SetHoveredSlot(nullptr);
	if (AMKCharacter* Character = GetLocalPlayerCharacter()) { Character->SetMenuInputBlocked(true); }
	return true;
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

	if (Menu->IsMenuOpen(EMenuContentsType::SkillTree))
	{
		Menu->CloseMenu();
		return false;
	}

	return Menu->OpenMenu(EMenuContentsType::SkillTree);
}

bool UHUDWidget::ToggleCrafting()
{
	if (::IsValid(Menu) == false)
	{
		return false;
	}

	if (Menu->IsMenuOpen(EMenuContentsType::Crafting))
	{
		Menu->CloseMenu();
		return false;
	}

	return Menu->OpenMenu(EMenuContentsType::Crafting);
}

bool UHUDWidget::TryOpenShop()
{
	if (::IsValid(Menu) == false)
	{
		return false;
	}

	return Menu->OpenMenu(EMenuContentsType::Shop);
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
	if (::IsValid(InHoveredSlot) && InHoveredSlot != HoveredSlot)
	{
		USoundManager::TryPlaySFX(this, ESFXType::UI_Hover);
	}

	HoveredSlot = InHoveredSlot;
	HoveredTooltipSource = InHoveredSlot;
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
	HoveredTooltipSource = nullptr;

	if (::IsValid(Tooltip))
	{
		Tooltip->HideTooltip();
	}
}

void UHUDWidget::ShowRecipeTooltip(UWidget* Source, FName RecipeKey)
{
	HoveredSlot = nullptr;
	HoveredTooltipSource = Source;
	if (::IsValid(Tooltip)) { Tooltip->ShowRecipeTooltip(RecipeKey); UpdateTooltipPosition(); }
}

void UHUDWidget::ClearTooltipSource(UWidget* Source)
{
	if (HoveredTooltipSource == Source) { HoveredTooltipSource = nullptr; HoveredSlot = nullptr; if (::IsValid(Tooltip)) { Tooltip->HideTooltip(); } }
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

	if (UCanvasPanelSlot* TooltipSlot = Cast<UCanvasPanelSlot>(Tooltip->Slot))
	{
		TooltipSlot->SetAnchors(FAnchors(0.f, 0.f));
		TooltipSlot->SetAlignment(FVector2D::ZeroVector);
		TooltipSlot->SetAutoSize(true);
		TooltipSlot->SetZOrder(100);
		Tooltip->SetRenderTranslation(FVector2D::ZeroVector);
		TooltipSlot->SetPosition(TooltipPosition);
		return;
	}

	Tooltip->SetRenderTranslation(TooltipPosition);
}

void UHUDWidget::OnDropConfirmed(FName ItemKey, int32 Count)
{
	UInventoryComponent* Items = GetLocalInventoryComponent();
	if (::IsValid(Items))
	{
		const TArray<FInventorySlotData>& Slots = Items->GetItemSlots(PendingDropContainer);
		if (Slots.IsValidIndex(PendingDropIndex) && Slots[PendingDropIndex].ItemUID == ItemKey && ItemKey == PendingDropKey)
		{
			Items->TryDropItemSlot(PendingDropContainer, PendingDropIndex, Count);
		}
	}
	OnDropCancelled();
}

void UHUDWidget::OnDropCancelled()
{
	PendingDropIndex = INDEX_NONE;
	PendingDropKey = NAME_None;
	if (AMKCharacter* Character = GetLocalPlayerCharacter()) { Character->SetMenuInputBlocked(IsMenuVisible()); }
}
