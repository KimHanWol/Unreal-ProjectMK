#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKMenuContentsBase.h"
#include "ShopMenuContents.generated.h"

class UButton;
class UPanelWidget;
class UShopRecipeSlotWidget;
class UShopSellSlotWidget;
class UTextBlock;
struct FOnAttributeChangeData;

/** 광물·아이템 판매와 레시피 구매 메뉴. 위젯 블루프린트가 없으면 기본 레이아웃을 직접 만든다. */
UCLASS()
class PROJECTMK_API UShopMenuContents : public UMKMenuContentsBase
{
	GENERATED_BODY()

public:
	UShopMenuContents(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;

public:
	bool TrySellItem(FName ItemUID);
	bool TryUnlockRecipe(FName RecipeKey);

private:
	void BuildDefaultWidgetTree();
	void RefreshSellSlotList();
	void RefreshRecipeSlotList();
	void ApplySaleResult(bool bSold);

	UFUNCTION()
	void OnSellAllButtonClicked();

	void OnInventoryChanged();
	void OnCoinChanged(const FOnAttributeChangeData& Data);
	void OnRecipeUnlocked(FName RecipeKey);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SellSlotListPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TotalPriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SellAllButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> RecipeSlotListPanel;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UShopSellSlotWidget> SellSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UShopRecipeSlotWidget> RecipeSlotWidgetClass;
};
