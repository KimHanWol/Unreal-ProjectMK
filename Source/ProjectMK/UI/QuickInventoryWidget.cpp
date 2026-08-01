// LINK

#include "ProjectMK/UI/QuickInventoryWidget.h"

#include "Components/HorizontalBox.h"
#include "ItemSlotWidget.h"
#include "ProjectMK/Component/InventoryComponent.h"

void UQuickInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateQuickInventorySlots();
	OnInventoryChanged();
}

void UQuickInventoryWidget::BindEvents()
{
	Super::BindEvents();

	if (UInventoryComponent* InventoryComponent = GetLocalInventoryComponent())
	{
		InventoryComponent->OnInventoryChangedDelegate.AddUObject(this, &UQuickInventoryWidget::OnInventoryChanged);
	}
}

void UQuickInventoryWidget::UnbindEvents()
{
	Super::UnbindEvents();

	if (UInventoryComponent* InventoryComponent = GetLocalInventoryComponent())
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
	}
}

void UQuickInventoryWidget::UpdateQuickInventorySlots()
{
	if (::IsValid(HBox_Slot) == false || ItemSlotList.Num() > 0)
	{
		return;
	}

	for (int32 i = 0; i < QuickInventorySlotCount; i++)
	{
		UItemSlotWidget* NewItemSlot = CreateWidget<UItemSlotWidget>(this, ItemSlotClass);
		if (::IsValid(NewItemSlot))
		{
			NewItemSlot->SetSlotIndex(i);
			HBox_Slot->AddChildToHorizontalBox(NewItemSlot);
			ItemSlotList.Add(NewItemSlot);
		}
	}
}

void UQuickInventoryWidget::OnInventoryChanged()
{
	for (UItemSlotWidget* ItemSlot : ItemSlotList)
	{
		if (::IsValid(ItemSlot))
		{
			ItemSlot->ClearItem();
		}
	}

	UInventoryComponent* InventoryComponent = GetLocalInventoryComponent();
	if (::IsValid(InventoryComponent) == false)
	{
		return;
	}

	const TArray<FInventorySlotData>& InventorySlotDataList = InventoryComponent->GetInventorySlotDataList();

	for (int32 SlotIndex = 0; SlotIndex < QuickInventorySlotCount; ++SlotIndex)
	{
		if (InventorySlotDataList.IsValidIndex(SlotIndex) == false || ItemSlotList.IsValidIndex(SlotIndex) == false)
		{
			break;
		}

		const FInventorySlotData& InventorySlotData = InventorySlotDataList[SlotIndex];
		if (InventorySlotData.IsEmpty())
		{
			continue;
		}

		ItemSlotList[SlotIndex]->SetItem(InventorySlotData.ItemUID, InventorySlotData.ItemCount);
	}
}
