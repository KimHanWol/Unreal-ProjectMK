#include "ProjectMK/UI/ShopSellSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "PaperSprite.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/UI/ShopMenuContents.h"
#include "ProjectMK/UI/MKMockupButton.h"

TSharedRef<SWidget> UShopSellSlotWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

void UShopSellSlotWidget::BindEvents()
{
	if (::IsValid(SellButton))
	{
		SellButton->OnClicked.AddUniqueDynamic(this, &UShopSellSlotWidget::OnSellButtonClicked);
	}
}

void UShopSellSlotWidget::UnbindEvents()
{
	if (::IsValid(SellButton))
	{
		SellButton->OnClicked.RemoveDynamic(this, &UShopSellSlotWidget::OnSellButtonClicked);
	}
}

void UShopSellSlotWidget::UpdateWidget()
{
	// NativeConstruct 에서 InitializeSellSlot 이전에 먼저 호출된다.
	if (ItemUID.IsNone())
	{
		return;
	}

	if (::IsValid(ItemNameText))
	{
		ItemNameText->SetText(FText::Format(NSLOCTEXT("ShopMenu", "ItemNameFormat", "{0} x{1}"), FText::FromName(ItemUID), ItemCount));
	}

	if (::IsValid(ItemPriceText))
	{
		ItemPriceText->SetText(FText::Format(NSLOCTEXT("ShopMenu", "ItemPriceFormat", "{0} 코인 (개당 {1})"), UnitPrice * ItemCount, UnitPrice));
	}

	if (::IsValid(ItemIconImage) == false)
	{
		return;
	}

	const UDataManager* DataManager = UDataManager::Get(this);
	const FItemDataTableRow* ItemDataTableRow = ::IsValid(DataManager) ? DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemUID) : nullptr;
	UPaperSprite* ItemIconSprite = ItemDataTableRow != nullptr ? ItemDataTableRow->ItemIcon.LoadSynchronous() : nullptr;
	if (::IsValid(ItemIconSprite) == false)
	{
		ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	ItemIconImage->SetBrushResourceObject(ItemIconSprite);
	ItemIconImage->SetDesiredSizeOverride(ItemIconSize);
	ItemIconImage->SetVisibility(ESlateVisibility::Visible);
}

void UShopSellSlotWidget::InitializeSellSlot(UShopMenuContents* InOwnerShopMenu, FName InItemUID, int32 InItemCount, int32 InUnitPrice)
{
	OwnerShopMenu = InOwnerShopMenu;
	ItemUID = InItemUID;
	ItemCount = InItemCount;
	UnitPrice = InUnitPrice;
	UpdateWidget();
}

void UShopSellSlotWidget::BuildDefaultWidgetTree()
{
	UHorizontalBox* RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RootBox"));
	WidgetTree->RootWidget = RootBox;

	ItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIconImage"));
	if (UHorizontalBoxSlot* IconSlot = RootBox->AddChildToHorizontalBox(ItemIconImage))
	{
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}

	ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemNameText"));
	if (UHorizontalBoxSlot* NameSlot = RootBox->AddChildToHorizontalBox(ItemNameText))
	{
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	ItemPriceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemPriceText"));
	if (UHorizontalBoxSlot* PriceSlot = RootBox->AddChildToHorizontalBox(ItemPriceText))
	{
		PriceSlot->SetVerticalAlignment(VAlign_Center);
		PriceSlot->SetPadding(FMargin(8.f, 0.f));
	}

	SellButton = WidgetTree->ConstructWidget<UMKMockupButton>(UMKMockupButton::StaticClass(), TEXT("SellButton"));
	UTextBlock* SellButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SellButtonText"));
	SellButtonText->SetText(NSLOCTEXT("ShopMenu", "Sell", "판매"));
	SellButton->AddChild(SellButtonText);
	if (UHorizontalBoxSlot* ButtonSlot = RootBox->AddChildToHorizontalBox(SellButton))
	{
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UShopSellSlotWidget::OnSellButtonClicked()
{
	if (OwnerShopMenu.IsValid())
	{
		OwnerShopMenu->TrySellItem(ItemUID);
	}
}
