// LINK

#include "ProjectMK/UI/ItemSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/UI/ItemSlotDragDropOperation.h"
#include "ProjectMK/UI/MKTooltipBase.h"

namespace
{
	const FVector2D InventoryIconBrushSize(64.f, 64.f);

	bool TrySetInventoryBrushFromTextureRegion(UImage* TargetImage, UTexture2D* Texture, const FVector2D& SourceUV, const FVector2D& SourceSize)
	{
		if (::IsValid(TargetImage) == false || ::IsValid(Texture) == false)
		{
			return false;
		}

		const float TextureWidth = static_cast<float>(Texture->GetSizeX());
		const float TextureHeight = static_cast<float>(Texture->GetSizeY());
		if (TextureWidth <= 0.f || TextureHeight <= 0.f)
		{
			return false;
		}

		const FVector2D MinUV(SourceUV.X / TextureWidth, SourceUV.Y / TextureHeight);
		const FVector2D MaxUV((SourceUV.X + SourceSize.X) / TextureWidth, (SourceUV.Y + SourceSize.Y) / TextureHeight);

		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = InventoryIconBrushSize;
		Brush.SetUVRegion(FBox2D(MinUV, MaxUV));

		TargetImage->SetBrush(Brush);
		TargetImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return true;
	}

	bool TrySetBrushFromSprite(UImage* TargetImage, UPaperSprite* Sprite)
	{
		if (::IsValid(Sprite) == false)
		{
			return false;
		}

		return TrySetInventoryBrushFromTextureRegion(TargetImage, Sprite->GetSourceTexture(), Sprite->GetSourceUV(), Sprite->GetSourceSize());
	}
}

void UItemSlotWidget::ClearItem()
{
	CurrentItemKey = NAME_None;
	CurrentItemCount = 0;

	if (::IsValid(Text_Count))
	{
		Text_Count->SetText(FText::GetEmpty());
	}

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

	if (::IsValid(Text_Count))
	{
		Text_Count->SetText(FText::AsNumber(ItemCount));
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemKey);
	if (ItemDataTableRow == nullptr)
	{
		return;
	}

	if (::IsValid(IconImage) && ItemDataTableRow->ItemIcon.IsNull() == false)
	{
		TrySetBrushFromSprite(IconImage, ItemDataTableRow->ItemIcon.LoadSynchronous());
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
	if (HasItem())
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
	DragDropOperation->Payload = this;
	DragDropOperation->Pivot = EDragPivot::MouseDown;
	DragDropOperation->DefaultDragVisual = CreateDragVisualWidget();
	OutOperation = DragDropOperation;
}

bool UItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UItemSlotDragDropOperation* DragDropOperation = Cast<UItemSlotDragDropOperation>(InOperation);
	if (::IsValid(DragDropOperation) == false || SlotIndex == INDEX_NONE)
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
