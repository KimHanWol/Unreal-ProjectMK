#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "ShopRecipeSlotWidget.generated.h"

class UButton;
class UImage;
class UShopMenuContents;
class UTextBlock;

/** 상점 레시피 구매 목록의 한 줄. 위젯 블루프린트가 없으면 기본 레이아웃을 직접 만든다. */
UCLASS()
class PROJECTMK_API UShopRecipeSlotWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;

	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;

public:
	void InitializeRecipeSlot(UShopMenuContents* InOwnerShopMenu, FName InRecipeKey, bool bInCanUnlock);

private:
	void BuildDefaultWidgetTree();

	UFUNCTION()
	void OnUnlockButtonClicked();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ResultItemIconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RecipeNameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SummaryText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UnlockPriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> UnlockButton;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UShopMenuContents> OwnerShopMenu;

	FName RecipeKey = NAME_None;
	bool bCanUnlock = false;
};
