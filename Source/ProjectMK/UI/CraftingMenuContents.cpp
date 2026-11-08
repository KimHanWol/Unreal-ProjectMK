#include "ProjectMK/UI/CraftingMenuContents.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/CraftingComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/UI/CraftingRecipeSlotWidget.h"
#include "ProjectMK/UI/PlaytestQuickSlotWidget.h"

UCraftingMenuContents::UCraftingMenuContents(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MenuContentsType = EMenuContentsType::Crafting;
}

TSharedRef<SWidget> UCraftingMenuContents::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->FindWidget(TEXT("RuntimeCraftingRoot")) == nullptr)
	{
		BuildDefaultWidgetTree();
	}
	return Super::RebuildWidget();
}

void UCraftingMenuContents::BindEvents()
{
	OnVisibilityChanged.AddUniqueDynamic(this, &UCraftingMenuContents::OnMenuVisibilityChanged);

	if (UInventoryComponent* InventoryComponent = GetLocalInventoryComponent())
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
		InventoryComponent->OnInventoryChangedDelegate.AddUObject(this, &UCraftingMenuContents::OnInventoryChanged);
	}

	if (UCraftingComponent* CraftingComponent = GetCraftingComponent())
	{
		CraftingComponent->OnRecipeUnlockedDelegate.RemoveAll(this);
		CraftingComponent->OnRecipeUnlockedDelegate.AddUObject(this, &UCraftingMenuContents::OnRecipeChanged);
		CraftingComponent->OnRecipeCraftedDelegate.RemoveAll(this);
		CraftingComponent->OnRecipeCraftedDelegate.AddUObject(this, &UCraftingMenuContents::OnRecipeChanged);
	}
}

void UCraftingMenuContents::UnbindEvents()
{
	OnVisibilityChanged.RemoveDynamic(this, &UCraftingMenuContents::OnMenuVisibilityChanged);

	if (UInventoryComponent* InventoryComponent = GetLocalInventoryComponent())
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
	}

	if (UCraftingComponent* CraftingComponent = GetCraftingComponent())
	{
		CraftingComponent->OnRecipeUnlockedDelegate.RemoveAll(this);
		CraftingComponent->OnRecipeCraftedDelegate.RemoveAll(this);
	}
}

void UCraftingMenuContents::UpdateWidget()
{
	UpdateRecipeSlots();
	for (UPlaytestQuickSlotWidget* InventorySlot : InventorySlots)
	{
		if (::IsValid(InventorySlot))
		{
			InventorySlot->RefreshSlot();
		}
	}
	BP_OnCraftingRefreshed();
}

TArray<FName> UCraftingMenuContents::GetUnlockedRecipeKeys() const
{
	const UCraftingComponent* CraftingComponent = GetCraftingComponent();
	return ::IsValid(CraftingComponent) ? CraftingComponent->GetUnlockedRecipeKeys() : TArray<FName>();
}

bool UCraftingMenuContents::CanCraftRecipe(FName RecipeKey) const
{
	const UCraftingComponent* CraftingComponent = GetCraftingComponent();
	return ::IsValid(CraftingComponent) && CraftingComponent->CanCraftRecipe(RecipeKey);
}

bool UCraftingMenuContents::TryCraftRecipe(FName RecipeKey)
{
	UCraftingComponent* CraftingComponent = GetCraftingComponent();
	const bool bCrafted = ::IsValid(CraftingComponent) && CraftingComponent->TryCraftRecipe(RecipeKey);
	return bCrafted;
}

