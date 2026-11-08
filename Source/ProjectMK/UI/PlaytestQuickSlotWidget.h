#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/ItemSlotWidget.h"
#include "PlaytestQuickSlotWidget.generated.h"

UCLASS()
class PROJECTMK_API UPlaytestQuickSlotWidget : public UItemSlotWidget
{
	GENERATED_BODY()
public:
	UPlaytestQuickSlotWidget(const FObjectInitializer& ObjectInitializer);
	void SetShowKeyGuide(bool bInShowKeyGuide);
	void RefreshSlot();
};
