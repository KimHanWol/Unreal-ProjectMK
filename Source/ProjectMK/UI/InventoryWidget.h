// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "InventoryWidget.generated.h"

class UInventoryBagWidget;
class UInventoryEquipmentWidget;

UCLASS()
class PROJECTMK_API UInventoryWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	UInventoryBagWidget* InventoryBag;

	UPROPERTY(meta = (BindWidgetOptional))
	UInventoryEquipmentWidget* InventoryEquipment;
};
