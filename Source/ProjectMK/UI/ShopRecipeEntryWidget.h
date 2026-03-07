#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "ProjectMK/Data/DataTable/ShopRecipeDataTableRow.h"
#include "ShopRecipeEntryWidget.generated.h"

class UButton;
class UHorizontalBox;
class UItemSlotWidget;
class UTextBlock;

UCLASS()
class PROJECTMK_API UShopRecipeEntryWidget : public UMKUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    void InitializeRecipe(FName InRecipeId, const FShopRecipeDataTableRow& InRecipeData);
    void RefreshState();

protected:
    UFUNCTION()
    void HandleCraftButtonClicked();

    UPROPERTY(EditAnywhere)
    TSubclassOf<UItemSlotWidget> ItemSlotClass;

    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* HBox_RecipeItems;

    UPROPERTY(meta = (BindWidget))
    UItemSlotWidget* OutputItemSlot;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Text_RecipeName;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Text_ResultKey;

    UPROPERTY(meta = (BindWidget))
    UButton* Button_Craft;

private:
    UPROPERTY(Transient)
    FName RecipeId;

    UPROPERTY(Transient)
    FShopRecipeDataTableRow RecipeData;
};
