#include "ProjectMK/UI/ShopMenuContents.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/CraftingComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/SoundManager.h"
#include "ProjectMK/UI/InventoryWidget.h"
#include "ProjectMK/UI/ItemSlotDragDropOperation.h"
#include "ProjectMK/UI/ItemQuantityDialog.h"
#include "ProjectMK/UI/MenuDisplayCardWidget.h"
#include "ProjectMK/UI/ShopStockEntryWidget.h"
#include "ProjectMK/Core/Subsystem/ShopTradeSubsystem.h"
#include "Engine/World.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "ProjectMK/Core/Manager/DataManager.h"

UShopMenuContents::UShopMenuContents(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MenuContentsType = EMenuContentsType::Shop;
}

TSharedRef<SWidget> UShopMenuContents::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->FindWidget(TEXT("RuntimeShopRoot")) == nullptr) { BuildDefaultWidgetTree(); }
	return Super::RebuildWidget();
}

void UShopMenuContents::NativeDestruct()
{
	if (::IsValid(QuantityDialog)) { QuantityDialog->RemoveFromParent(); }
	Super::NativeDestruct();
}

bool UShopMenuContents::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	if (UShopStockDragDropOperation* Stock = Cast<UShopStockDragDropOperation>(Operation))
	{
		if (MerchantColumn->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition()) == false && Stock->SourceEntry.IsValid()) { Stock->SourceEntry->RequestPurchase(); }
		return true;
	}
	const UItemSlotDragDropOperation* Item = Cast<UItemSlotDragDropOperation>(Operation);
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	if (::IsValid(Item) && ::IsValid(Inventory) && MerchantColumn->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition()))
	{
		const TArray<FInventorySlotData>& Slots = Inventory->GetItemSlots(Item->SourceContainer);
		if (Slots.IsValidIndex(Item->SourceSlotIndex) == false || Slots[Item->SourceSlotIndex].ItemUID != Item->ItemKey) { return true; }
		const int32 Count = FMath::Min(Item->ItemCount, Slots[Item->SourceSlotIndex].ItemCount);
		if (Count > 0 && Inventory->CalculateSellPrice(Item->ItemKey, 1) > 0)
		{
			OnItemSaleRequested(Item->SourceContainer, Item->SourceSlotIndex, Item->ItemKey);
			return true;
		}
		return true;
	}
	return Super::NativeOnDrop(InGeometry, Event, Operation);
}

void UShopMenuContents::BindEvents()
{
	Super::BindEvents();
	GetWorld()->GetSubsystem<UShopTradeSubsystem>()->OnStockChanged.AddUObject(this, &UShopMenuContents::RefreshRecipeSlotList);
	GetLocalInventoryComponent()->OnInventoryChangedDelegate.AddUObject(this, &UShopMenuContents::RefreshRecipeSlotList);
	if (QuantityDialog->IsInViewport() == false) { QuantityDialog->AddToViewport(200); }
	OnVisibilityChanged.AddUniqueDynamic(this, &UShopMenuContents::OnMenuVisibilityChanged);
	QuantityDialog->OnQuantityConfirmed.AddUObject(this, &UShopMenuContents::OnSaleConfirmed);
	if (UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCoinAttribute()).AddUObject(this, &UShopMenuContents::OnCoinChanged);
	}
	if (AMKCharacter* Character = GetLocalPlayerCharacter())
	{
		Character->GetCraftingComponent()->OnRecipeUnlockedDelegate.AddUObject(this, &UShopMenuContents::OnRecipeUnlocked);
	}
}

void UShopMenuContents::UnbindEvents()
{
	Super::UnbindEvents();
	GetWorld()->GetSubsystem<UShopTradeSubsystem>()->OnStockChanged.RemoveAll(this);
	if (UInventoryComponent* Items = GetLocalInventoryComponent()) { Items->OnInventoryChangedDelegate.RemoveAll(this); }
	OnVisibilityChanged.RemoveDynamic(this, &UShopMenuContents::OnMenuVisibilityChanged);
	QuantityDialog->OnQuantityConfirmed.RemoveAll(this);
	if (UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCoinAttribute()).RemoveAll(this);
	}
	if (AMKCharacter* Character = GetLocalPlayerCharacter())
	{
		Character->GetCraftingComponent()->OnRecipeUnlockedDelegate.RemoveAll(this);
	}
}

void UShopMenuContents::UpdateWidget() { RefreshRecipeSlotList(); }

