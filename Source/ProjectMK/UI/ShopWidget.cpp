#include "ProjectMK/UI/ShopWidget.h"

#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ShopRecipeDataTableRow.h"
#include "ProjectMK/UI/ShopRecipeEntryWidget.h"

void UShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindInventory();
	BuildRecipeList();
}

void UShopWidget::NativeDestruct()
{
	if (::IsValid(BoundInventoryComponent))
	{
		BoundInventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UShopWidget::BuildRecipeList()
{
	RecipeEntries.Reset();

	if (::IsValid(VBox_RecipeList) == false)
	{
		return;
	}

	VBox_RecipeList->ClearChildren();

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	UDataTable* ShopRecipeDataTable = DataManager->GetDataTable(EDataTableType::ShopRecipe);
	if (ShopRecipeDataTable == nullptr)
	{
		return;
	}

	TArray<FName> RowNames = ShopRecipeDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		const FShopRecipeDataTableRow* ShopRecipeRow = ShopRecipeDataTable->FindRow<FShopRecipeDataTableRow>(RowName, TEXT("BuildRecipeList"));
		if (ShopRecipeRow == nullptr)
		{
			continue;
		}

		UShopRecipeEntryWidget* EntryWidget = CreateWidget<UShopRecipeEntryWidget>(this, ShopRecipeEntryClass);
		if (::IsValid(EntryWidget) == false)
		{
			continue;
		}

		VBox_RecipeList->AddChildToVerticalBox(EntryWidget);
		EntryWidget->InitializeRecipe(RowName, *ShopRecipeRow);
		RecipeEntries.Add(EntryWidget);
	}

	RefreshEntries();
}

void UShopWidget::BindInventory()
{
	if (::IsValid(BoundInventoryComponent))
	{
		BoundInventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
		BoundInventoryComponent = nullptr;
	}

	AMKCharacter* PlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (::IsValid(PlayerCharacter) == false)
	{
		return;
	}

	BoundInventoryComponent = PlayerCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(BoundInventoryComponent))
	{
		BoundInventoryComponent->OnInventoryChangedDelegate.AddUObject(this, &UShopWidget::HandleInventoryChanged);
	}
}

void UShopWidget::RefreshEntries()
{
	for (UShopRecipeEntryWidget* EntryWidget : RecipeEntries)
	{
		if (::IsValid(EntryWidget))
		{
			EntryWidget->RefreshState();
		}
	}
}

void UShopWidget::HandleInventoryChanged()
{
	RefreshEntries();
}
