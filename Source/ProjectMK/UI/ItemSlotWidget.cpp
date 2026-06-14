// LINK

#include "ProjectMK/UI/ItemSlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Helper/Utils/EquipmentItemDataTableUtil.h"

namespace
{
	const FVector2D InventoryIconBrushSize(64.f, 64.f);

	bool TrySetBrushFromTextureRegion(UImage* TargetImage, UTexture2D* Texture, const FVector2D& SourceUV, const FVector2D& SourceSize)
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

		return TrySetBrushFromTextureRegion(TargetImage, Sprite->GetSourceTexture(), Sprite->GetSourceUV(), Sprite->GetSourceSize());
	}
}

void UItemSlotWidget::ClearItem()
{
	if (::IsValid(Text_Count))
	{
		Text_Count->SetText(FText::GetEmpty());
	}

	if (::IsValid(Image_Item))
	{
		Image_Item->SetBrush(FSlateBrush());
		Image_Item->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UItemSlotWidget::SetItem(FName ItemKey, int32 ItemCount)
{
	if (::IsValid(Image_Item) == false)
	{
		return;
	}

	ClearItem();

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	if (::IsValid(Text_Count))
	{
		Text_Count->SetText(FText::AsNumber(ItemCount));
	}

	if (const FEquipmentItemDataTableRow* EquipmentData = FEquipmentItemDataTableUtil::FindEquipmentItemData(this, ItemKey))
	{
		if (EquipmentData->EquipmentIcon.IsNull() == false)
		{
			if (TrySetBrushFromSprite(Image_Item, EquipmentData->EquipmentIcon.LoadSynchronous()))
			{
				return;
			}
		}

		UTexture2D* PreviewTexture = nullptr;
		FVector2D SourceUV = FVector2D::ZeroVector;
		FVector2D SourceSize = FVector2D::ZeroVector;
		if (FEquipmentItemDataTableUtil::GetIdlePreviewTextureRegion(*EquipmentData, PreviewTexture, SourceUV, SourceSize))
		{
			if (TrySetBrushFromTextureRegion(Image_Item, PreviewTexture, SourceUV, SourceSize))
			{
				return;
			}
		}

		if (UPaperSprite* StatePreviewSprite = FEquipmentItemDataTableUtil::LoadStatePreviewSprite(*EquipmentData))
		{
			if (TrySetBrushFromSprite(Image_Item, StatePreviewSprite))
			{
				return;
			}
		}
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemKey);
	if (ItemDataTableRow == nullptr)
	{
		return;
	}

	if (::IsValid(Image_Item) && ItemDataTableRow->ItemIcon.IsNull() == false)
	{
		TrySetBrushFromSprite(Image_Item, ItemDataTableRow->ItemIcon.LoadSynchronous());
	}
}
