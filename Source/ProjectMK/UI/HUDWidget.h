#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "HUDWidget.generated.h"

class UInventoryWidget;
class UMKMenuBase;
class UMKSlotBase;
class UMKTooltipBase;

UCLASS()
class PROJECTMK_API UHUDWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	bool ToggleInventoryWidget();
	bool ToggleSkillTree();
	bool IsMenuVisible() const;
	UMKTooltipBase* GetTooltipWidget() const;
	void SetHoveredSlot(UMKSlotBase* HoveredSlot);
	void ClearHoveredSlot(UMKSlotBase* HoveredSlot);

private:
	void UpdateTooltipWidget();
	void UpdateTooltipPosition();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInventoryWidget> Inventory;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMKMenuBase> Menu;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMKTooltipBase> Tooltip;

	UPROPERTY(EditDefaultsOnly, Category = "Tooltip")
	FVector2D TooltipOffset = FVector2D(16.f, 16.f);

	UPROPERTY(Transient)
	TObjectPtr<UMKSlotBase> HoveredSlot;
};
