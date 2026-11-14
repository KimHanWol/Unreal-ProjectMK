#include "ProjectMK/UI/CraftingRecipeSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "ProjectMK/UI/RecipeInfoWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "PaperSprite.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/CraftingComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/UI/CraftingMenuContents.h"

TSharedRef<SWidget> UCraftingRecipeSlotWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultWidgetTree();
	}
	return Super::RebuildWidget();
}

void UCraftingRecipeSlotWidget::BindEvents()
{
	if (::IsValid(CraftButton))
	{
		CraftButton->OnClicked.AddUniqueDynamic(this, &UCraftingRecipeSlotWidget::OnCraftButtonClicked);
	}
}

void UCraftingRecipeSlotWidget::UnbindEvents()
{
	if (::IsValid(CraftButton))
	{
		CraftButton->OnClicked.RemoveDynamic(this, &UCraftingRecipeSlotWidget::OnCraftButtonClicked);
	}
}

void UCraftingRecipeSlotWidget::UpdateWidget()
{
	if (URecipeInfoWidget* Info = Cast<URecipeInfoWidget>(WidgetTree->FindWidget(TEXT("RecipeInfo")))) { Info->SetRecipe(RecipeKey, false); }
	const UDataManager* DataManager = UDataManager::Get(this);
	const FShopRecipeDataTableRow* Recipe = ::IsValid(DataManager) && RecipeKey.IsNone() == false
		? DataManager->GetShopRecipeDataTableRow(RecipeKey) : nullptr;
	const AMKCharacter* Character = GetLocalPlayerCharacter();
	const UCraftingComponent* Crafting = ::IsValid(Character) ? Character->GetCraftingComponent() : nullptr;
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	if (Recipe == nullptr || ::IsValid(Crafting) == false || ::IsValid(Inventory) == false)
	{
		return;
	}

	const bool bUnlocked = Crafting->IsRecipeUnlocked(RecipeKey);
	RecipeText->SetText(DataManager->GetRecipeDisplayName(RecipeKey));
	SummaryText->SetText(Recipe->ShortDescription);
	bool bHasMaterials = true;
	for (const FShopRecipeItem& RequiredItem : Recipe->RequiredItems)
	{
		const int32 Owned = Inventory->GetItemCount(RequiredItem.GetItemKey());
		bHasMaterials &= Owned >= RequiredItem.ItemCount;
	}

	const bool bCanCraft = Crafting->CanCraftRecipe(RecipeKey);
	CraftButton->SetIsEnabled(bCanCraft);
	CraftButtonText->SetText(bUnlocked
		? (bCanCraft ? NSLOCTEXT("CraftingMenu", "Craft", "제작") : (bHasMaterials ? NSLOCTEXT("CraftingMenu", "NoSpace", "공간 부족") : NSLOCTEXT("CraftingMenu", "NoMaterials", "재료 부족")))
		: NSLOCTEXT("CraftingMenu", "Locked", "해금 필요"));
	const FItemDataTableRow* Item = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, Recipe->GetResultItemKey());
	UPaperSprite* Sprite = Item != nullptr ? Item->ItemIcon.LoadSynchronous() : nullptr;
	ItemIcon->SetBrushResourceObject(Sprite);
	ItemIcon->SetVisibility(::IsValid(Sprite) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void UCraftingRecipeSlotWidget::InitializeRecipeSlot(UCraftingMenuContents* InOwnerMenu, FName InRecipeKey)
{
	OwnerMenu = InOwnerMenu;
	RecipeKey = InRecipeKey;
	UpdateWidget();
}

void UCraftingRecipeSlotWidget::RefreshRecipe()
{
	UpdateWidget();
}

void UCraftingRecipeSlotWidget::BuildDefaultWidgetTree()
{
	UImage* Icon = nullptr;
	UTextBlock* Name = nullptr;
	UTextBlock* Summary = nullptr;
	UButton* Button = nullptr;
	UTextBlock* Label = nullptr;
	WidgetTree->RootWidget = BuildRecipeEntry(Icon, Name, Summary, Button, Label, false);
	ItemIcon = Icon;
	RecipeText = Name;
	SummaryText = Summary;
	CraftButton = Button;
	CraftButtonText = Label;
}

void UCraftingRecipeSlotWidget::OnCraftButtonClicked()
{
	if (OwnerMenu.IsValid())
	{
		OwnerMenu->TryCraftRecipe(RecipeKey);
	}
}