bool UShopMenuContents::TryUnlockRecipe(FName RecipeKey)
{
	AMKCharacter* Character = GetLocalPlayerCharacter();
	return ::IsValid(Character) && Character->GetCraftingComponent()->TryUnlockRecipe(RecipeKey);
}

void UShopMenuContents::SetMerchantDialogue(const FText& Dialogue)
{
	if (::IsValid(MerchantDialogue)) { MerchantDialogue->SetText(Dialogue); }
}

void UShopMenuContents::SetCatalogKey(FName Key)
{
	CatalogKey = Key;
	if (Key == TEXT("RailFactory"))
	{
		const FText Lines[] = {
			NSLOCTEXT("Town", "FactoryStock", "철도와 열차를 준비했습니다. 설계도를 배우면 작업대에서도 제작할 수 있어요."),
			NSLOCTEXT("Town", "FactorySlope", "경사에 놓은 수레는 스스로 굴러가니 바퀴를 잘 살펴보세요."),
			NSLOCTEXT("Town", "FactoryRecovery", "설치물을 거둘 때는 F를 꾹 눌러보세요. 상자, 열차, 철도 순서로 정리하면 됩니다.") };
		SetMerchantDialogue(Lines[FMath::RandRange(0, UE_ARRAY_COUNT(Lines) - 1)]);
	}
	else { SetMerchantDialogue(NSLOCTEXT("Shop", "SaleHint", "광물을 놓아 판매 · 우클릭으로 전부 판매")); }
	RefreshRecipeSlotList();
}

void UShopMenuContents::RequestPurchase(FName RecipeKey, FGuid BuybackId)
{
	if (UHUDWidget* HUD = GetHUDWidget()) { HUD->SetHoveredSlot(nullptr); }
	PendingBuybackId.Invalidate();
	PendingCatalogKey = NAME_None;
	if (RecipeKey.IsNone() == false)
	{
		const FShopRecipeDataTableRow* Offer = UDataManager::Get(this)->GetShopRecipeDataTableRow(RecipeKey);
		if (Offer == nullptr) { return; }
		if (Offer->OfferType == EShopOfferType::Recipe)
		{
			if (TryUnlockRecipe(RecipeKey)) { GetLocalPlayerCharacter()->TrySaveGame(); }
			return;
		}
		const int32 Maximum = GetWorld()->GetSubsystem<UShopTradeSubsystem>()->GetMaximumPurchaseCount(GetLocalPlayerCharacter(), RecipeKey);
		if (Maximum <= 0) { return; }
		PendingCatalogKey = RecipeKey;
		if (Maximum == 1) { OnSaleConfirmed(Offer->ResultItemKey, 1); }
		else { QuantityDialog->ShowPurchase(Offer->ResultItemKey, Maximum, Offer->PurchasePrice); }
		return;
	}
	UShopTradeSubsystem* Trade = GetWorld()->GetSubsystem<UShopTradeSubsystem>();
	const FShopBuybackEntry* Entry = Trade->FindBuyback(BuybackId);
	const int32 Maximum = Trade->GetMaximumBuybackCount(GetLocalPlayerCharacter(), BuybackId);
	if (Entry == nullptr || Maximum <= 0) { return; }
	PendingBuybackId = BuybackId;
	if (Maximum == 1) { OnSaleConfirmed(Entry->ItemKey, 1); }
	else { QuantityDialog->ShowPurchase(Entry->ItemKey, Maximum, Entry->UnitPrice); }
}

