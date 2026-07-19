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

private:
	float GetHealthRatio() const;
	void StartProgressInterpolation();
	void UpdateHealthProgressBar();

	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	void OnCurrentHealthChanged(const FOnAttributeChangeData& Data);

protected:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* ProgressBar_Health;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float ProgressBarInterpSpeed = 6.f;

	bool bIsInterpolatingHealth = false;
	float CurrentHealthPercent = 1.f;
	float TargetHealthPercent = 1.f;
};
