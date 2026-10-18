#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "CraftingRecipeSlotWidget.generated.h"

class UButton;
class UCraftingMenuContents;
class UImage;
class UTextBlock;

UCLASS()
class PROJECTMK_API UCraftingRecipeSlotWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;

public:
	void InitializeRecipeSlot(UCraftingMenuContents* InOwnerMenu, FName InRecipeKey);
	void RefreshRecipe();

private:
	void BuildDefaultWidgetTree();

	UFUNCTION()
	void OnCraftButtonClicked();

	UPROPERTY(Transient)
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RecipeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RequiredItemsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CraftButtonText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CraftButton;

	TWeakObjectPtr<UCraftingMenuContents> OwnerMenu;
	FName RecipeKey = NAME_None;
};
