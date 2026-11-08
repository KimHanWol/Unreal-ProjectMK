#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "HUDWidget.generated.h"

class UInventoryWidget;
class UMKMenuBase;
class UMKSlotBase;
class UMKTooltipBase;
class UItemQuantityDialog;

UCLASS()
class PROJECTMK_API UHUDWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;

	bool ToggleInventoryWidget();
	bool ToggleSkillTree();
	bool ToggleCrafting();
	bool TryOpenShop();
	bool IsMenuVisible() const;
	UMKTooltipBase* GetTooltipWidget() const;
	void SetHoveredSlot(UMKSlotBase* HoveredSlot);
	void ClearHoveredSlot(UMKSlotBase* HoveredSlot);
	void ShowRecipeTooltip(UWidget* Source, FName RecipeKey, bool bBlueprint = false);
	void ClearTooltipSource(UWidget* Source);

private:
	void UpdateTooltipWidget();
	void UpdateTooltipPosition();
	void OnDropConfirmed(FName ItemKey, int32 Count);
	void OnDropCancelled();

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
	UPROPERTY(Transient)
	TObjectPtr<UWidget> HoveredTooltipSource;
	UPROPERTY(Transient)
	TObjectPtr<UItemQuantityDialog> DropQuantityDialog;
	EItemContainer PendingDropContainer = EItemContainer::Inventory;
	int32 PendingDropIndex = INDEX_NONE;
	FName PendingDropKey;
};
