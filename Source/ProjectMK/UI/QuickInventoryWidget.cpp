// LINK

#include "ProjectMK/UI/QuickInventoryWidget.h"

#include "Components/HorizontalBox.h"
#include "ItemSlotWidget.h"
#include "ProjectMK/Component/InventoryComponent.h"

void UQuickInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildQuickInventorySlots();
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

void UQuickInventoryWidget::RebuildQuickInventorySlots()
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

	const TMap<FName, int32> InventoryItems = InventoryComponent->GetInventoryItems();
	const TArray<FName>& InventoryItemOrder = InventoryComponent->GetInventoryItemOrder();

	for (int32 SlotIndex = 0; SlotIndex < QuickInventorySlotCount; ++SlotIndex)
	{
		if (InventoryItemOrder.IsValidIndex(SlotIndex) == false || ItemSlotList.IsValidIndex(SlotIndex) == false)
		{
			break;
		}

		const FName& ItemKey = InventoryItemOrder[SlotIndex];
		const int32* ItemCountPtr = InventoryItems.Find(ItemKey);
		if (ItemCountPtr == nullptr)
		{
			continue;
		}

		ItemSlotList[SlotIndex]->SetItem(ItemKey, *ItemCountPtr);
	}
}
