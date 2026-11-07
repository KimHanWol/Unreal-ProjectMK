#include "ProjectMK/UI/ShopMenuContents.h"

#include "Blueprint/WidgetTree.h"
#include "AbilitySystemComponent.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/CraftingComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/SoundManager.h"
#include "ProjectMK/UI/ShopRecipeSlotWidget.h"
#include "ProjectMK/UI/ShopSellSlotWidget.h"

namespace
{
	UCraftingComponent* GetCharacterCraftingComponent(const AMKCharacter* Character)
	{
		return ::IsValid(Character) ? Character->GetCraftingComponent() : nullptr;
	}
}

UShopMenuContents::UShopMenuContents(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MenuContentsType = EMenuContentsType::Shop;
	SellSlotWidgetClass = UShopSellSlotWidget::StaticClass();
	RecipeSlotWidgetClass = UShopRecipeSlotWidget::StaticClass();
}

TSharedRef<SWidget> UShopMenuContents::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->FindWidget(TEXT("RuntimeShopRoot")) == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

void UShopMenuContents::BindEvents()
{
	if (UAbilitySystemComponent* AbilitySystem = GetOwnerAbilitySystemComponent())
	{
		AbilitySystem->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCoinAttribute()).AddUObject(this, &UShopMenuContents::OnCoinChanged);
	}
	if (::IsValid(SellAllButton))
	{
		SellAllButton->OnClicked.AddUniqueDynamic(this, &UShopMenuContents::OnSellAllButtonClicked);
	}

	if (UInventoryComponent* InventoryComponent = GetLocalInventoryComponent())
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
		InventoryComponent->OnInventoryChangedDelegate.AddUObject(this, &UShopMenuContents::OnInventoryChanged);
	}

	if (UCraftingComponent* CraftingComponent = GetCharacterCraftingComponent(GetLocalPlayerCharacter()))
	{
		CraftingComponent->OnRecipeUnlockedDelegate.RemoveAll(this);
		CraftingComponent->OnRecipeUnlockedDelegate.AddUObject(this, &UShopMenuContents::OnRecipeUnlocked);
	}
}

void UShopMenuContents::UnbindEvents()
{
	if (UAbilitySystemComponent* AbilitySystem = GetOwnerAbilitySystemComponent())
	{
		AbilitySystem->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCoinAttribute()).RemoveAll(this);
	}
	if (::IsValid(SellAllButton))
	{
		SellAllButton->OnClicked.RemoveDynamic(this, &UShopMenuContents::OnSellAllButtonClicked);
	}

	if (UInventoryComponent* InventoryComponent = GetLocalInventoryComponent())
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
	}

	if (UCraftingComponent* CraftingComponent = GetCharacterCraftingComponent(GetLocalPlayerCharacter()))
	{
		CraftingComponent->OnRecipeUnlockedDelegate.RemoveAll(this);
	}
}

void UShopMenuContents::UpdateWidget()
{
	RefreshSellSlotList();
	RefreshRecipeSlotList();
}

bool UShopMenuContents::TrySellItem(FName ItemUID)
{
	UInventoryComponent* InventoryComponent = GetLocalInventoryComponent();
	if (::IsValid(InventoryComponent) == false)
	{
		return false;
	}

	const bool bSold = InventoryComponent->TrySellItem(ItemUID, InventoryComponent->GetItemCount(ItemUID));
	ApplySaleResult(bSold);
	return bSold;
}

void UShopMenuContents::ToggleSellSelection(FName ItemUID)
{
	if (SelectedItemUIDs.Contains(ItemUID))
	{
		SelectedItemUIDs.Remove(ItemUID);
	}
	else
	{
		SelectedItemUIDs.Add(ItemUID);
	}

	RefreshSellSlotList();
}

bool UShopMenuContents::IsItemSelectedForSale(FName ItemUID) const
{
	return SelectedItemUIDs.Contains(ItemUID);
}

bool UShopMenuContents::TryUnlockRecipe(FName RecipeKey)
{
	USoundManager::TryPlaySFX(this, ESFXType::UI_Click);
	UCraftingComponent* CraftingComponent = GetCharacterCraftingComponent(GetLocalPlayerCharacter());
	return ::IsValid(CraftingComponent) && CraftingComponent->TryUnlockRecipe(RecipeKey);
}

