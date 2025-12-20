// LINK

#include "ProjectMK/UI/ItemSlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "Components/CanvasPanelSlot.h"

void UItemSlotWidget::SetItem(FName ItemKey, int32 ItemCount)
{
	if (::IsValid(Image_Item) == false)
	{
		return;
	}

	Image_Item->SetVisibility(ESlateVisibility::Collapsed);

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemKey);
	if (ItemDataTableRow == nullptr)
	{
		return;
	}

	if (::IsValid(Text_Count))
	{
		Text_Count->SetText(FText::AsNumber(ItemCount));
	}

	if (::IsValid(Image_Item) && ItemDataTableRow->ItemIcon.IsNull() == false)
	{
		UPaperSprite* Sprite = ItemDataTableRow->ItemIcon.LoadSynchronous();
		if (::IsValid(Sprite) == false)
		{
			return;
		}

		UTexture2D* Tex = Sprite->GetSourceTexture();
		if (::IsValid(Tex) == false)
		{
			return;
		}

		// 잘라낸 영역 (픽셀 단위) 
		FVector2D SourceUV = Sprite->GetSourceUV(); 
		FVector2D SourceSize = Sprite->GetSourceSize(); 
		
		// 텍스처 전체 크기 
		float TexW = Tex->GetSizeX(); 
		float TexH = Tex->GetSizeY(); 
		
		FVector2D MinUV(SourceUV.X / TexW, SourceUV.Y / TexH); 
		FVector2D MaxUV((SourceUV.X + SourceSize.X) / TexW, (SourceUV.Y + SourceSize.Y) / TexH); 
		
		FSlateBrush Brush; 
		Brush.DrawAs = ESlateBrushDrawType::Image; 
		Brush.SetResourceObject(Tex); 
		Brush.ImageSize = FVector2D(64.f, 64.f); 
		Brush.SetUVRegion(FBox2D(MinUV, MaxUV)); 

		Image_Item->SetBrush(Brush); 
		Image_Item->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}