void UShopMenuContents::BuildDefaultWidgetTree()
{
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Overlay;
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RuntimeShopRoot"));
	ApplyBorderStyle(Panel, TEXT("InsetPanel"));
	Panel->SetPadding(FMargin(24.f));
	UOverlaySlot* PanelSlot = Overlay->AddChildToOverlay(Panel);
	PanelSlot->SetHorizontalAlignment(HAlign_Fill);
	PanelSlot->SetVerticalAlignment(VAlign_Fill);
	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Root);
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	Root->AddChildToVerticalBox(Columns)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	MerchantColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBoxSlot* MerchantSlot = Columns->AddChildToHorizontalBox(MerchantColumn);
	MerchantSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	MerchantSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	UMenuDisplayCardWidget* Merchant = CreateWidget<UMenuDisplayCardWidget>(this);
	Merchant->SetDisplayRole(TEXT("Shop"));
	MerchantColumn->AddChildToVerticalBox(Merchant);
	MerchantColumn->AddChildToVerticalBox(BuildSectionDivider());
	MerchantDialogue = BuildSectionLabel(NSLOCTEXT("Shop", "SellDialogue", "필요한 물품을 골라보세요.\n물품을 이쪽에 놓으면 원하는 수량을 매입해 드립니다."));
	ApplyTextRole(MerchantDialogue, EMKTextRole::Body, true);
	UBorder* SaleMat = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MerchantMat"));
	SaleMat->SetPadding(FMargin(24.f, 20.f));
	SaleMat->SetVerticalAlignment(VAlign_Center);
	SaleMat->SetContent(MerchantDialogue);
	USizeBox* DialogueSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MerchantDialogueArea"));
	DialogueSize->SetMinDesiredHeight(112.f);
	DialogueSize->SetContent(SaleMat);
	MerchantColumn->AddChildToVerticalBox(DialogueSize)->SetPadding(FMargin(0.f, 8.f, 0.f, 12.f));
	UBorder* CatalogFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ShopCatalogFrame"));
	ApplyBorderStyle(CatalogFrame, TEXT("InventoryDrawer"));
	CatalogFrame->SetPadding(FMargin(12.f, 24.f, 12.f, 20.f));
	MerchantColumn->AddChildToVerticalBox(CatalogFrame)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UScrollBox* Recipes = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ShopCatalogScroll"));
	Recipes->SetClipping(EWidgetClipping::ClipToBounds);
	CatalogFrame->SetContent(Recipes);
	UVerticalBox* Catalog = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShopCatalogList"));
	RecipeSlotListPanel = Catalog;
	Recipes->AddChild(Catalog);
	UVerticalBox* PlayerColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	USizeBox* InventoryWidth = WidgetTree->ConstructWidget<USizeBox>();
	InventoryWidth->SetWidthOverride(304.f);
	InventoryWidth->SetContent(PlayerColumn);
	Columns->AddChildToHorizontalBox(InventoryWidth);
	UMenuDisplayCardWidget* Player = CreateWidget<UMenuDisplayCardWidget>(this);
	Player->SetDisplayRole(TEXT("Player"));
	PlayerColumn->AddChildToVerticalBox(Player);
	PlayerColumn->AddChildToVerticalBox(BuildSectionDivider());
	UScrollBox* Items = WidgetTree->ConstructWidget<UScrollBox>();
	PlayerColumn->AddChildToVerticalBox(Items)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UInventoryWidget* PlayerInventory = CreateWidget<UInventoryWidget>(this);
	PlayerInventory->SetShowQuickSlots(true);
	PlayerInventory->OnItemSecondaryAction.AddUObject(this, &UShopMenuContents::OnSellStack);
	CastChecked<UScrollBoxSlot>(Items->AddChild(PlayerInventory))->SetHorizontalAlignment(HAlign_Center);
	QuantityDialog = CreateWidget<UItemQuantityDialog>(this);
	QuantityDialog->SetVisibility(ESlateVisibility::Collapsed);
}

void UShopMenuContents::RefreshRecipeSlotList()
{
	AMKCharacter* Character = GetLocalPlayerCharacter();
	if (::IsValid(RecipeSlotListPanel) == false || ::IsValid(Character) == false) { return; }
	RecipeSlotListPanel->ClearChildren();
	UCraftingComponent* Crafting = Character->GetCraftingComponent();
	TArray<FName> Keys = Crafting->GetLockedRecipeKeys();
	Keys.Sort(FNameLexicalLess());
	for (const FName Key : Keys)
	{
		if (UDataManager::Get(this)->GetShopRecipeDataTableRow(Key)->CatalogKey != CatalogKey) { continue; }
		UShopStockEntryWidget* Entry = CreateWidget<UShopStockEntryWidget>(this);
		CastChecked<UVerticalBoxSlot>(RecipeSlotListPanel->AddChild(Entry))->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		Entry->InitializeRecipe(this, Key);
	}
	TArray<FName> Offers = UDataManager::Get(this)->GetShopItemOfferRowNames();
	Offers.Sort(FNameLexicalLess());
	for (FName Key : Offers)
	{
		if (UDataManager::Get(this)->GetShopRecipeDataTableRow(Key)->CatalogKey != CatalogKey) { continue; }
		UShopStockEntryWidget* Entry = CreateWidget<UShopStockEntryWidget>(this);
		CastChecked<UVerticalBoxSlot>(RecipeSlotListPanel->AddChild(Entry))->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		Entry->InitializeRecipe(this, Key);
	}
	const UShopTradeSubsystem* Trade = GetWorld()->GetSubsystem<UShopTradeSubsystem>();
	for (const FShopBuybackEntry& Stock : Trade->GetBuybackEntries())
	{
		if (CatalogKey.IsNone() == false || Trade->FindBuyback(Stock.Id) == nullptr) { continue; }
		UShopStockEntryWidget* Entry = CreateWidget<UShopStockEntryWidget>(this);
		CastChecked<UVerticalBoxSlot>(RecipeSlotListPanel->AddChild(Entry))->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		Entry->InitializeBuyback(this, Stock.Id);
	}
}

