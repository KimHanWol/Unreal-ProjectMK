#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "StorageChestWidget.generated.h"

class AStorageChestActor;
class UGridPanel;
class UItemSlotWidget;

UCLASS()
class PROJECTMK_API UStorageChestWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;

protected:
	virtual void UpdateWidget() override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

public:
	bool OpenChest(AStorageChestActor* Chest);
	UFUNCTION()
	void CloseChest();
	bool TryStoreStack(EItemContainer Container, int32 Index);

private:
	void BuildDefaultWidgetTree();
	void SetChest(AStorageChestActor* Chest);
	void TransferStack(int32 Index);
	void RefreshSlots();
	void UpdateInteractionRange();
	void OnStorageSecondaryAction(EItemContainer Container, int32 Index, FName Key);
	UFUNCTION()
	void OnChestDestroyed(AActor* Actor);

	TWeakObjectPtr<AStorageChestActor> StorageChest;
	UPROPERTY(Transient)
	TObjectPtr<UGridPanel> StorageGrid;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UItemSlotWidget>> StorageSlots;
	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;
	FTimerHandle RangeTimer;
};
