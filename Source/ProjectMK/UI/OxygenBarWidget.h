#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "OxygenBarWidget.generated.h"

UCLASS()
class PROJECTMK_API UOxygenBarWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

private:
	float GetOxygenRatio() const;
	void StartProgressInterpolation();
	void UpdateOxygenProgressBar();

	void OnMaxOxygenChanged(const FOnAttributeChangeData& Data);
	void OnCurrentOxygenChanged(const FOnAttributeChangeData& Data);

protected:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* ProgressBar_Oxygen;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float ProgressBarInterpSpeed = 6.f;

	bool bIsInterpolatingOxygen = false;
	float CurrentOxygenPercent = 1.f;
	float TargetOxygenPercent = 1.f;
};
