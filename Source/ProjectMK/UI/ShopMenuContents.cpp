#include "ProjectMK/UI/ShopMenuContents.h"

#include "Blueprint/WidgetTree.h"
#include "AbilitySystemComponent.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
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
#include "ProjectMK/UI/MKMockupButton.h"

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
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
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

bool UShopMenuContents::TryUnlockRecipe(FName RecipeKey)
{
	USoundManager::TryPlaySFX(this, ESFXType::UI_Click);
	UCraftingComponent* CraftingComponent = GetCharacterCraftingComponent(GetLocalPlayerCharacter());
	return ::IsValid(CraftingComponent) && CraftingComponent->TryUnlockRecipe(RecipeKey);
}

void UShopMenuContents::BuildDefaultWidgetTree()
{
	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
	RootBorder->SetPadding(FMargin(24.f));
	WidgetTree->RootWidget = RootBorder;

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	RootBorder->SetContent(RootBox);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(NSLOCTEXT("ShopMenu", "Title", "상점"));
	if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	}

	UScrollBox* SellSlotScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SellSlotScrollBox"));
	if (UVerticalBoxSlot* ScrollSlot = RootBox->AddChildToVerticalBox(SellSlotScrollBox))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	SellSlotListPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SellSlotListPanel"));
	SellSlotScrollBox->AddChild(SellSlotListPanel);

	UHorizontalBox* BottomBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomBox"));
	if (UVerticalBoxSlot* BottomSlot = RootBox->AddChildToVerticalBox(BottomBox))
	{
		BottomSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}

	TotalPriceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TotalPriceText"));
	if (UHorizontalBoxSlot* TotalSlot = BottomBox->AddChildToHorizontalBox(TotalPriceText))
	{
		TotalSlot->SetVerticalAlignment(VAlign_Center);
		TotalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	SellAllButton = WidgetTree->ConstructWidget<UMKMockupButton>(UMKMockupButton::StaticClass(), TEXT("SellAllButton"));
	UTextBlock* SellAllButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SellAllButtonText"));
	SellAllButtonText->SetText(NSLOCTEXT("ShopMenu", "SellAll", "전체 판매"));
	SellAllButton->AddChild(SellAllButtonText);
	BottomBox->AddChildToHorizontalBox(SellAllButton);

	UTextBlock* RecipeTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RecipeTitleText"));
	RecipeTitleText->SetText(NSLOCTEXT("ShopMenu", "RecipeTitle", "레시피"));
	if (UVerticalBoxSlot* RecipeTitleSlot = RootBox->AddChildToVerticalBox(RecipeTitleText))
	{
		RecipeTitleSlot->SetPadding(FMargin(0.f, 24.f, 0.f, 8.f));
	}

	UScrollBox* RecipeSlotScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RecipeSlotScrollBox"));
	if (UVerticalBoxSlot* RecipeScrollSlot = RootBox->AddChildToVerticalBox(RecipeSlotScrollBox))
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

		SellSlotListPanel->AddChild(SellSlotWidget);
		SellSlotWidget->InitializeSellSlot(this, ItemUID, ItemCount, UnitPrice);
		TotalPrice += UnitPrice * ItemCount;
	}

	if (::IsValid(TotalPriceText))
	{
		TotalPriceText->SetText(TotalPrice > 0
			? FText::Format(NSLOCTEXT("ShopMenu", "TotalPriceFormat", "합계 {0} 코인"), TotalPrice)
			: NSLOCTEXT("ShopMenu", "NoSellableItem", "판매할 아이템이 없습니다."));
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

		RecipeSlotListPanel->AddChild(RecipeSlotWidget);
		RecipeSlotWidget->InitializeRecipeSlot(this, RecipeKey, CraftingComponent->CanUnlockRecipe(RecipeKey));
	}

	if (LockedRecipeKeys.IsEmpty())
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		EmptyText->SetText(NSLOCTEXT("ShopMenu", "NoLockedRecipe", "구매할 레시피가 없습니다."));
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
	if (::IsValid(InventoryComponent) == false)
	{
		return;
	}

	ApplySaleResult(InventoryComponent->TrySellAllItems());
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
