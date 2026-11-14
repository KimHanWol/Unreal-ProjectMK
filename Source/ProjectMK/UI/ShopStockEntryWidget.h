#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "ProjectMK/UI/MKSlotBase.h"
#include "ShopStockEntryWidget.generated.h"

class UShopMenuContents;
class UShopStockEntryWidget;
class URecipeInfoWidget;
class UButton;
class UTextBlock;

UCLASS()
class PROJECTMK_API UShopStockDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient)
	TWeakObjectPtr<UShopStockEntryWidget> SourceEntry;
};

UCLASS()
class PROJECTMK_API UShopStockSlotWidget : public UMKSlotBase
{
	GENERATED_BODY()
public:
	UShopStockSlotWidget(const FObjectInitializer& Initializer);
	void SetEntry(UShopStockEntryWidget* InEntry);
protected:
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& Operation) override;
	virtual bool CanClickSlot() const override;
private:
	TWeakObjectPtr<UShopStockEntryWidget> Entry;
	bool bPendingClick = false;
};

UCLASS()
class PROJECTMK_API UShopStockEntryWidget : public UMKUserWidget
{
	GENERATED_BODY()
public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	void InitializeRecipe(UShopMenuContents* Shop, FName Key);
	void InitializeBuyback(UShopMenuContents* Shop, FGuid Id);
	UFUNCTION()
	void RequestPurchase();
protected:
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
private:
	void RefreshStock();
	TWeakObjectPtr<UShopMenuContents> OwnerShop;
	FName RecipeKey;
	FGuid BuybackId;
	UPROPERTY(Transient)
	TObjectPtr<UShopStockSlotWidget> StockSlot;
	UPROPERTY(Transient)
	TObjectPtr<URecipeInfoWidget> Info;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PriceText;
	UPROPERTY(Transient)
	TObjectPtr<UButton> BuyButton;
};
