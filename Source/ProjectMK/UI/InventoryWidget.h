#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "InventoryWidget.generated.h"

class UGridPanel;
class UItemSlotWidget;
struct FOnAttributeChangeData;

UCLASS()
class PROJECTMK_API UInventoryWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	void SetShowQuickSlots(bool bShow);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;

	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

private:
	void BuildDefaultWidgetTree();
	void UpdateItemSlotPanel();
	void RefreshInventory();

	int32 GetDesiredSlotCount() const;

	void OnInventoryChanged();
	void OnInventorySlotCountChanged(const FOnAttributeChangeData& Data);
	void OnSkillPurchased(FName SkillKey);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UItemSlotWidget> ItemSlotClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 InventoryColumnCount = 4;

	UPROPERTY(meta = (BindWidgetOptional))
	UGridPanel* ItemSlotPanel;

private:
	UPROPERTY(Transient)
	TArray<UItemSlotWidget*> ItemSlotList;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UItemSlotWidget>> QuickSlotList;
	UPROPERTY(Transient)
	TObjectPtr<UGridPanel> QuickSlotPanel;
	bool bShowQuickSlots = false;
};
