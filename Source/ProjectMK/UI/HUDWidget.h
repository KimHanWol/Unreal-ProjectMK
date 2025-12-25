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
    virtual void BindEvents() override;
    virtual void UnbindEvents() override;

    void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
    void OnCurrentHealthChanged(const FOnAttributeChangeData& Data);

private:
    void UpdateHealthProgressBar();

protected:
    UPROPERTY(meta = (BindWidget))
    class UQuickInventoryWidget* QuickInventory;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* ProgressBar_Health;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* ProgressBar_Oxygen;
};