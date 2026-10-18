#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "QuickInventoryWidget.generated.h"

class UHorizontalBox;
class UPlaytestQuickSlotWidget;
class UTextBlock;

/** 인벤토리 첫 8칸의 전용 퀵바. WBP가 없으면 독립된 기본 레이아웃을 만든다. */
UCLASS()
class PROJECTMK_API UQuickInventoryWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	UQuickInventoryWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;

private:
	void BuildDefaultWidgetTree();
	void UpdateQuickInventorySlots();
	void UpdateRecall();
	void OnInventoryChanged();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Quick Inventory")
	TSubclassOf<UPlaytestQuickSlotWidget> QuickSlotClass;

	UPROPERTY(EditDefaultsOnly, Category = "Quick Inventory")
	FVector2D ViewportAnchor = FVector2D(0.5f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Quick Inventory")
	FVector2D ViewportOffset = FVector2D(0.f, -24.f);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HBox_Slot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RecallText;

private:
	static constexpr int32 QuickInventorySlotCount = 8;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPlaytestQuickSlotWidget>> ItemSlotList;

	FTimerHandle RecallTimerHandle;
};
