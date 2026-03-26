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

	void OnMaxOxygenChanged(const FOnAttributeChangeData& Data);
	void OnCurrentOxygenChanged(const FOnAttributeChangeData& Data);

private:
	float GetOxygenRatio() const;
	void UpdateOxygenProgressBar();

protected:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* ProgressBar_Oxygen;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float ProgressBarInterpSpeed = 6.f;

	float CurrentOxygenPercent = 1.f;
	float TargetOxygenPercent = 1.f;
};
