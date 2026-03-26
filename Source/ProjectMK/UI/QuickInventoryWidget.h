#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "QuickInventoryWidget.generated.h"

class UItemSlotWidget;
class UHorizontalBox;
class UOverlay;

UCLASS()
class PROJECTMK_API UQuickInventoryWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

private:
	void SetInventorySlot();
	void OnInventoryChanged();

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UItemSlotWidget> ItemSlotClass;

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* HBox_Slot;

private:
	static constexpr int32 QuickInventorySlotCount = 8;

	UPROPERTY(Transient)
	TArray<UItemSlotWidget*> ItemSlotList;
};
