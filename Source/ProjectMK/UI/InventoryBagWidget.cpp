// LINK

#include "ProjectMK/UI/InventoryBagWidget.h"

#include "Blueprint/WidgetTree.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/UI/ItemSlotWidget.h"

void UInventoryBagWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CollectItemSlots();
	OnInventoryChanged();
}

void UInventoryBagWidget::BindEvents()
{
	Super::BindEvents();

	if (::IsValid(LocalPlayerCharacter) == false)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryChangedDelegate.AddUObject(this, &UInventoryBagWidget::OnInventoryChanged);
	}
}

void UInventoryBagWidget::UnbindEvents()
{
	Super::UnbindEvents();

	if (::IsValid(LocalPlayerCharacter) == false)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
	}
}

void UInventoryBagWidget::CollectItemSlots()
{
	if (WidgetTree == nullptr || ItemSlotList.Num() > 0)
	{
		return;
	}

	TArray<UWidget*> AllWidgets;
	WidgetTree->GetAllWidgets(AllWidgets);

	for (UWidget* Widget : AllWidgets)
	{
		UItemSlotWidget* ItemSlotWidget = Cast<UItemSlotWidget>(Widget);
		if (::IsValid(ItemSlotWidget) == false)
		{
			continue;
		}

		ItemSlotList.Add(ItemSlotWidget);
		if (ItemSlotList.Num() >= InventorySlotCount)
		{
			break;
		}
	}
}

void UInventoryBagWidget::OnInventoryChanged()
{
	for (UItemSlotWidget* ItemSlot : ItemSlotList)
	{
		if (::IsValid(ItemSlot))
		{
			ItemSlot->ClearItem();
		}
	}

	if (::IsValid(LocalPlayerCharacter) == false)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent) == false)
	{
		return;
	}

	const TMap<FName, int32> InventoryItems = InventoryComponent->GetInventoryItems();
	const TArray<FName>& InventoryItemOrder = InventoryComponent->GetInventoryItemOrder();

	for (int32 SlotIndex = 0; SlotIndex < InventoryItemOrder.Num(); ++SlotIndex)
	{
		if (ItemSlotList.IsValidIndex(SlotIndex) == false)
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
