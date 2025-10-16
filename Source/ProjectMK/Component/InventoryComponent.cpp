// LINK

#include "ProjectMK/Component/InventoryComponent.h"

UInventoryComponent::UInventoryComponent()
{
	
}

int32 UInventoryComponent::GetItemCount(FName ItemUID)
{
	int32* ItemCountPtr = InventoryItemMap.Find(ItemUID);
	if (ItemCountPtr)
	{
		return *ItemCountPtr;
	}
	
	return 0;
}

void UInventoryComponent::SetItemCount(FName ItemUID, int32 ItemCount)
{
	if (InventoryItemMap.Contains(ItemUID) == false)
	{
		return;
	}

	InventoryItemMap[ItemUID] = FMath::Max(InventoryItemMap[ItemUID] - ItemCount, 0);

	if (InventoryItemMap[ItemUID] == 0)
	{
		InventoryItemMap.Remove(ItemUID);
	}

	OnInventoryUpdated();
}

void UInventoryComponent::OnGainItem(FName ItemUID, int32 ItemCount)
{
	if (InventoryItemMap.Contains(ItemUID) == false && InventoryItemMap.Num() >= MaxInventoryCount)
	{
		ensure(false);
		return;
	}

	InventoryItemMap.FindOrAdd(ItemUID) += ItemCount;

	OnInventoryUpdated();
}

void UInventoryComponent::OnSpendItem(FName ItemUID, int32 ItemCount)
{
	InventoryItemMap.FindOrAdd(ItemUID) -= ItemCount;
	
	if (InventoryItemMap[ItemUID] == 0)
	{
		InventoryItemMap.Remove(ItemUID);
	}

	OnInventoryUpdated();
}

void UInventoryComponent::OnInventoryUpdated()
{
	//TArray<FInventoryItemData> ItemDataList;
	//for (const auto& InventoryItem : InventoryItemMap)
	//{
	//	FInventoryItemData NewInventoryData;
	//	NewInventoryData.ItemUID = InventoryItem.Key;
	//	NewInventoryData.ItemCount = InventoryItem.Value;

	//	ItemDataList.Add(NewInventoryData);
	//}

	//OnInventoryChangedDelegate.Broadcast(ItemDataList);
}
