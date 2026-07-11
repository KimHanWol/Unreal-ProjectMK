#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "InventoryEquipmentWidget.generated.h"

class UItemSlotWidget;

UCLASS()
class PROJECTMK_API UInventoryEquipmentWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	UItemSlotWidget* HelmetSlot;

	UPROPERTY(meta = (BindWidgetOptional))
	UItemSlotWidget* ArmorSlot;

	UPROPERTY(meta = (BindWidgetOptional))
	UItemSlotWidget* GlovesSlot;

	UPROPERTY(meta = (BindWidgetOptional))
	UItemSlotWidget* ShoesSlot;

	UPROPERTY(meta = (BindWidgetOptional))
	UItemSlotWidget* DrillSlot;

	UPROPERTY(meta = (BindWidgetOptional))
	UItemSlotWidget* BalloonSlot;
};
