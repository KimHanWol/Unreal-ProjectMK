#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKMenuContentsBase.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ShopMenuContents.generated.h"

class UPanelWidget;
class UVerticalBox;
class UItemQuantityDialog;
struct FOnAttributeChangeData;

UCLASS()
class PROJECTMK_API UShopMenuContents : public UMKMenuContentsBase
{
	GENERATED_BODY()

public:
	UShopMenuContents(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;

public:
	bool TryUnlockRecipe(FName RecipeKey);

private:
	void BuildDefaultWidgetTree();
	void RefreshRecipeSlotList();
	void OnItemSaleRequested(EItemContainer Container, int32 Index, FName ItemKey);
	void OnSaleConfirmed(FName ItemKey, int32 Count);
	void OnCoinChanged(const FOnAttributeChangeData& Data);
	void OnRecipeUnlocked(FName RecipeKey);
	UFUNCTION()
	void OnMenuVisibilityChanged(ESlateVisibility InVisibility);

	UPROPERTY(Transient)
	TObjectPtr<UPanelWidget> RecipeSlotListPanel;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> MerchantColumn;
	UPROPERTY(Transient)
	TObjectPtr<UItemQuantityDialog> QuantityDialog;
	EItemContainer PendingSaleContainer = EItemContainer::Inventory;
	int32 PendingSaleIndex = INDEX_NONE;
};
