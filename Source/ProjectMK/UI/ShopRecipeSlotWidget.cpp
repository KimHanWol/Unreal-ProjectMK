#include "ProjectMK/UI/ShopRecipeSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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

void UShopRecipeSlotWidget::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Super::NativeOnMouseEnter(Geometry, Event);
	if (UHUDWidget* HUD = GetHUDWidget()) { HUD->ShowRecipeTooltip(this, RecipeKey); }
}

void UShopRecipeSlotWidget::NativeOnMouseLeave(const FPointerEvent& Event)
{
	Super::NativeOnMouseLeave(Event);
	if (UHUDWidget* HUD = GetHUDWidget()) { HUD->ClearTooltipSource(this); }
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
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RecipeCard"));
	Card->SetPadding(FMargin(4.f, 8.f, 12.f, 8.f));
	Card->SetContent(RootBox);
	WidgetTree->RootWidget = Card;

	UImage* RawResultItemIcon = nullptr;
	UBorder* IconFrame = BuildItemIconFrame(RawResultItemIcon, 32.f, NAME_None);
	ResultItemIconImage = RawResultItemIcon;
	if (UHorizontalBoxSlot* IconSlot = RootBox->AddChildToHorizontalBox(IconFrame))
	{
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, FMKUILayout::SpaceM, 0.f));
	}

	RecipeNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RecipeNameText"));
	ApplyTextRole(RecipeNameText, EMKTextRole::Label, true);
	if (UHorizontalBoxSlot* TextSlot = RootBox->AddChildToHorizontalBox(RecipeNameText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UButton* RawUnlockButton = nullptr;
	UTextBlock* RawUnlockPriceText = nullptr;
	USizeBox* UnlockButtonSize = BuildCurrencyButton(RawUnlockButton, RawUnlockPriceText, FText::GetEmpty());
	UnlockButton = RawUnlockButton;
	UnlockPriceText = RawUnlockPriceText;
	if (UHorizontalBoxSlot* ButtonSlot = RootBox->AddChildToHorizontalBox(UnlockButtonSize))
	{
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
		ButtonSlot->SetPadding(FMargin(FMKUILayout::SpaceM, 0.f, 0.f, 0.f));
	}
}

void UShopRecipeSlotWidget::OnUnlockButtonClicked()
{
	if (OwnerShopMenu.IsValid())
	{
		OwnerShopMenu->TryUnlockRecipe(RecipeKey);
	}
}
