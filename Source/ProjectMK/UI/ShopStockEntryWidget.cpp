#include "ProjectMK/UI/ShopStockEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
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
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(132.f);
		Size->SetHeightOverride(174.f);
		WidgetTree->RootWidget = Size;
		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
		ApplyBorderStyle(Frame, TEXT("RecipeCard"));
		Frame->SetPadding(FMargin(10.f));
		Size->SetContent(Frame);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Frame->SetContent(Column);
		Info = CreateWidget<URecipeInfoWidget>(this);
		Column->AddChildToVerticalBox(Info)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UVerticalBox* Description = WidgetTree->ConstructWidget<UVerticalBox>();
		Info->SetInfoContent(Description);
		StockSlot = CreateWidget<UShopStockSlotWidget>(this);
		StockSlot->SetEntry(this);
		Description->AddChildToVerticalBox(StockSlot)->SetHorizontalAlignment(HAlign_Center);
		NameText = WidgetTree->ConstructWidget<UTextBlock>();
		ApplyTextRole(NameText, EMKTextRole::Caption, true);
		NameText->SetJustification(ETextJustify::Center);
		Description->AddChildToVerticalBox(NameText)->SetPadding(FMargin(0.f, 6.f, 0.f, 6.f));
		UButton* Button = nullptr;
		UTextBlock* Label = nullptr;
		Column->AddChildToVerticalBox(BuildCurrencyButton(Button, Label, FText::GetEmpty(), 108.f, 32.f));
		BuyButton = Button;
		PriceText = Label;
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
		Item = Data->GetRecipeItemData(RecipeKey, true);
		NameText->SetText(Data->GetRecipeDisplayName(RecipeKey, true));
		Info->SetRecipe(RecipeKey, true);
		Price = Recipe->UnlockPrice;
		bCanBuy = GetLocalPlayerCharacter()->GetCraftingComponent()->CanUnlockRecipe(RecipeKey);
	}
	else
	{
		const UShopTradeSubsystem* Trade = GetWorld()->GetSubsystem<UShopTradeSubsystem>();
		const FShopBuybackEntry* Entry = Trade->FindBuyback(BuybackId);
		if (Entry == nullptr) { return; }
		Item = Data->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, Entry->ItemKey);
		NameText->SetText(Data->GetItemDisplayName(Entry->ItemKey));
		Info->SetItem(Entry->ItemKey);
		Price = Entry->UnitPrice;
		Count = Entry->Count;
		bCanBuy = Trade->GetMaximumBuybackCount(GetLocalPlayerCharacter(), BuybackId) > 0;
	}
	StockSlot->SetSlotIcon(Item != nullptr ? Item->ItemIcon.LoadSynchronous() : nullptr);
	StockSlot->SetSlotCount(Count, BuybackId.IsValid());
	PriceText->SetText(FText::Format(NSLOCTEXT("Shop", "PriceBuy", "{0} 구매"), FText::AsNumber(Price)));
	BuyButton->SetIsEnabled(bCanBuy);
}
