#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "PlaytestQuickSlotWidget.generated.h"

class UButton;
class UBorder;
class UImage;
class UTextBlock;

UCLASS()
class PROJECTMK_API UPlaytestQuickSlotWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;

public:
	void SetSlotIndex(int32 InSlotIndex);
	void RefreshSlot();

private:
	void BuildDefaultWidgetTree();

	UFUNCTION()
	void OnUseButtonClicked();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Quick Slot")
	FVector2D SlotSize = FVector2D(68.f, 84.f);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> UseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> FrameBorder;

private:
	int32 SlotIndex = INDEX_NONE;
};
