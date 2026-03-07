// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "HUDWidget.generated.h"

class UHealthBarWidget;
class UOxygenBarWidget;
class UQuickInventoryWidget;
class UShopWidget;

UCLASS()
class PROJECTMK_API UHUDWidget : public UMKUserWidget
{
    GENERATED_BODY()

public:
    bool ToggleShopWidget();

protected:
    UPROPERTY(meta = (BindWidget))
    UHealthBarWidget* HealthBar;

    UPROPERTY(meta = (BindWidget))
    UOxygenBarWidget* OxygenBar;

    UPROPERTY(meta = (BindWidget))
    UQuickInventoryWidget* QuickInventory;

    UPROPERTY(meta = (BindWidget))
    UShopWidget* Shop;
};
