// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "HealthBarWidget.generated.h"

UCLASS()
class PROJECTMK_API UHealthBarWidget : public UMKUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual void BindEvents() override;
    virtual void UnbindEvents() override;

    void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
    void OnCurrentHealthChanged(const FOnAttributeChangeData& Data);

private:
    float GetHealthRatio() const;
    void UpdateHealthProgressBar();

protected:
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* ProgressBar_Health;

    UPROPERTY(EditAnywhere, Category = "HUD")
    float ProgressBarInterpSpeed = 6.f;

    float CurrentHealthPercent = 1.f;
    float TargetHealthPercent = 1.f;
};
