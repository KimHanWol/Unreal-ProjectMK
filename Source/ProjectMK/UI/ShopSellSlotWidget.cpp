#include "ProjectMK/UI/ShopSellSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "PaperSprite.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/UI/ShopMenuContents.h"

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
		ItemPriceText->SetText(FText::Format(NSLOCTEXT("ShopMenu", "ItemPriceFormat", "{0} · 개당 {1}"), UnitPrice * ItemCount, UnitPrice));
	}

	const bool bSelected = OwnerShopMenu.IsValid() && OwnerShopMenu->IsItemSelectedForSale(ItemUID);
	if (::IsValid(SelectionText))
	{
		SelectionText->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (::IsValid(SellButtonText))
	{
		SellButtonText->SetText(bSelected ? NSLOCTEXT("ShopMenu", "Selected", "선택됨") : NSLOCTEXT("ShopMenu", "Select", "선택"));
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
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RecipeCard"));
	Card->SetPadding(FMargin(FMKUILayout::SpaceM));
	Card->SetContent(RootBox);
	WidgetTree->RootWidget = Card;

	SelectionText = WidgetTree->ConstructWidget<UTextBlock>();
	SelectionText->SetText(FText::FromString(TEXT("✓")));
	ApplyTextRole(SelectionText, EMKTextRole::Label);
	SelectionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.9f, 0.75f, 1.f)));
	SelectionText->SetVisibility(ESlateVisibility::Hidden);
	RootBox->AddChildToHorizontalBox(SelectionText)->SetPadding(FMargin(0.f, 0.f, FMKUILayout::SpaceS, 0.f));

	UImage* RawItemIcon = nullptr;
	UBorder* IconFrame = BuildItemIconFrame(RawItemIcon);
	ItemIconImage = RawItemIcon;
	if (UHorizontalBoxSlot* IconSlot = RootBox->AddChildToHorizontalBox(IconFrame))
	{
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, FMKUILayout::SpaceM, 0.f));
	}

	UVerticalBox* Details = WidgetTree->ConstructWidget<UVerticalBox>();
	ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemNameText"));
	ApplyTextRole(ItemNameText, EMKTextRole::Label, true);
	Details->AddChildToVerticalBox(ItemNameText);
	if (UHorizontalBoxSlot* NameSlot = RootBox->AddChildToHorizontalBox(Details))
	{
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UTextBlock* RawItemPriceText = nullptr;
	UHorizontalBox* PriceRow = BuildCurrencyAmount(RawItemPriceText, FText::GetEmpty(), EMKTextRole::Caption);
	ItemPriceText = RawItemPriceText;
	ItemPriceText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.75f, 0.4f)));
	Details->AddChildToVerticalBox(PriceRow)->SetPadding(FMargin(0.f, FMKUILayout::SpaceXS, 0.f, 0.f));

	UButton* RawSellButton = nullptr;
	UTextBlock* RawSellButtonText = nullptr;
	USizeBox* SellButtonSize = BuildActionButton(RawSellButton, RawSellButtonText, NSLOCTEXT("ShopMenu", "Select", "선택"));
	SellButton = RawSellButton;
	SellButtonText = RawSellButtonText;
	if (UHorizontalBoxSlot* ButtonSlot = RootBox->AddChildToHorizontalBox(SellButtonSize))
	{
		ButtonSlot->SetPadding(FMargin(FMKUILayout::SpaceM, 0.f, 0.f, 0.f));
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UShopSellSlotWidget::OnSellButtonClicked()
{
	if (OwnerShopMenu.IsValid())
	{
		OwnerShopMenu->ToggleSellSelection(ItemUID);
		UpdateWidget();
	}
}