void UShopMenuContents::OnItemSaleRequested(EItemContainer Container, int32 Index, FName ItemKey)
{
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	if (::IsValid(Inventory) == false) { return; }
	const TArray<FInventorySlotData>& Slots = Inventory->GetItemSlots(Container);
	if (Slots.IsValidIndex(Index) == false || Slots[Index].ItemUID != ItemKey || Inventory->CalculateSellPrice(ItemKey, 1) <= 0) { return; }
	PendingBuybackId.Invalidate();
	PendingCatalogKey = NAME_None;
	PendingSaleContainer = Container;
	PendingSaleIndex = Index;
	QuantityDialog->ShowQuantity(ItemKey, Slots[Index].ItemCount);
	if (UHUDWidget* HUD = GetHUDWidget()) { HUD->SetHoveredSlot(nullptr); }
}

void UShopMenuContents::OnSellStack(EItemContainer Container, int32 Index, FName ItemKey)
{
	const TArray<FInventorySlotData>& Slots = GetLocalInventoryComponent()->GetItemSlots(Container);
	if (Slots.IsValidIndex(Index) && Slots[Index].ItemUID == ItemKey)
	{
		if (GetWorld()->GetSubsystem<UShopTradeSubsystem>()->TrySell(GetLocalPlayerCharacter(), Container, Index, Slots[Index].ItemCount))
		{
			USoundManager::TryPlaySFX(this, ESFXType::Shop_Sell);
		}
	}
}

void UShopMenuContents::OnSaleConfirmed(FName ItemKey, int32 Count)
{
	UShopTradeSubsystem* Trade = GetWorld()->GetSubsystem<UShopTradeSubsystem>();
	bool bSucceeded = false;
	if (PendingCatalogKey.IsNone() == false) { bSucceeded = Trade->TryPurchaseItem(GetLocalPlayerCharacter(), PendingCatalogKey, Count); }
	else if (PendingBuybackId.IsValid()) { bSucceeded = Trade->TryBuyback(GetLocalPlayerCharacter(), PendingBuybackId, Count); }
	else
	{
		const TArray<FInventorySlotData>& Slots = GetLocalInventoryComponent()->GetItemSlots(PendingSaleContainer);
		if (Slots.IsValidIndex(PendingSaleIndex) && Slots[PendingSaleIndex].ItemUID == ItemKey) { bSucceeded = Trade->TrySell(GetLocalPlayerCharacter(), PendingSaleContainer, PendingSaleIndex, Count); }
	}
	PendingBuybackId.Invalidate();
	PendingCatalogKey = NAME_None;
	if (bSucceeded) { USoundManager::TryPlaySFX(this, ESFXType::Shop_Sell); }
	else { SetMerchantDialogue(NSLOCTEXT("Shop", "TransactionUnavailable", "거래할 수 없습니다. 재화·가방 공간·남은 재고를 확인해주세요.")); }
}

void UShopMenuContents::OnCoinChanged(const FOnAttributeChangeData& Data) { RefreshRecipeSlotList(); }
void UShopMenuContents::OnRecipeUnlocked(FName RecipeKey) { RefreshRecipeSlotList(); }

void UShopMenuContents::OnMenuVisibilityChanged(ESlateVisibility InVisibility)
{
	QuantityDialog->SetVisibility(ESlateVisibility::Collapsed);
	PendingBuybackId.Invalidate();
	PendingCatalogKey = NAME_None;
	if (InVisibility != ESlateVisibility::Collapsed && InVisibility != ESlateVisibility::Hidden)
	{
		RefreshRecipeSlotList();
	}
}
