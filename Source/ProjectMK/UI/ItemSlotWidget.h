// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "ItemSlotWidget.generated.h"

UCLASS()
class PROJECTMK_API UItemSlotWidget : public UMKUserWidget
{
    GENERATED_BODY()

public:
    void SetItem(FName ItemKey, int32 ItemCount);

protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_Count;

    UPROPERTY(meta = (BindWidget))
    class UImage* Image_Item;
};