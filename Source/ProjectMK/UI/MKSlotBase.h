#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"

#include "MKSlotBase.generated.h"

UCLASS()
class PROJECTMK_API UMKSlotBase : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool CanClickSlot() const;
	virtual void OnSlotClicked();

private:
	FReply HandleSlotClicked(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
};
