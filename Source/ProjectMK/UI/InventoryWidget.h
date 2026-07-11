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
	UInventoryWidget();

protected:
	virtual void NativeConstruct() override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

private:
	void RebuildItemSlotPanel();
	void RefreshInventory();
	int32 GetDesiredSlotCount() const;

	void OnInventoryChanged();
	void OnInventorySlotCountChanged(const FOnAttributeChangeData& Data);

protected:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UItemSlotWidget> ItemSlotClass;

	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1"))
	int32 InventoryColumnCount = 4;

	UPROPERTY(meta = (BindWidget))
	UGridPanel* ItemSlotPanel;

private:
	UPROPERTY(Transient)
	TArray<UItemSlotWidget*> ItemSlotList;
};
