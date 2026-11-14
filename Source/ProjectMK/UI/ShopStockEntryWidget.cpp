#include "ProjectMK/UI/ShopStockEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "PaperSprite.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/CraftingComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/ShopTradeSubsystem.h"
#include "ProjectMK/UI/RecipeInfoWidget.h"
#include "ProjectMK/UI/ShopMenuContents.h"

UShopStockSlotWidget::UShopStockSlotWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
	bShowSlotTooltip = false;
}

void UShopStockSlotWidget::SetEntry(UShopStockEntryWidget* InEntry) { Entry = InEntry; }

FReply UShopStockSlotWidget::NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	bPendingClick = Event.GetEffectingButton() == EKeys::LeftMouseButton;
	return bPendingClick ? UWidgetBlueprintLibrary::DetectDragIfPressed(Event, this, EKeys::LeftMouseButton).NativeReply : FReply::Unhandled();
}

FReply UShopStockSlotWidget::NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (bPendingClick && Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bPendingClick = false;
		if (Entry.IsValid()) { Entry->RequestPurchase(); }
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(Geometry, Event);
}

void UShopStockSlotWidget::NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& Operation)
{
	bPendingClick = false;
	UShopStockDragDropOperation* Drag = NewObject<UShopStockDragDropOperation>();
	Drag->SourceEntry = Entry;
	Drag->Pivot = EDragPivot::CenterCenter;
	Drag->DefaultDragVisual = CreateDragVisualWidget();
	Operation = Drag;
}

bool UShopStockSlotWidget::CanClickSlot() const { return false; }

TSharedRef<SWidget> UShopStockEntryWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget == nullptr)
	{
		StockSlot = CreateWidget<UShopStockSlotWidget>(this);
		StockSlot->SetEntry(this);
		UImage* Icon = nullptr;
		UTextBlock* Name = nullptr;
		UTextBlock* Summary = nullptr;
		UButton* Button = nullptr;
		UTextBlock* Label = nullptr;
		WidgetTree->RootWidget = BuildRecipeEntry(Icon, Name, Summary, Button, Label, true, StockSlot, true);
		NameText = Name;
		SummaryText = Summary;
		BuyButton = Button;
		PriceText = Label;
		Info = CastChecked<URecipeInfoWidget>(WidgetTree->FindWidget(TEXT("RecipeInfo")));
		CurrencyIcon = CastChecked<UHorizontalBox>(Button->GetContent())->GetChildAt(0);
	}
	return Super::RebuildWidget();
}

void UShopStockEntryWidget::InitializeRecipe(UShopMenuContents* Shop, FName Key)
{
	OwnerShop = Shop;
	RecipeKey = Key;
	BuybackId.Invalidate();
	TakeWidget();
	RefreshStock();
}

void UShopStockEntryWidget::InitializeBuyback(UShopMenuContents* Shop, FGuid Id)
{
	OwnerShop = Shop;
	RecipeKey = NAME_None;
	BuybackId = Id;
	TakeWidget();
	RefreshStock();
}

void UShopStockEntryWidget::RequestPurchase()
{
	if (OwnerShop.IsValid()) { OwnerShop->RequestPurchase(RecipeKey, BuybackId); }
}

void UShopStockEntryWidget::BindEvents()
{
	Super::BindEvents();
	BuyButton->OnClicked.AddUniqueDynamic(this, &UShopStockEntryWidget::RequestPurchase);
}

void UShopStockEntryWidget::UnbindEvents()
{
	BuyButton->OnClicked.RemoveAll(this);
	Super::UnbindEvents();
}

void UShopStockEntryWidget::RefreshStock()
{
	const UDataManager* Data = UDataManager::Get(this);
	const FItemDataTableRow* Item = nullptr;
	int32 Price = 0, Count = 1;
	bool bCanBuy = false;
	if (RecipeKey.IsNone() == false)
	{
		const FShopRecipeDataTableRow* Recipe = Data->GetShopRecipeDataTableRow(RecipeKey);
		if (Recipe == nullptr) { return; }
		const bool bItemOffer = Recipe->OfferType == EShopOfferType::Item;
		Item = Data->GetRecipeItemData(RecipeKey, bItemOffer == false);
		NameText->SetText(bItemOffer ? Data->GetItemDisplayName(Recipe->ResultItemKey) : Data->GetRecipeDisplayName(RecipeKey, true));
		SummaryText->SetText(Recipe->ShortDescription);
		if (bItemOffer) { Info->SetItem(Recipe->ResultItemKey); }
		else { Info->SetRecipe(RecipeKey, true); }
		Price = bItemOffer ? Recipe->PurchasePrice : Recipe->UnlockPrice;
		bCanBuy = bItemOffer ? GetWorld()->GetSubsystem<UShopTradeSubsystem>()->GetMaximumPurchaseCount(GetLocalPlayerCharacter(), RecipeKey) > 0
			: GetLocalPlayerCharacter()->GetCraftingComponent()->CanUnlockRecipe(RecipeKey);
	}
	else
	{
		const UShopTradeSubsystem* Trade = GetWorld()->GetSubsystem<UShopTradeSubsystem>();
		const FShopBuybackEntry* Entry = Trade->FindBuyback(BuybackId);
		if (Entry == nullptr) { return; }
		Item = Data->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, Entry->ItemKey);
		NameText->SetText(Data->GetItemDisplayName(Entry->ItemKey));
		Info->SetItem(Entry->ItemKey);
		SummaryText->SetText(FText::Format(NSLOCTEXT("Shop", "StockCount", "보유 재고 {0}개"), FText::AsNumber(Entry->Count)));
		Price = Entry->UnitPrice;
		Count = Entry->Count;
		bCanBuy = Trade->GetMaximumBuybackCount(GetLocalPlayerCharacter(), BuybackId) > 0;
	}
	StockSlot->SetSlotIcon(Item != nullptr ? Item->ItemIcon.LoadSynchronous() : nullptr);
	StockSlot->SetSlotCount(Count, BuybackId.IsValid() && Count > 1);
	CurrencyIcon->SetVisibility(Price > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	PriceText->SetText(Price > 0 ? FText::Format(NSLOCTEXT("Shop", "PriceBuy", "{0} 구매"), FText::AsNumber(Price)) : NSLOCTEXT("Shop", "FreePurchase", "무료 구매"));
	BuyButton->SetIsEnabled(bCanBuy);
}
