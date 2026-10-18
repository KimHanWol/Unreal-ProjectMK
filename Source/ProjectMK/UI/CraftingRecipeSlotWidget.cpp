#include "ProjectMK/UI/CraftingRecipeSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "PaperSprite.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/CraftingComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/UI/CraftingMenuContents.h"
#include "ProjectMK/UI/MKMockupButton.h"

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
	const FText DisplayName = Recipe->DisplayName.IsEmpty() ? FText::FromName(Recipe->GetResultItemKey()) : Recipe->DisplayName;
	RecipeText->SetText(FText::Format(NSLOCTEXT("CraftingMenu", "Result", "{0} ×{1}"), DisplayName, Recipe->ResultItemCount));
	TArray<FString> Materials;
	bool bHasMaterials = true;
	for (const FShopRecipeItem& RequiredItem : Recipe->RequiredItems)
	{
		const int32 Owned = Inventory->GetItemCount(RequiredItem.GetItemKey());
		bHasMaterials &= Owned >= RequiredItem.ItemCount;
		Materials.Add(FString::Printf(TEXT("%s %d/%d"), *RequiredItem.GetItemKey().ToString(), Owned, RequiredItem.ItemCount));
	}
	RequiredItemsText->SetText(FText::FromString(FString::Join(Materials, TEXT(" · "))));
	RequiredItemsText->SetColorAndOpacity(FSlateColor(bHasMaterials ? FLinearColor(0.55f, 0.85f, 0.65f) : FLinearColor(1.f, 0.65f, 0.4f)));

	const bool bCanCraft = Crafting->CanCraftRecipe(RecipeKey);
	CraftButton->SetIsEnabled(bCanCraft);
	CraftButtonText->SetText(bUnlocked
		? (bCanCraft ? NSLOCTEXT("CraftingMenu", "Craft", "제작") : NSLOCTEXT("CraftingMenu", "Unavailable", "재료 / 공간 부족"))
		: FText::Format(NSLOCTEXT("CraftingMenu", "Locked", "상점 해금\n{0} 코인"), Recipe->UnlockPrice));
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
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
	Border->SetBrushColor(FLinearColor(0.06f, 0.09f, 0.13f, 1.f));
	Border->SetPadding(FMargin(10.f, 8.f));
	WidgetTree->RootWidget = Border;
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Border->SetContent(Row);
	ItemIcon = WidgetTree->ConstructWidget<UImage>();
	ItemIcon->SetDesiredSizeOverride(FVector2D(32.f));
	UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(ItemIcon);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	IconSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	UVerticalBox* TextColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextColumn);
	TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TextSlot->SetVerticalAlignment(VAlign_Center);
	RecipeText = WidgetTree->ConstructWidget<UTextBlock>();
	TextColumn->AddChildToVerticalBox(RecipeText);
	RequiredItemsText = WidgetTree->ConstructWidget<UTextBlock>();
	RequiredItemsText->SetAutoWrapText(true);
	FSlateFontInfo MaterialFont = RequiredItemsText->GetFont();
	MaterialFont.Size = 12;
	RequiredItemsText->SetFont(MaterialFont);
	TextColumn->AddChildToVerticalBox(RequiredItemsText);
	CraftButton = WidgetTree->ConstructWidget<UMKMockupButton>();
	CraftButtonText = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo ButtonFont = CraftButtonText->GetFont();
	ButtonFont.Size = 12;
	CraftButtonText->SetFont(ButtonFont);
	CraftButtonText->SetJustification(ETextJustify::Center);
	CraftButton->SetContent(CraftButtonText);
	UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(CraftButton);
	ButtonSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	ButtonSlot->SetVerticalAlignment(VAlign_Center);
}

void UCraftingRecipeSlotWidget::OnCraftButtonClicked()
{
	if (OwnerMenu.IsValid())
	{
		OwnerMenu->TryCraftRecipe(RecipeKey);
	}
}
