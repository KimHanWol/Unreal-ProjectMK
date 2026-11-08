// LINK

#include "ProjectMK/UI/ItemSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "InputCoreTypes.h"
#include "PaperSprite.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/UI/ItemSlotDragDropOperation.h"
#include "ProjectMK/UI/MKTooltipBase.h"
#include "ProjectMK/UI/HUDWidget.h"

void UItemSlotWidget::ClearItem()
{
	CurrentItemKey = NAME_None;
	CurrentItemCount = 0;

	SetSlotCount(0, false);

	if (::IsValid(IconImage))
	{
		IconImage->SetBrush(FSlateBrush());
		IconImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UItemSlotWidget::SetItem(FName ItemKey, int32 ItemCount)
{
	if (::IsValid(IconImage) == false)
	{
		return;
	}

	ClearItem();
	CurrentItemKey = ItemKey;
	CurrentItemCount = ItemCount;

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	SetSlotCount(ItemCount, ItemCount > 1);

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemKey);
	if (ItemDataTableRow == nullptr)
	{
		return;
	}

	if (::IsValid(IconImage) && ItemDataTableRow->ItemIcon.IsNull() == false)
	{
		SetSlotIcon(ItemDataTableRow->ItemIcon.LoadSynchronous());
	}
}

void UItemSlotWidget::SetSlotIndex(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
	UpdateWidget();
}

void UItemSlotWidget::SetSlotAddress(EItemContainer Container, int32 InSlotIndex)
{
	SlotContainer = Container;
	SetSlotIndex(InSlotIndex);
}

void UItemSlotWidget::UpdateTooltipWidget(UMKTooltipBase* TooltipWidget) const
{
	Super::UpdateTooltipWidget(TooltipWidget);

	if (::IsValid(TooltipWidget) == false || CurrentItemKey.IsNone())
	{
		return;
	}

	TooltipWidget->ShowItemTooltip(CurrentItemKey);
}

void UItemSlotWidget::UpdateWidget()
{
	Super::UpdateWidget();
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	if (::IsValid(Inventory) == false || SlotIndex == INDEX_NONE) { return; }
	const TArray<FInventorySlotData>& Slots = Inventory->GetItemSlots(SlotContainer);
	if (Slots.IsValidIndex(SlotIndex) && Slots[SlotIndex].IsEmpty() == false) { SetItem(Slots[SlotIndex].ItemUID, Slots[SlotIndex].ItemCount); }
	else { ClearItem(); }
	SetKeyGuide(SlotContainer == EItemContainer::QuickSlot && bShowKeyGuide ? FText::AsNumber(SlotIndex + 1) : FText::GetEmpty());
}

FReply UItemSlotWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsSlotLocked == false && HasItem())
	{
		bPendingUseClick = SlotContainer == EItemContainer::QuickSlot && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UItemSlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPendingUseClick && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bPendingUseClick = false;
		OnSlotClicked();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	bPendingUseClick = false;
	if (UHUDWidget* HUD = GetHUDWidget()) { HUD->SetHoveredSlot(nullptr); }
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (HasItem() == false)
	{
		return;
	}

	UItemSlotDragDropOperation* DragDropOperation = NewObject<UItemSlotDragDropOperation>();
	if (::IsValid(DragDropOperation) == false)
	{
		return;
	}

	DragDropOperation->SourceSlotIndex = SlotIndex;
	DragDropOperation->SourceContainer = SlotContainer;
	DragDropOperation->ItemKey = CurrentItemKey;
	DragDropOperation->ItemCount = CurrentItemCount;
	DragDropOperation->Payload = this;
	DragDropOperation->Pivot = EDragPivot::CenterCenter;
	DragDropOperation->DefaultDragVisual = CreateDragVisualWidget();
	OutOperation = DragDropOperation;
}

bool UItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UItemSlotDragDropOperation* DragDropOperation = Cast<UItemSlotDragDropOperation>(InOperation);
	if (::IsValid(DragDropOperation) == false || SlotIndex == INDEX_NONE || bIsSlotLocked)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	UInventoryComponent* InventoryComponent = GetLocalInventoryComponent();
	if (::IsValid(InventoryComponent) == false)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	const TArray<FInventorySlotData>& Sources = InventoryComponent->GetItemSlots(DragDropOperation->SourceContainer);
	if (Sources.IsValidIndex(DragDropOperation->SourceSlotIndex) == false || Sources[DragDropOperation->SourceSlotIndex].ItemUID != DragDropOperation->ItemKey) { return true; }
	if (InventoryComponent->TryMoveItemSlot(DragDropOperation->SourceSlotIndex, SlotIndex, DragDropOperation->SourceContainer, SlotContainer))
	{
		if (AMKCharacter* Character = GetLocalPlayerCharacter()) { Character->TrySaveGame(); }
	}
	return true;
}

bool UItemSlotWidget::CanClickSlot() const
{
	return false;
}

void UItemSlotWidget::OnSlotClicked()
{
	AMKCharacter* Character = GetLocalPlayerCharacter();
	if (SlotContainer == EItemContainer::QuickSlot && ::IsValid(Character)) { Character->GetConsumableComponent()->TryUseQuickSlot(SlotIndex); }
}

UWidget* UItemSlotWidget::CreateDragVisualWidget() const
{
	if (::IsValid(IconImage) == false)
	{
		return nullptr;
	}

	UImage* DragVisualImage = NewObject<UImage>(GetTransientPackage());
	if (::IsValid(DragVisualImage) == false)
	{
		return nullptr;
	}

	DragVisualImage->SetBrush(IconImage->GetBrush());
	DragVisualImage->SetRenderOpacity(0.6f);
	DragVisualImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	USizeBox* Size = NewObject<USizeBox>(GetTransientPackage());
	const FGeometry& Geometry = IconImage->GetCachedGeometry();
	FVector2D IconSize = Geometry.GetLocalSize();
	if (IconSize.X > 0.f && IconSize.Y > 0.f)
	{
		IconSize = (Geometry.LocalToAbsolute(IconSize) - Geometry.LocalToAbsolute(FVector2D::ZeroVector)) / FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
	}
	Size->SetWidthOverride(IconSize.X > 0.f ? IconSize.X : DefaultSlotSize.X * 0.625f);
	Size->SetHeightOverride(IconSize.Y > 0.f ? IconSize.Y : DefaultSlotSize.Y * 0.625f);
	Size->SetContent(DragVisualImage);
	Size->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Size;
}

bool UItemSlotWidget::HasItem() const
{
	return CurrentItemKey.IsNone() == false && CurrentItemCount > 0;
}
