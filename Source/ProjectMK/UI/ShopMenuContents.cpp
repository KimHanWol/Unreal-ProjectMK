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
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
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

void UShopMenuContents::RequestPurchase(FName RecipeKey, FGuid BuybackId)
{
	if (UHUDWidget* HUD = GetHUDWidget()) { HUD->SetHoveredSlot(nullptr); }
	if (RecipeKey.IsNone() == false)
	{
		if (TryUnlockRecipe(RecipeKey)) { GetLocalPlayerCharacter()->TrySaveGame(); }
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
	MerchantColumn->AddChildToVerticalBox(BuildSectionLabel(NSLOCTEXT("Shop", "Purchase", "구매")))->SetPadding(FMargin(8.f, 8.f, 8.f, 16.f));
	UScrollBox* Recipes = WidgetTree->ConstructWidget<UScrollBox>();
	MerchantColumn->AddChildToVerticalBox(Recipes)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UVerticalBox* StockColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	Recipes->AddChild(StockColumn);
	UWrapBox* Catalog = WidgetTree->ConstructWidget<UWrapBox>();
	Catalog->SetInnerSlotPadding(FVector2D(8.f));
	RecipeSlotListPanel = Catalog;
	StockColumn->AddChildToVerticalBox(Catalog);
	BuybackHeading = BuildSectionLabel(NSLOCTEXT("Shop", "Buyback", "재구매"));
	StockColumn->AddChildToVerticalBox(BuybackHeading)->SetPadding(FMargin(8.f, 20.f, 8.f, 12.f));
	UWrapBox* Recent = WidgetTree->ConstructWidget<UWrapBox>();
	Recent->SetInnerSlotPadding(FVector2D(8.f));
	BuybackPanel = Recent;
	StockColumn->AddChildToVerticalBox(Recent);
	UVerticalBox* PlayerColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	USizeBox* InventoryWidth = WidgetTree->ConstructWidget<USizeBox>();
	InventoryWidth->SetWidthOverride(304.f);
	InventoryWidth->SetContent(PlayerColumn);
	Columns->AddChildToHorizontalBox(InventoryWidth);
	UMenuDisplayCardWidget* Player = CreateWidget<UMenuDisplayCardWidget>(this);
	Player->SetDisplayRole(TEXT("Player"));
	PlayerColumn->AddChildToVerticalBox(Player);
	PlayerColumn->AddChildToVerticalBox(BuildSectionDivider());
	PlayerColumn->AddChildToVerticalBox(BuildSectionLabel(NSLOCTEXT("Shop", "Sale", "판매")))->SetPadding(FMargin(8.f, 8.f, 8.f, 16.f));
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
	BuybackPanel->ClearChildren();
	UCraftingComponent* Crafting = Character->GetCraftingComponent();
	TArray<FName> Keys = Crafting->GetLockedRecipeKeys();
	Keys.Sort(FNameLexicalLess());
	for (const FName Key : Keys)
	{
		UShopStockEntryWidget* Entry = CreateWidget<UShopStockEntryWidget>(this);
		RecipeSlotListPanel->AddChild(Entry);
		Entry->InitializeRecipe(this, Key);
	}
	const UShopTradeSubsystem* Trade = GetWorld()->GetSubsystem<UShopTradeSubsystem>();
	for (const FShopBuybackEntry& Stock : Trade->GetBuybackEntries())
	{
		if (Trade->FindBuyback(Stock.Id) == nullptr) { continue; }
		UShopStockEntryWidget* Entry = CreateWidget<UShopStockEntryWidget>(this);
		BuybackPanel->AddChild(Entry);
		Entry->InitializeBuyback(this, Stock.Id);
	}
	BuybackHeading->SetVisibility(BuybackPanel->GetChildrenCount() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UShopMenuContents::OnItemSaleRequested(EItemContainer Container, int32 Index, FName ItemKey)
{
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	if (::IsValid(Inventory) == false) { return; }
	const TArray<FInventorySlotData>& Slots = Inventory->GetItemSlots(Container);
	if (Slots.IsValidIndex(Index) == false || Slots[Index].ItemUID != ItemKey || Inventory->CalculateSellPrice(ItemKey, 1) <= 0) { return; }
	PendingBuybackId.Invalidate();
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
	if (PendingBuybackId.IsValid()) { bSucceeded = Trade->TryBuyback(GetLocalPlayerCharacter(), PendingBuybackId, Count); }
	else
	{
		const TArray<FInventorySlotData>& Slots = GetLocalInventoryComponent()->GetItemSlots(PendingSaleContainer);
		if (Slots.IsValidIndex(PendingSaleIndex) && Slots[PendingSaleIndex].ItemUID == ItemKey) { bSucceeded = Trade->TrySell(GetLocalPlayerCharacter(), PendingSaleContainer, PendingSaleIndex, Count); }
	}
	PendingBuybackId.Invalidate();
	if (bSucceeded) { USoundManager::TryPlaySFX(this, ESFXType::Shop_Sell); }
	else { SetMerchantDialogue(NSLOCTEXT("Shop", "TransactionUnavailable", "거래할 수 없습니다. 재화·가방 공간·남은 재고를 확인해주세요.")); }
}

void UShopMenuContents::OnCoinChanged(const FOnAttributeChangeData& Data) { RefreshRecipeSlotList(); }
void UShopMenuContents::OnRecipeUnlocked(FName RecipeKey) { RefreshRecipeSlotList(); }

void UShopMenuContents::OnMenuVisibilityChanged(ESlateVisibility InVisibility)
{
	QuantityDialog->SetVisibility(ESlateVisibility::Collapsed);
	PendingBuybackId.Invalidate();
	if (InVisibility != ESlateVisibility::Collapsed && InVisibility != ESlateVisibility::Hidden)
	{
		RefreshRecipeSlotList();
	}
}
