// LINK

#include "ProjectMK/UI/MKTooltipBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
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

TSharedRef<SWidget> UMKTooltipBase::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->FindWidget(TEXT("RuntimeTooltipRoot")) == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
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

void UMKTooltipBase::BuildDefaultWidgetTree()
{
	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RuntimeTooltipRoot"));
	RootSize->SetWidthOverride(340.f);
	WidgetTree->RootWidget = RootSize;

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Tooltip"));
	Frame->SetPadding(FMargin(FMKUILayout::SpaceM));
	RootSize->SetContent(Frame);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Frame->SetContent(Row);

	UImage* IconImage = nullptr;
	UBorder* IconFrame = BuildItemIconFrame(IconImage, 48.f);
	Icon = IconImage;
	UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconFrame);
	IconSlot->SetVerticalAlignment(VAlign_Top);
	IconSlot->SetPadding(FMargin(0.f, 0.f, FMKUILayout::SpaceM, 0.f));

	UVerticalBox* CopyColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBoxSlot* CopySlot = Row->AddChildToHorizontalBox(CopyColumn);
	CopySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	ApplyTextRole(Title, EMKTextRole::Label, true);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.82f, 0.48f)));
	CopyColumn->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 0.f, 0.f, FMKUILayout::SpaceXS));

	Desc = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Desc"));
	ApplyTextRole(Desc, EMKTextRole::Body, true);
	Desc->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.84f, 0.86f)));
	CopyColumn->AddChildToVerticalBox(Desc);
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
		Desc->SetText(FText::Format(NSLOCTEXT("Tooltip", "SkillPrice", "{0}\n가격  {1}\n길게 눌러 구매"),
			SkillDataTableRow.Description, FText::AsNumber(SkillDataTableRow.Price)));
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
