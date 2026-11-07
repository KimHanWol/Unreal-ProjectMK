#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKSlotBase.h"
#include "PlaytestQuickSlotWidget.generated.h"

UCLASS()
class PROJECTMK_API UPlaytestQuickSlotWidget : public UMKSlotBase
{
	GENERATED_BODY()

protected:
	virtual void UpdateWidget() override;
	virtual bool CanClickSlot() const override;
	virtual void OnSlotClicked() override;
	virtual void UpdateTooltipWidget(UMKTooltipBase* TooltipWidget) const override;

public:
	void SetSlotIndex(int32 InSlotIndex);
	void SetShowKeyGuide(bool bInShowKeyGuide);
	void RefreshSlot();

private:
	int32 SlotIndex = INDEX_NONE;
	FName ItemDataKey;
	bool bCanUseItem = false;
	bool bShowKeyGuide = true;
};
