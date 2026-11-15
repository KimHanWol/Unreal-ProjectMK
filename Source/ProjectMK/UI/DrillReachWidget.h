#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "DrillReachWidget.generated.h"

UCLASS()
class PROJECTMK_API UDrillReachWidget : public UMKUserWidget
{
	GENERATED_BODY()
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
public:
	void SetReach(int32 Cells, int32 Maximum);
private:
	UPROPERTY(Transient) TObjectPtr<class USizeBox> Shaft;
	UPROPERTY(Transient) TObjectPtr<class UImage> DrillTip;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> Readout;
};