void UShopMenuContents::BuildDefaultWidgetTree()
{
	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RuntimeShopRoot"));
	RootBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
	RootBorder->SetPadding(FMargin(FMKUILayout::SpaceXL));
	WidgetTree->RootWidget = RootBorder;

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	RootBorder->SetContent(RootBox);

	RootBox->AddChildToVerticalBox(BuildSectionHeader(NSLOCTEXT("ShopMenu", "Title", "광산 보급소"), TEXT("ShopMarker")))->SetPadding(FMargin(FMKUILayout::SpaceS, 0.f, FMKUILayout::SpaceS, FMKUILayout::SpaceL));
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	RootBox->AddChildToVerticalBox(Columns)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UVerticalBox* SellColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBoxSlot* SellColumnSlot = Columns->AddChildToHorizontalBox(SellColumn);
	SellColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SellColumnSlot->SetPadding(FMargin(0.f, 0.f, FMKUILayout::SpaceL, 0.f));
	UBorder* PlayerCard = WidgetTree->ConstructWidget<UBorder>();
	PlayerCard->SetBrushColor(FLinearColor(0.055f, 0.075f, 0.1f, 1.f));
	PlayerCard->SetPadding(FMargin(FMKUILayout::SpaceL));
	PlayerCard->SetContent(BuildSectionLabel(NSLOCTEXT("ShopMenu", "Player", "플레이어")));
	SellColumn->AddChildToVerticalBox(PlayerCard)->SetPadding(FMargin(0.f, 0.f, 0.f, FMKUILayout::SpaceM));
	UTextBlock* SellTitle = BuildSectionLabel(NSLOCTEXT("ShopMenu", "SellTitle", "보유 광물"));
	SellColumn->AddChildToVerticalBox(SellTitle)->SetPadding(FMargin(FMKUILayout::SpaceS, 0.f, 0.f, FMKUILayout::SpaceXS));

	UScrollBox* SellSlotScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SellSlotScrollBox"));
	if (UVerticalBoxSlot* ScrollSlot = SellColumn->AddChildToVerticalBox(SellSlotScrollBox))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	SellSlotListPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SellSlotListPanel"));
	SellSlotScrollBox->AddChild(SellSlotListPanel);

	UHorizontalBox* BottomBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomBox"));
	if (UVerticalBoxSlot* BottomSlot = SellColumn->AddChildToVerticalBox(BottomBox))
	{
		BottomSlot->SetPadding(FMargin(0.f, FMKUILayout::SpaceL, 0.f, 0.f));
	}

	UTextBlock* RawTotalPriceText = nullptr;
	UHorizontalBox* TotalPriceRow = BuildCurrencyAmount(RawTotalPriceText, FText::AsNumber(0));
	TotalPriceText = RawTotalPriceText;
	if (UHorizontalBoxSlot* TotalSlot = BottomBox->AddChildToHorizontalBox(TotalPriceRow))
	{
		TotalSlot->SetVerticalAlignment(VAlign_Center);
		TotalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UButton* RawSellAllButton = nullptr;
	UTextBlock* SellAllButtonText = nullptr;
	USizeBox* SellAllButtonSize = BuildActionButton(RawSellAllButton, SellAllButtonText, NSLOCTEXT("ShopMenu", "SellSelected", "선택 판매"));
	SellAllButton = RawSellAllButton;
	BottomBox->AddChildToHorizontalBox(SellAllButtonSize);

	UVerticalBox* RecipeColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBoxSlot* RecipeColumnSlot = Columns->AddChildToHorizontalBox(RecipeColumn);
	RecipeColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RecipeColumnSlot->SetPadding(FMargin(0.f, 0.f, FMKUILayout::SpaceL, 0.f));
	Columns->ShiftChild(0, RecipeColumn);
	SellColumnSlot->SetPadding(FMargin(0.f));
	UBorder* MerchantCard = WidgetTree->ConstructWidget<UBorder>();
	MerchantCard->SetBrushColor(FLinearColor(0.055f, 0.075f, 0.1f, 1.f));
	MerchantCard->SetPadding(FMargin(FMKUILayout::SpaceL));
	MerchantCard->SetContent(BuildSectionLabel(NSLOCTEXT("ShopMenu", "Merchant", "상점 주인")));
	RecipeColumn->AddChildToVerticalBox(MerchantCard)->SetPadding(FMargin(0.f, 0.f, 0.f, FMKUILayout::SpaceM));
	UTextBlock* RecipeTitleText = BuildSectionLabel(NSLOCTEXT("ShopMenu", "RecipeTitle", "설계도"));
	if (UVerticalBoxSlot* RecipeTitleSlot = RecipeColumn->AddChildToVerticalBox(RecipeTitleText))
	{
		RecipeTitleSlot->SetPadding(FMargin(FMKUILayout::SpaceS, 0.f, 0.f, FMKUILayout::SpaceXS));
	}

	UScrollBox* RecipeSlotScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RecipeSlotScrollBox"));
	if (UVerticalBoxSlot* RecipeScrollSlot = RecipeColumn->AddChildToVerticalBox(RecipeSlotScrollBox))
	{
		RecipeScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	RecipeSlotListPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RecipeSlotListPanel"));
	RecipeSlotScrollBox->AddChild(RecipeSlotListPanel);
}

void UShopMenuContents::RefreshSellSlotList()
{
	UInventoryComponent* InventoryComponent = GetLocalInventoryComponent();
	if (::IsValid(SellSlotListPanel) == false || ::IsValid(InventoryComponent) == false)
	{
		return;
	}

	SellSlotListPanel->ClearChildren();

	TArray<FName> ItemUIDs;
	const TMap<FName, int32> InventoryItems = InventoryComponent->GetInventoryItems();
	InventoryItems.GenerateKeyArray(ItemUIDs);
	ItemUIDs.Sort(FNameLexicalLess());
	for (auto SelectionIterator = SelectedItemUIDs.CreateIterator(); SelectionIterator; ++SelectionIterator)
	{
		if (InventoryItems.Contains(*SelectionIterator) == false)
		{
			SelectionIterator.RemoveCurrent();
		}
	}

	int32 SellableItemCount = 0;
	int32 TotalPrice = 0;
	for (const FName& ItemUID : ItemUIDs)
	{
		const int32 ItemCount = InventoryItems[ItemUID];
		const int32 UnitPrice = InventoryComponent->CalculateSellPrice(ItemUID, 1);
		if (UnitPrice <= 0 || ItemCount <= 0)
		{
			continue;
		}

		UShopSellSlotWidget* SellSlotWidget = CreateWidget<UShopSellSlotWidget>(this, SellSlotWidgetClass);
		if (::IsValid(SellSlotWidget) == false)
		{
			continue;
		}

		if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(SellSlotListPanel->AddChild(SellSlotWidget)))
		{
			RowSlot->SetPadding(FMargin(FMKUILayout::SpaceS, 0.f, FMKUILayout::SpaceS, FMKUILayout::SpaceS));
		}
		SellSlotWidget->InitializeSellSlot(this, ItemUID, ItemCount, UnitPrice);
		++SellableItemCount;
		if (SelectedItemUIDs.Contains(ItemUID))
		{
			TotalPrice += UnitPrice * ItemCount;
		}
	}

	if (::IsValid(TotalPriceText))
	{
		TotalPriceText->SetText(FText::AsNumber(TotalPrice));
	}
	if (SellableItemCount <= 0)
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>();
		EmptyText->SetText(NSLOCTEXT("ShopMenu", "NoSellableItem", "판매할 광물이 없습니다."));
		ApplyTextRole(EmptyText, EMKTextRole::Body);
		SellSlotListPanel->AddChild(EmptyText);
	}

	if (::IsValid(SellAllButton))
	{
		SellAllButton->SetIsEnabled(TotalPrice > 0);
	}
}

