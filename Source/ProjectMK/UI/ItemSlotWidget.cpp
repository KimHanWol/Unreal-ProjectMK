// LINK

#include "ProjectMK/UI/ItemSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "PaperSprite.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/UI/ItemSlotDragDropOperation.h"
#include "ProjectMK/UI/MKTooltipBase.h"

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

FReply UItemSlotWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsSlotLocked == false && HasItem())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
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
	DragDropOperation->ItemKey = CurrentItemKey;
	DragDropOperation->ItemCount = CurrentItemCount;
	DragDropOperation->Payload = this;
	DragDropOperation->Pivot = EDragPivot::MouseDown;
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

	return InventoryComponent->TryMoveItemSlot(DragDropOperation->SourceSlotIndex, SlotIndex);
}

bool UItemSlotWidget::CanClickSlot() const
{
	return false;
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
	return DragVisualImage;
}

bool UItemSlotWidget::HasItem() const
{
	return CurrentItemKey.IsNone() == false && CurrentItemCount > 0;
}
