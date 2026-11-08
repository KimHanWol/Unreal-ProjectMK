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
#include "ProjectMK/UI/ShopRecipeSlotWidget.h"

UShopMenuContents::UShopMenuContents(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MenuContentsType = EMenuContentsType::Shop;
}

TSharedRef<SWidget> UShopMenuContents::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr) { BuildDefaultWidgetTree(); }
	return Super::RebuildWidget();
}

bool UShopMenuContents::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	const UItemSlotDragDropOperation* Item = Cast<UItemSlotDragDropOperation>(Operation);
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	if (::IsValid(Item) && ::IsValid(Inventory) && MerchantColumn->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition()))
	{
		const TArray<FInventorySlotData>& Slots = Inventory->GetItemSlots(Item->SourceContainer);
		if (Slots.IsValidIndex(Item->SourceSlotIndex) == false || Slots[Item->SourceSlotIndex].ItemUID != Item->ItemKey) { return true; }
		const int32 Count = FMath::Min(Item->ItemCount, Slots[Item->SourceSlotIndex].ItemCount);
		if (Count > 0 && Inventory->CalculateSellPrice(Item->ItemKey, 1) > 0)
		{
			PendingSaleContainer = Item->SourceContainer;
			PendingSaleIndex = Item->SourceSlotIndex;
			QuantityDialog->ShowQuantity(Item->ItemKey, Count);
			return true;
		}
		return true;
	}
	return Super::NativeOnDrop(InGeometry, Event, Operation);
}

void UShopMenuContents::BindEvents()
{
	Super::BindEvents();
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

void UShopMenuContents::BuildDefaultWidgetTree()
{
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Overlay;
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InsetPanel"));
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
	MerchantColumn->AddChildToVerticalBox(BuildSectionLabel(NSLOCTEXT("Shop", "Blueprints", "설계도")))->SetPadding(FMargin(8.f));
	UScrollBox* Recipes = WidgetTree->ConstructWidget<UScrollBox>();
	MerchantColumn->AddChildToVerticalBox(Recipes)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RecipeSlotListPanel = WidgetTree->ConstructWidget<UVerticalBox>();
	Recipes->AddChild(RecipeSlotListPanel);
	UVerticalBox* PlayerColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	Columns->AddChildToHorizontalBox(PlayerColumn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UMenuDisplayCardWidget* Player = CreateWidget<UMenuDisplayCardWidget>(this);
	Player->SetDisplayRole(TEXT("Player"));
	PlayerColumn->AddChildToVerticalBox(Player);
	PlayerColumn->AddChildToVerticalBox(BuildSectionDivider());
	UScrollBox* Items = WidgetTree->ConstructWidget<UScrollBox>();
	PlayerColumn->AddChildToVerticalBox(Items)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UInventoryWidget* PlayerInventory = CreateWidget<UInventoryWidget>(this);
	PlayerInventory->SetShowQuickSlots(true);
	CastChecked<UScrollBoxSlot>(Items->AddChild(PlayerInventory))->SetHorizontalAlignment(HAlign_Center);
	QuantityDialog = CreateWidget<UItemQuantityDialog>(this);
	UOverlaySlot* DialogSlot = Overlay->AddChildToOverlay(QuantityDialog);
	DialogSlot->SetHorizontalAlignment(HAlign_Fill);
	DialogSlot->SetVerticalAlignment(VAlign_Fill);
	QuantityDialog->SetVisibility(ESlateVisibility::Collapsed);
}

void UShopMenuContents::RefreshRecipeSlotList()
{
	AMKCharacter* Character = GetLocalPlayerCharacter();
	if (::IsValid(RecipeSlotListPanel) == false || ::IsValid(Character) == false) { return; }
	RecipeSlotListPanel->ClearChildren();
	UCraftingComponent* Crafting = Character->GetCraftingComponent();
	for (const FName Key : Crafting->GetLockedRecipeKeys())
	{
		UShopRecipeSlotWidget* Entry = CreateWidget<UShopRecipeSlotWidget>(this);
		CastChecked<UVerticalBox>(RecipeSlotListPanel)->AddChildToVerticalBox(Entry)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		Entry->InitializeRecipeSlot(this, Key, Crafting->CanUnlockRecipe(Key));
	}
}

void UShopMenuContents::OnSaleConfirmed(FName ItemKey, int32 Count)
{
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	if (::IsValid(Inventory) == false) { return; }
	const TArray<FInventorySlotData>& Slots = Inventory->GetItemSlots(PendingSaleContainer);
	if (Slots.IsValidIndex(PendingSaleIndex) && Slots[PendingSaleIndex].ItemUID == ItemKey && Inventory->TrySellItemSlot(PendingSaleContainer, PendingSaleIndex, Count))
	{
		USoundManager::TryPlaySFX(this, ESFXType::Shop_Sell);
		if (AMKCharacter* Character = GetLocalPlayerCharacter()) { Character->TrySaveGame(); }
	}
}

void UShopMenuContents::OnCoinChanged(const FOnAttributeChangeData& Data) { RefreshRecipeSlotList(); }
void UShopMenuContents::OnRecipeUnlocked(FName RecipeKey) { RefreshRecipeSlotList(); }

void UShopMenuContents::OnMenuVisibilityChanged(ESlateVisibility InVisibility)
{
	QuantityDialog->SetVisibility(ESlateVisibility::Collapsed);
	if (InVisibility != ESlateVisibility::Collapsed && InVisibility != ESlateVisibility::Hidden)
	{
		RefreshRecipeSlotList();
	}
}
