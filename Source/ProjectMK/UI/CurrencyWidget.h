// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "CurrencyWidget.generated.h"

UCLASS()
class PROJECTMK_API UCurrencyWidget : public UMKUserWidget
{
    GENERATED_BODY()

protected:
    virtual void BindEvents() override;
    virtual void UnbindEvents() override;

    void OnCoinChanged(const FOnAttributeChangeData& Data);

protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_Coin;
};