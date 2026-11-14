#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKSlotBase.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ItemSlotWidget.generated.h"

class FDragDropEvent;
class UDragDropOperation;
class UWidget;

UCLASS()
class PROJECTMK_API UItemSlotWidget : public UMKSlotBase
{
	GENERATED_BODY()

public:
	void ClearItem();
	void SetItem(FName ItemKey, int32 ItemCount);
	void SetSlotIndex(int32 InSlotIndex);
	void SetSlotAddress(EItemContainer Container, int32 InSlotIndex);
	virtual void UpdateTooltipWidget(UMKTooltipBase* TooltipWidget) const override;
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSecondaryAction, EItemContainer, int32, FName);
	FOnSecondaryAction OnSecondaryAction;

protected:
	virtual void UpdateWidget() override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool CanClickSlot() const override;
	virtual void OnSlotClicked() override;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_Count;

protected:
	EItemContainer SlotContainer = EItemContainer::Inventory;
	bool bShowKeyGuide = true;

private:
	bool HasItem() const;

private:
	UPROPERTY(Transient)
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(Transient)
	FName CurrentItemKey = NAME_None;

	UPROPERTY(Transient)
	int32 CurrentItemCount = 0;
	bool bPendingUseClick = false;
};
