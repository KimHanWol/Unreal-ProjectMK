// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"
#include "InventoryEquipmentWidget.generated.h"

class UItemSlotWidget;

UCLASS()
class PROJECTMK_API UInventoryEquipmentWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

private:
	void OnInventoryChanged();
	void RefreshEquipmentSlot(EEuipmentType EquipmentType, UItemSlotWidget* PreferredSlot, UItemSlotWidget* FallbackSlot = nullptr);

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