void UShopMenuContents::RefreshRecipeSlotList()
{
	UCraftingComponent* CraftingComponent = GetCharacterCraftingComponent(GetLocalPlayerCharacter());
	if (::IsValid(RecipeSlotListPanel) == false || ::IsValid(CraftingComponent) == false)
	{
		return;
	}

	RecipeSlotListPanel->ClearChildren();

	const TArray<FName> LockedRecipeKeys = CraftingComponent->GetLockedRecipeKeys();
	for (const FName& RecipeKey : LockedRecipeKeys)
	{
		UShopRecipeSlotWidget* RecipeSlotWidget = CreateWidget<UShopRecipeSlotWidget>(this, RecipeSlotWidgetClass);
		if (::IsValid(RecipeSlotWidget) == false)
		{
			continue;
		}

		if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(RecipeSlotListPanel->AddChild(RecipeSlotWidget)))
		{
			RowSlot->SetPadding(FMargin(FMKUILayout::SpaceS, 0.f, FMKUILayout::SpaceS, FMKUILayout::SpaceS));
		}
		RecipeSlotWidget->InitializeRecipeSlot(this, RecipeKey, CraftingComponent->CanUnlockRecipe(RecipeKey));
	}

	if (LockedRecipeKeys.IsEmpty())
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		EmptyText->SetText(NSLOCTEXT("ShopMenu", "NoLockedRecipe", "해금할 설계도가 없습니다."));
		ApplyTextRole(EmptyText, EMKTextRole::Body);
		RecipeSlotListPanel->AddChild(EmptyText);
	}
}

void UShopMenuContents::ApplySaleResult(bool bSold)
{
	USoundManager::TryPlaySFX(this, ESFXType::UI_Click);
	if (bSold == false)
	{
		return;
	}

	USoundManager::TryPlaySFX(this, ESFXType::Shop_Sell);
	RefreshRecipeSlotList();
	if (AMKCharacter* LocalCharacter = GetLocalPlayerCharacter())
	{
		LocalCharacter->TrySaveGame();
	}
}

void UShopMenuContents::OnSellAllButtonClicked()
{
	UInventoryComponent* InventoryComponent = GetLocalInventoryComponent();
	if (::IsValid(InventoryComponent) == false || SelectedItemUIDs.IsEmpty())
	{
		return;
	}

	bool bSoldAnyItem = false;
	const TArray<FName> SelectedItems = SelectedItemUIDs.Array();
	for (const FName ItemUID : SelectedItems)
	{
		bSoldAnyItem |= InventoryComponent->TrySellItem(ItemUID, InventoryComponent->GetItemCount(ItemUID));
	}
	SelectedItemUIDs.Reset();
	ApplySaleResult(bSoldAnyItem);
	RefreshSellSlotList();
}

void UShopMenuContents::OnInventoryChanged()
{
	RefreshSellSlotList();
}

void UShopMenuContents::OnCoinChanged(const FOnAttributeChangeData& Data)
{
	RefreshRecipeSlotList();
}

void UShopMenuContents::OnRecipeUnlocked(FName RecipeKey)
{
	RefreshRecipeSlotList();
}
