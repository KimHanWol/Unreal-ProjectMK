#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "ShopSellSlotWidget.generated.h"

class UButton;
class UImage;
class UShopMenuContents;
class UTextBlock;

/** 상점 판매 목록의 광물 한 줄. 위젯 블루프린트가 없으면 기본 레이아웃을 직접 만든다. */
UCLASS()
class PROJECTMK_API UShopSellSlotWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;

public:
	void InitializeSellSlot(UShopMenuContents* InOwnerShopMenu, FName InItemUID, int32 InItemCount, int32 InUnitPrice);

private:
	void BuildDefaultWidgetTree();

	UFUNCTION()
	void OnSellButtonClicked();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemPriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SellButton;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	FVector2D ItemIconSize = FVector2D(32.f, 32.f);

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UShopMenuContents> OwnerShopMenu;

	FName ItemUID = NAME_None;
	int32 ItemCount = 0;
	int32 UnitPrice = 0;
};
