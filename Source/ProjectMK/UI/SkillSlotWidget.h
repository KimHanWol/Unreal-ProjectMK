#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKSlotBase.h"

#include "SkillSlotWidget.generated.h"

struct FOnAttributeChangeData;

class UImage;

UCLASS()
class PROJECTMK_API USkillSlotWidget : public UMKSlotBase
{
	GENERATED_BODY()

protected:
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;
	virtual bool CanClickSlot() const override;
	virtual void OnSlotClicked() override;

private:
	void UpdateCompleteImage();
	void UpdateSkillImage();

	void OnCoinChanged(const FOnAttributeChangeData& Data);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetSkillRowNames"))
	FName SkillKeyName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CompleteImage;
};
