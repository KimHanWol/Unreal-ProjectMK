// LINK

#include "ProjectMK/UI/MKTooltipBase.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"

namespace
{
	const FVector2D TooltipItemIconBrushSize(64.f, 64.f);

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
		Brush.ImageSize = TooltipItemIconBrushSize;
		Brush.SetUVRegion(FBox2D(MinUV, MaxUV));

		TargetImage->SetBrush(Brush);
		TargetImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return true;
	}
}

void UMKTooltipBase::ShowItemTooltip(FName ItemDataKey)
{
	ClearTooltip();

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false || ItemDataKey.IsNone())
	{
		SetVisible(false);
		return;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemDataKey);
	if (ItemDataTableRow == nullptr)
	{
		SetVisible(false);
		return;
	}

	UpdateTooltipFromItemData(ItemDataKey, *ItemDataTableRow);
	SetVisible(true);
}

void UMKTooltipBase::ShowSkillTooltip(FName SkillDataKey)
{
	ClearTooltip();

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false || SkillDataKey.IsNone())
	{
		SetVisible(false);
		return;
	}

	const FSkillDataTableRow* SkillDataTableRow = DataManager->GetSkillDataTableRow(SkillDataKey);
	if (SkillDataTableRow == nullptr)
	{
		SetVisible(false);
		return;
	}

	UpdateTooltipFromSkillData(*SkillDataTableRow);
	SetVisible(true);
}

void UMKTooltipBase::HideTooltip()
{
	ClearTooltip();
	SetVisible(false);
}

void UMKTooltipBase::UpdateWidget()
{
	ClearTooltip();
	SetVisible(false);
}

void UMKTooltipBase::ClearTooltip()
{
	if (::IsValid(Title))
	{
		Title->SetText(FText::GetEmpty());
	}

	if (::IsValid(Desc))
	{
		Desc->SetText(FText::GetEmpty());
	}

	if (::IsValid(Icon))
	{
		Icon->SetBrush(FSlateBrush());
		Icon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMKTooltipBase::UpdateTooltipFromItemData(FName ItemDataKey, const FItemDataTableRow& ItemDataTableRow)
{
	if (::IsValid(Title))
	{
		Title->SetText(FText::FromName(ItemDataKey));
	}

	if (ItemDataTableRow.ItemIcon.IsNull() == false)
	{
		UpdateTooltipIcon(ItemDataTableRow.ItemIcon.LoadSynchronous());
	}
}

void UMKTooltipBase::UpdateTooltipFromSkillData(const FSkillDataTableRow& SkillDataTableRow)
{
	if (::IsValid(Title))
	{
		Title->SetText(SkillDataTableRow.DisplayName);
	}

	if (::IsValid(Desc))
	{
		Desc->SetText(SkillDataTableRow.Description);
	}

	if (SkillDataTableRow.IconPath.IsNull() == false)
	{
		UpdateTooltipIcon(SkillDataTableRow.IconPath.LoadSynchronous());
	}
}

void UMKTooltipBase::UpdateTooltipIcon(UTexture2D* IconTexture)
{
	if (::IsValid(Icon) == false || ::IsValid(IconTexture) == false)
	{
		return;
	}

	Icon->SetBrushFromTexture(IconTexture, true);
	Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UMKTooltipBase::UpdateTooltipIcon(UPaperSprite* IconSprite)
{
	if (::IsValid(IconSprite) == false)
	{
		return;
	}

	TrySetBrushFromTextureRegion(Icon, IconSprite->GetSourceTexture(), IconSprite->GetSourceUV(), IconSprite->GetSourceSize());
}
