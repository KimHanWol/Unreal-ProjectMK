// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "HUDWidget.generated.h"

class UQuickInventoryWidget;

UCLASS()
class PROJECTMK_API UHUDWidget : public UMKUserWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget))
    class UQuickInventoryWidget* QuickInventory;
};