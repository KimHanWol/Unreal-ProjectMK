#include "ProjectMK/UI/ShopRecipeSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "ProjectMK/UI/RecipeInfoWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "PaperSprite.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/ShopRecipeDataTableRow.h"
#include "ProjectMK/UI/ShopMenuContents.h"

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
	if (URecipeInfoWidget* Info = Cast<URecipeInfoWidget>(WidgetTree->FindWidget(TEXT("RecipeInfo")))) { Info->SetRecipe(RecipeKey, true); }
	// NativeConstruct 에서 InitializeRecipeSlot 이전에 먼저 호출된다.
	const UDataManager* DataManager = UDataManager::Get(this);
	const FShopRecipeDataTableRow* RecipeDataTableRow = RecipeKey.IsNone() == false && ::IsValid(DataManager) ? DataManager->GetShopRecipeDataTableRow(RecipeKey) : nullptr;
	if (RecipeDataTableRow == nullptr)
	{
		return;
	}

	if (::IsValid(RecipeNameText))
	{
		RecipeNameText->SetText(DataManager->GetRecipeDisplayName(RecipeKey, true));
	}
	if (::IsValid(SummaryText)) { SummaryText->SetText(RecipeDataTableRow->ShortDescription); }

	if (::IsValid(UnlockPriceText))
	{
		UnlockPriceText->SetText(FText::AsNumber(RecipeDataTableRow->UnlockPrice));
	}

	if (::IsValid(UnlockButton))
	{
		UnlockButton->SetIsEnabled(bCanUnlock);
	}

	if (::IsValid(ResultItemIconImage) == false)
	{
		return;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetRecipeItemData(RecipeKey, true);
	UPaperSprite* ItemIconSprite = ItemDataTableRow != nullptr ? ItemDataTableRow->ItemIcon.LoadSynchronous() : nullptr;
	if (::IsValid(ItemIconSprite) == false)
	{
		ResultItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	ResultItemIconImage->SetBrushResourceObject(ItemIconSprite);
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
	UImage* Icon = nullptr;
	UTextBlock* Name = nullptr;
	UTextBlock* Summary = nullptr;
	UButton* Button = nullptr;
	UTextBlock* Price = nullptr;
	WidgetTree->RootWidget = BuildRecipeEntry(Icon, Name, Summary, Button, Price, true);
	ResultItemIconImage = Icon;
	RecipeNameText = Name;
	SummaryText = Summary;
	UnlockButton = Button;
	UnlockPriceText = Price;
}

void UShopRecipeSlotWidget::OnUnlockButtonClicked()
{
	if (OwnerShopMenu.IsValid())
	{
		OwnerShopMenu->TryUnlockRecipe(RecipeKey);
	}
}
