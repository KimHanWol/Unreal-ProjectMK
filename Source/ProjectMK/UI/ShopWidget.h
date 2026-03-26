#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "ShopWidget.generated.h"

class UInventoryComponent;
class UShopRecipeEntryWidget;
class UVerticalBox;

UCLASS()
class PROJECTMK_API UShopWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UShopRecipeEntryWidget> ShopRecipeEntryClass;

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* VBox_RecipeList;

private:
	void BuildRecipeList();
	void BindInventory();
	void RefreshEntries();
	void HandleInventoryChanged();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UShopRecipeEntryWidget>> RecipeEntries;

	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> BoundInventoryComponent;
};
