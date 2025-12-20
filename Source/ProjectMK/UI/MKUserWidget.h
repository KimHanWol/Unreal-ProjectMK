// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MKUserWidget.generated.h"

UCLASS()
class PROJECTMK_API UMKUserWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    virtual void BindEvents() {};
    virtual void UnbindEvents() {};
};