void UCraftingMenuContents::BuildDefaultWidgetTree()
{
	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RuntimeCraftingRoot"));
	RootBorder->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.065f, 0.97f));
	RootBorder->SetPadding(FMargin(FMKUILayout::SpaceXL));
	WidgetTree->RootWidget = RootBorder;
	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>();
	RootBorder->SetContent(RootBox);
	RootBox->AddChildToVerticalBox(BuildSectionHeader(NSLOCTEXT("CraftingMenu", "Title", "제작 작업대"), TEXT("Crafting")))->SetPadding(FMargin(FMKUILayout::SpaceS, 0.f, FMKUILayout::SpaceS, FMKUILayout::SpaceL));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetVisibility(ESlateVisibility::Collapsed);

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	RootBox->AddChildToVerticalBox(Columns)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UVerticalBox* RecipeColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBoxSlot* RecipeColumnSlot = Columns->AddChildToHorizontalBox(RecipeColumn);
	RecipeColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RecipeColumnSlot->SetPadding(FMargin(0.f, 0.f, FMKUILayout::SpaceL, 0.f));
	UBorder* WorkbenchCard = WidgetTree->ConstructWidget<UBorder>();
	WorkbenchCard->SetBrushColor(FLinearColor(0.055f, 0.075f, 0.1f, 1.f));
	WorkbenchCard->SetPadding(FMargin(FMKUILayout::SpaceL));
	WorkbenchCard->SetContent(BuildSectionLabel(NSLOCTEXT("CraftingMenu", "Workbench", "작업대")));
	RecipeColumn->AddChildToVerticalBox(WorkbenchCard)->SetPadding(FMargin(0.f, 0.f, 0.f, FMKUILayout::SpaceM));
	UScrollBox* RecipeScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	RecipeColumn->AddChildToVerticalBox(RecipeScrollBox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RecipeSlotListPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RecipeSlotListPanel"));
	RecipeScrollBox->AddChild(RecipeSlotListPanel);

	UVerticalBox* InventoryColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	Columns->AddChildToHorizontalBox(InventoryColumn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UBorder* PlayerCard = WidgetTree->ConstructWidget<UBorder>();
	PlayerCard->SetBrushColor(FLinearColor(0.055f, 0.075f, 0.1f, 1.f));
	PlayerCard->SetPadding(FMargin(FMKUILayout::SpaceL));
	PlayerCard->SetContent(BuildSectionLabel(NSLOCTEXT("CraftingMenu", "Player", "플레이어")));
	InventoryColumn->AddChildToVerticalBox(PlayerCard)->SetPadding(FMargin(0.f, 0.f, 0.f, FMKUILayout::SpaceM));
	InventoryColumn->AddChildToVerticalBox(BuildSectionLabel(NSLOCTEXT("CraftingMenu", "Inventory", "인벤토리")))
		->SetPadding(FMargin(FMKUILayout::SpaceS, 0.f, 0.f, FMKUILayout::SpaceS));
	UUniformGridPanel* InventoryGrid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	InventoryGrid->SetSlotPadding(FMargin(FMKUILayout::SpaceXS));
	InventoryColumn->AddChildToVerticalBox(InventoryGrid);
	for (int32 SlotIndex = 0; SlotIndex < 8; ++SlotIndex)
	{
		UPlaytestQuickSlotWidget* InventorySlot = CreateWidget<UPlaytestQuickSlotWidget>(this, UPlaytestQuickSlotWidget::StaticClass());
		if (::IsValid(InventorySlot))
		{
			InventoryGrid->AddChildToUniformGrid(InventorySlot, SlotIndex / 4, SlotIndex % 4);
			InventorySlot->SetShowKeyGuide(false);
			InventorySlot->SetSlotIndex(SlotIndex);
			InventorySlots.Add(InventorySlot);
		}
	}
}

void UCraftingMenuContents::UpdateRecipeSlots()
{
	const UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(RecipeSlotListPanel) == false || ::IsValid(DataManager) == false)
	{
		return;
	}
	RecipeSlotListPanel->ClearChildren();
	RecipeSlots.Reset();
	TArray<FName> RecipeKeys = DataManager->GetShopRecipeRowNames();
	RecipeKeys.Sort([this](const FName A, const FName B)
	{
		const bool bCanCraftA = CanCraftRecipe(A);
		const bool bCanCraftB = CanCraftRecipe(B);
		if (bCanCraftA != bCanCraftB)
		{
			return bCanCraftA;
		}
		const AMKCharacter* Character = GetLocalPlayerCharacter();
		const UCraftingComponent* Crafting = ::IsValid(Character) ? Character->GetCraftingComponent() : nullptr;
		const bool bUnlockedA = ::IsValid(Crafting) && Crafting->IsRecipeUnlocked(A);
		const bool bUnlockedB = ::IsValid(Crafting) && Crafting->IsRecipeUnlocked(B);
		return bUnlockedA != bUnlockedB ? bUnlockedA : A.LexicalLess(B);
	});
	for (const FName RecipeKey : RecipeKeys)
	{
		UCraftingRecipeSlotWidget* RecipeSlot = CreateWidget<UCraftingRecipeSlotWidget>(this, UCraftingRecipeSlotWidget::StaticClass());
		if (::IsValid(RecipeSlot))
		{
			if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(RecipeSlotListPanel->AddChild(RecipeSlot)))
			{
				RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, FMKUILayout::SpaceS));
			}
			RecipeSlot->InitializeRecipeSlot(this, RecipeKey);
			RecipeSlots.Add(RecipeSlot);
		}
	}
	for (UCraftingRecipeSlotWidget* RecipeSlot : RecipeSlots)
	{
		if (::IsValid(RecipeSlot))
		{
			RecipeSlot->RefreshRecipe();
		}
	}
}

UCraftingComponent* UCraftingMenuContents::GetCraftingComponent() const
{
	const AMKCharacter* LocalCharacter = GetLocalPlayerCharacter();
	return ::IsValid(LocalCharacter) ? LocalCharacter->GetCraftingComponent() : nullptr;
}

void UCraftingMenuContents::OnMenuVisibilityChanged(ESlateVisibility InVisibility)
{
	if (InVisibility != ESlateVisibility::Collapsed && InVisibility != ESlateVisibility::Hidden)
	{
		UpdateWidget();
	}
}

void UCraftingMenuContents::OnInventoryChanged()
{
	UpdateWidget();
}

void UCraftingMenuContents::OnRecipeChanged(FName RecipeKey)
{
	UpdateWidget();
}
