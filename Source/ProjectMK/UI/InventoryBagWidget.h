// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "InventoryBagWidget.generated.h"

class UItemSlotWidget;

UCLASS()
class PROJECTMK_API UInventoryBagWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

private:
	void CollectItemSlots();
	void OnInventoryChanged();

protected:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 InventorySlotCount = 30;

private:
	UPROPERTY(Transient)
	TArray<UItemSlotWidget*> ItemSlotList;
};
