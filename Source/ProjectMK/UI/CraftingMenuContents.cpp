#include "ProjectMK/UI/CraftingMenuContents.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/CraftingComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/UI/CraftingRecipeSlotWidget.h"

UCraftingMenuContents::UCraftingMenuContents(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MenuContentsType = EMenuContentsType::Crafting;
}

TSharedRef<SWidget> UCraftingMenuContents::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
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
	if (::IsValid(StatusText))
	{
		StatusText->SetText(bCrafted
			? NSLOCTEXT("CraftingMenu", "Crafted", "제작 완료 · 숫자 키 또는 퀵슬롯 클릭으로 사용하세요.")
			: NSLOCTEXT("CraftingMenu", "CraftFailed", "제작 실패 · 레시피 해금, 재료와 인벤토리 공간을 확인하세요."));
	}
	return bCrafted;
}

void UCraftingMenuContents::BuildDefaultWidgetTree()
{
	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CraftingRoot"));
	RootBorder->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.065f, 0.97f));
	RootBorder->SetPadding(FMargin(20.f));
	WidgetTree->RootWidget = RootBorder;
	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>();
	RootBorder->SetContent(RootBox);
	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	TitleText->SetText(NSLOCTEXT("CraftingMenu", "Title", "제작 / C"));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 24;
	TitleText->SetFont(TitleFont);
	RootBox->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(NSLOCTEXT("CraftingMenu", "Hint", "레시피는 상점에서 해금합니다. 재료의 보유 / 필요 수량을 확인하세요."));
	StatusText->SetAutoWrapText(true);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.8f, 0.9f)));
	RootBox->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	UScrollBox* RecipeScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	RootBox->AddChildToVerticalBox(RecipeScrollBox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RecipeSlotListPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RecipeSlotListPanel"));
	RecipeScrollBox->AddChild(RecipeSlotListPanel);
}

void UCraftingMenuContents::UpdateRecipeSlots()
{
	const UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(RecipeSlotListPanel) == false || ::IsValid(DataManager) == false)
	{
		return;
	}
	if (RecipeSlots.IsEmpty())
	{
		TArray<FName> RecipeKeys = DataManager->GetShopRecipeRowNames();
		RecipeKeys.Sort(FNameLexicalLess());
		for (const FName& RecipeKey : RecipeKeys)
		{
			UCraftingRecipeSlotWidget* RecipeSlot = CreateWidget<UCraftingRecipeSlotWidget>(this, UCraftingRecipeSlotWidget::StaticClass());
			if (::IsValid(RecipeSlot))
			{
				RecipeSlotListPanel->AddChild(RecipeSlot);
				RecipeSlot->InitializeRecipeSlot(this, RecipeKey);
				RecipeSlots.Add(RecipeSlot);
			}
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
