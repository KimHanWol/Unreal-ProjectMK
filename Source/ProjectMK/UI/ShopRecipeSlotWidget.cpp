#include "ProjectMK/UI/ShopRecipeSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "PaperSprite.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/ShopRecipeDataTableRow.h"
#include "ProjectMK/UI/ShopMenuContents.h"
#include "ProjectMK/UI/MKMockupButton.h"

TSharedRef<SWidget> UShopRecipeSlotWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

void UShopRecipeSlotWidget::BindEvents()
{
	if (::IsValid(UnlockButton))
	{
		UnlockButton->OnClicked.AddUniqueDynamic(this, &UShopRecipeSlotWidget::OnUnlockButtonClicked);
	}
}

void UShopRecipeSlotWidget::UnbindEvents()
{
	if (::IsValid(UnlockButton))
	{
		UnlockButton->OnClicked.RemoveDynamic(this, &UShopRecipeSlotWidget::OnUnlockButtonClicked);
	}
}

void UShopRecipeSlotWidget::UpdateWidget()
{
	// NativeConstruct 에서 InitializeRecipeSlot 이전에 먼저 호출된다.
	const UDataManager* DataManager = UDataManager::Get(this);
	const FShopRecipeDataTableRow* RecipeDataTableRow = RecipeKey.IsNone() == false && ::IsValid(DataManager) ? DataManager->GetShopRecipeDataTableRow(RecipeKey) : nullptr;
	if (RecipeDataTableRow == nullptr)
	{
		return;
	}

	if (::IsValid(RecipeNameText))
	{
		const FText RecipeName = RecipeDataTableRow->DisplayName.IsEmpty() ? FText::FromName(RecipeDataTableRow->GetResultItemKey()) : RecipeDataTableRow->DisplayName;
		RecipeNameText->SetText(RecipeDataTableRow->ResultItemCount > 1
			? FText::Format(NSLOCTEXT("ShopMenu", "RecipeNameCountFormat", "{0} x{1}"), RecipeName, RecipeDataTableRow->ResultItemCount)
			: RecipeName);
	}

	if (::IsValid(RequiredItemsText))
	{
		TArray<FString> RequiredItemStrings;
		for (const FShopRecipeItem& RequiredItem : RecipeDataTableRow->RequiredItems)
		{
			RequiredItemStrings.Add(FString::Printf(TEXT("%s x%d"), *RequiredItem.GetItemKey().ToString(), RequiredItem.ItemCount));
		}

		RequiredItemsText->SetText(FText::Format(NSLOCTEXT("ShopMenu", "RequiredItemsFormat", "재료: {0}"), FText::FromString(FString::Join(RequiredItemStrings, TEXT(", ")))));
	}

	if (::IsValid(UnlockPriceText))
	{
		UnlockPriceText->SetText(FText::Format(NSLOCTEXT("ShopMenu", "UnlockPriceFormat", "{0} 코인"), RecipeDataTableRow->UnlockPrice));
	}

	if (::IsValid(UnlockButton))
	{
		UnlockButton->SetIsEnabled(bCanUnlock);
	}

	if (::IsValid(ResultItemIconImage) == false)
	{
		return;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, RecipeDataTableRow->GetResultItemKey());
	UPaperSprite* ItemIconSprite = ItemDataTableRow != nullptr ? ItemDataTableRow->ItemIcon.LoadSynchronous() : nullptr;
	if (::IsValid(ItemIconSprite) == false)
	{
		ResultItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	ResultItemIconImage->SetBrushResourceObject(ItemIconSprite);
	ResultItemIconImage->SetDesiredSizeOverride(ItemIconSize);
	ResultItemIconImage->SetVisibility(ESlateVisibility::Visible);
}

void UShopRecipeSlotWidget::InitializeRecipeSlot(UShopMenuContents* InOwnerShopMenu, FName InRecipeKey, bool bInCanUnlock)
{
	OwnerShopMenu = InOwnerShopMenu;
	RecipeKey = InRecipeKey;
	bCanUnlock = bInCanUnlock;
	UpdateWidget();
}

void UShopRecipeSlotWidget::BuildDefaultWidgetTree()
{
	UHorizontalBox* RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RootBox"));
	WidgetTree->RootWidget = RootBox;

	ResultItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ResultItemIconImage"));
	if (UHorizontalBoxSlot* IconSlot = RootBox->AddChildToHorizontalBox(ResultItemIconImage))
	{
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}

	UVerticalBox* TextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TextBox"));
	if (UHorizontalBoxSlot* TextSlot = RootBox->AddChildToHorizontalBox(TextBox))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	RecipeNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RecipeNameText"));
	TextBox->AddChildToVerticalBox(RecipeNameText);

	RequiredItemsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RequiredItemsText"));
	TextBox->AddChildToVerticalBox(RequiredItemsText);

	UnlockPriceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UnlockPriceText"));
	if (UHorizontalBoxSlot* PriceSlot = RootBox->AddChildToHorizontalBox(UnlockPriceText))
	{
		PriceSlot->SetVerticalAlignment(VAlign_Center);
		PriceSlot->SetPadding(FMargin(8.f, 0.f));
	}

	UnlockButton = WidgetTree->ConstructWidget<UMKMockupButton>(UMKMockupButton::StaticClass(), TEXT("UnlockButton"));
	UTextBlock* UnlockButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UnlockButtonText"));
	UnlockButtonText->SetText(NSLOCTEXT("ShopMenu", "UnlockRecipe", "레시피 구매"));
	UnlockButton->AddChild(UnlockButtonText);
	if (UHorizontalBoxSlot* ButtonSlot = RootBox->AddChildToHorizontalBox(UnlockButton))
	{
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UShopRecipeSlotWidget::OnUnlockButtonClicked()
{
	if (OwnerShopMenu.IsValid())
	{
		OwnerShopMenu->TryUnlockRecipe(RecipeKey);
	}
}
