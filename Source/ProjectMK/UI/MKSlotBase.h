#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"

#include "MKSlotBase.generated.h"

class UImage;
class UMKTooltipBase;

UCLASS()
class PROJECTMK_API UMKSlotBase : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual void UpdateTooltipWidget(UMKTooltipBase* TooltipWidget) const;

protected:
	virtual void UpdateWidget() override;

	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	virtual bool CanClickSlot() const;
	virtual void OnSlotClicked();
	virtual void OnSlotHovered();
	virtual void OnSlotUnhovered();

private:
	FReply HandleSlotClicked(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
	void UpdateFocusImageVisibility();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> FocusImage;

private:
	UPROPERTY(Transient)
	bool bHasSlotFocus = false;
};
