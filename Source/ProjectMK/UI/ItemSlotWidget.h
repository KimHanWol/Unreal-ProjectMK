#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKSlotBase.h"
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
	virtual void UpdateTooltipWidget(UMKTooltipBase* TooltipWidget) const override;

protected:
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool CanClickSlot() const override;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_Count;

private:
	UWidget* CreateDragVisualWidget() const;
	bool HasItem() const;

private:
	UPROPERTY(Transient)
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(Transient)
	FName CurrentItemKey = NAME_None;

	UPROPERTY(Transient)
	int32 CurrentItemCount = 0;
};
