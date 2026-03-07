#include "ProjectMK/UI/ShopRecipeEntryWidget.h"

#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Data/DataTable/ShopRecipeDataTableRow.h"
#include "ProjectMK/UI/ItemSlotWidget.h"

void UShopRecipeEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (::IsValid(Button_Craft))
    {
        Button_Craft->OnClicked.AddDynamic(this, &UShopRecipeEntryWidget::HandleCraftButtonClicked);
    }
}

void UShopRecipeEntryWidget::InitializeRecipe(FName InRecipeId, const FShopRecipeDataTableRow& InRecipeData)
{
    RecipeId = InRecipeId;
    RecipeData = InRecipeData;

    if (::IsValid(Text_RecipeName))
    {
        Text_RecipeName->SetText(RecipeData.DisplayName.IsEmpty() ? FText::FromName(RecipeId) : RecipeData.DisplayName);
    }

    if (::IsValid(Text_ResultKey))
    {
        Text_ResultKey->SetText(FText::FromName(RecipeData.GetResultItemKey()));
    }

    if (::IsValid(HBox_RecipeItems))
    {
        HBox_RecipeItems->ClearChildren();

        for (const FShopRecipeItem& RequiredItem : RecipeData.RequiredItems)
        {
            UItemSlotWidget* ItemSlotWidget = CreateWidget<UItemSlotWidget>(this, ItemSlotClass);
            if (::IsValid(ItemSlotWidget) == false)
            {
                continue;
            }

            HBox_RecipeItems->AddChildToHorizontalBox(ItemSlotWidget);
            ItemSlotWidget->SetItem(RequiredItem.GetItemKey(), RequiredItem.ItemCount);
        }
    }

    if (::IsValid(OutputItemSlot))
    {
        OutputItemSlot->SetItem(RecipeData.GetResultItemKey(), 1);
    }

    RefreshState();
}

void UShopRecipeEntryWidget::RefreshState()
{
    if (::IsValid(Button_Craft) == false)
    {
        return;
    }

    AMKCharacter* PlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (::IsValid(PlayerCharacter) == false)
    {
        Button_Craft->SetIsEnabled(false);
        return;
    }

    UInventoryComponent* InventoryComponent = PlayerCharacter->GetComponentByClass<UInventoryComponent>();
    if (::IsValid(InventoryComponent) == false)
    {
        Button_Craft->SetIsEnabled(false);
        return;
    }

    Button_Craft->SetIsEnabled(InventoryComponent->CanCraftShopRecipe(RecipeData));
}

void UShopRecipeEntryWidget::HandleCraftButtonClicked()
{
    AMKCharacter* PlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (::IsValid(PlayerCharacter) == false)
    {
        return;
    }

    UInventoryComponent* InventoryComponent = PlayerCharacter->GetComponentByClass<UInventoryComponent>();
    if (::IsValid(InventoryComponent) == false)
    {
        return;
    }

    InventoryComponent->CraftShopRecipe(RecipeData);
    RefreshState();
}
