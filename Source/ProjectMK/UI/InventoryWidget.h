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

protected:
	virtual void NativeConstruct() override;

	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

private:
	void UpdateItemSlotPanel();
	void RefreshInventory();

	int32 GetDesiredSlotCount() const;

	void OnInventoryChanged();
	void OnInventorySlotCountChanged(const FOnAttributeChangeData& Data);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UItemSlotWidget> ItemSlotClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 InventoryColumnCount = 4;

	UPROPERTY(meta = (BindWidget))
	UGridPanel* ItemSlotPanel;

private:
	UPROPERTY(Transient)
	TArray<UItemSlotWidget*> ItemSlotList;
};
