// LINK

#include "ProjectMK/Component/InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/ShopRecipeDataTableRow.h"

namespace
{
	const FName InventoryCoinItemKey = TEXT("Coin");
	const FName InventoryOxygenItemKey = TEXT("Oxygen");
}

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	const UAttributeSet_Character* CharacterAttributeSet = GetCharacterAttributeSet();
	if (::IsValid(CharacterAttributeSet) == false)
	{
		return;
	}

	ItemCollectRange = CharacterAttributeSet->GetItemCollectRange();
	SetGainRadius(ItemCollectRange);

	OnComponentBeginOverlap.AddDynamic(this, &UInventoryComponent::OnSphereOverlap);
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ItemGainRange <= 0.f)
	{
		return;
	}

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(GetOwner());

	TArray<AActor*> OverlappedActors;
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		GetOwner()->GetActorLocation(),
		GetScaledSphereRadius(),
		{},
		AItemBase::StaticClass(),
		IgnoreActors,
		OverlappedActors
	);

	for (const auto& OverlappedActor : OverlappedActors)
	{
		AItemBase* OverlappedItem = Cast<AItemBase>(OverlappedActor);
		if (::IsValid(OverlappedItem) == false)
		{
			continue;
		}

		if (FVector::Dist(OverlappedActor->GetActorLocation(), GetOwner()->GetActorLocation()) > ItemGainRange)
		{
			continue;
		}

		if (OverlappedItem->IsOccupied() || CanCollectWorldItem(OverlappedItem) == false)
		{
			continue;
		}

		OverlappedItem->TryLoot(Cast<AMKCharacter>(GetOwner()));
	}
}

float UInventoryComponent::GetItemGainRange() const
{
	return ItemGainRange;
}

int32 UInventoryComponent::GetItemCount(FName ItemUID)
{
	UpdateInventoryItemMap();

	const int32* ItemCountPtr = InventoryItemMap.Find(ItemUID);
	if (ItemCountPtr)
	{
		return *ItemCountPtr;
	}

	return 0;
}

int32 UInventoryComponent::GetMaxInventoryCount() const
{
	const int32 FallbackInventorySlotCount = FMath::Max(1, MaxInventoryCount);

	const UAttributeSet_Character* CharacterAttributeSet = GetCharacterAttributeSet();
	if (::IsValid(CharacterAttributeSet) == false)
	{
		return FallbackInventorySlotCount;
	}

	return FMath::Max(1, FMath::RoundToInt(CharacterAttributeSet->GetInventorySlotCount()));
}

float UInventoryComponent::GetInventoryOccupancyRatio() const
{
	const_cast<UInventoryComponent*>(this)->UpdateInventorySlotDataList();

	const int32 MaxInventorySlotCount = FMath::Max(1, GetMaxInventoryCount());
	int32 OccupiedSlotCount = 0;
	for (const FInventorySlotData& SlotData : InventorySlotDataList)
	{
		if (SlotData.IsEmpty() == false)
		{
			OccupiedSlotCount++;
		}
	}

	return FMath::Clamp(static_cast<float>(OccupiedSlotCount) / static_cast<float>(MaxInventorySlotCount), 0.f, 1.f);
}

void UInventoryComponent::SetItemCount(FName ItemUID, int32 ItemCount)
{
	if (ItemUID.IsNone() || ItemCount <= 0)
	{
		return;
	}

	SpendItem(ItemUID, ItemCount);
}

bool UInventoryComponent::AddItem(FName ItemUID, int32 ItemCount)
{
	if (ItemUID.IsNone() || ItemCount <= 0)
	{
		return false;
	}

	if (CanGainItem(ItemUID, ItemCount) == false)
	{
		return false;
	}

	GainItem(ItemUID, ItemCount);
	return true;
}

void UInventoryComponent::RemoveAllOreItems()
{
	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	UpdateInventorySlotDataList();
	bool bRemovedOre = false;
	for (FInventorySlotData& SlotData : InventorySlotDataList)
	{
		if (SlotData.IsEmpty())
		{
			continue;
		}

		const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, SlotData.ItemUID);
		if (ItemDataTableRow == nullptr || ItemDataTableRow->bIsOre == false)
		{
			continue;
		}

		SlotData.Clear();
		bRemovedOre = true;
	}

	if (bRemovedOre)
	{
		OnInventoryUpdated();
	}
}

bool UInventoryComponent::CanGainItem(FName ItemUID, int32 ItemCount)
{
	const_cast<UInventoryComponent*>(this)->UpdateInventorySlotDataList();
	return CalculateAvailableItemCapacity(ItemUID) >= ItemCount;
}

bool UInventoryComponent::TryCollectWorldItem(AItemBase* ItemActor)
{
	if (::IsValid(ItemActor) == false)
	{
		return false;
	}

	const FName ItemKey = ItemActor->GetItemKey();
	if (ItemActor->IsInitialized() == false)
	{
		return false;
	}

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return false;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemKey);
	if (ItemDataTableRow == nullptr)
	{
		return false;
	}

	const int32 ItemCount = FMath::Max(1, ItemActor->GetItemCount());
	if (ItemDataTableRow->bIsInventoryItem)
	{
		if (CanGainItem(ItemKey, ItemCount) == false)
		{
			return false;
		}

		GainItem(ItemKey, ItemCount);
	}
	else
	{
		const bool bGrantedItemReward = TryGrantNonInventoryItem(ItemKey, ItemCount);
		if (bGrantedItemReward == false)
		{
			return false;
		}
	}

	ItemActor->OnLootFinished();
	return true;
}

bool UInventoryComponent::TryMoveItemSlot(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	UpdateInventorySlotDataList();

	if (IsValidSlotIndex(SourceSlotIndex) == false || IsValidSlotIndex(TargetSlotIndex) == false || SourceSlotIndex == TargetSlotIndex)
	{
		return false;
	}

	FInventorySlotData& SourceSlotData = InventorySlotDataList[SourceSlotIndex];
	FInventorySlotData& TargetSlotData = InventorySlotDataList[TargetSlotIndex];

	if (SourceSlotData.IsEmpty())
	{
		return false;
	}

	if (TargetSlotData.IsEmpty())
	{
		TargetSlotData = SourceSlotData;
		SourceSlotData.Clear();
		OnInventoryUpdated();
		return true;
	}

	if (SourceSlotData.ItemUID == TargetSlotData.ItemUID)
	{
		const int32 MaxStackCount = GetItemMaxStackCount(SourceSlotData.ItemUID);
		if (TargetSlotData.ItemCount >= MaxStackCount)
		{
			return false;
		}

		const int32 MoveCount = FMath::Min(SourceSlotData.ItemCount, MaxStackCount - TargetSlotData.ItemCount);
		if (MoveCount <= 0)
		{
			return false;
		}

		TargetSlotData.ItemCount += MoveCount;
		SourceSlotData.ItemCount -= MoveCount;
		if (SourceSlotData.ItemCount <= 0)
		{
			SourceSlotData.Clear();
		}

		OnInventoryUpdated();
		return true;
	}

	Swap(SourceSlotData, TargetSlotData);
	OnInventoryUpdated();
	return true;
}

void UInventoryComponent::SetGainRadius(float NewRadius)
{
	ItemGainRange = FMath::Max(0.f, NewRadius);
	SetSphereRadius(ItemGainRange);
}

bool UInventoryComponent::CraftShopRecipe(const FShopRecipeDataTableRow& ShopRecipeData)
{
	if (CanCraftShopRecipe(ShopRecipeData) == false)
	{
		return false;
	}

	for (const FShopRecipeItem& RequiredItem : ShopRecipeData.RequiredItems)
	{
		SpendItem(RequiredItem.GetItemKey(), RequiredItem.ItemCount);
	}
	AddItem(ShopRecipeData.GetResultItemKey(), 1);

	return true;
}

bool UInventoryComponent::CanCraftShopRecipe(const FShopRecipeDataTableRow& ShopRecipeData) const
{
	const_cast<UInventoryComponent*>(this)->UpdateInventorySlotDataList();

	if (ShopRecipeData.GetResultItemKey().IsNone())
	{
		return false;
	}

	TMap<FName, int32> SimulatedInventory = GetInventoryItems();
	for (const FShopRecipeItem& RequiredItem : ShopRecipeData.RequiredItems)
	{
		const FName RequiredItemKey = RequiredItem.GetItemKey();
		if (RequiredItemKey.IsNone() || RequiredItem.ItemCount <= 0)
		{
			return false;
		}

		int32* InventoryItemCountPtr = SimulatedInventory.Find(RequiredItemKey);
		if (InventoryItemCountPtr == nullptr || (*InventoryItemCountPtr) < RequiredItem.ItemCount)
		{
			return false;
		}

		(*InventoryItemCountPtr) -= RequiredItem.ItemCount;
		if ((*InventoryItemCountPtr) <= 0)
		{
			SimulatedInventory.Remove(RequiredItemKey);
		}
	}

	if (CalculateAvailableItemCapacity(ShopRecipeData.GetResultItemKey()) <= 0 && SimulatedInventory.Contains(ShopRecipeData.GetResultItemKey()) == false)
	{
		return false;
	}

	return true;
}

void UInventoryComponent::GainItem(FName ItemUID, int32 ItemCount)
{
	if (CanGainItem(ItemUID, ItemCount) == false)
	{
		ensure(false);
		return;
	}

	UpdateInventorySlotDataList();

	int32 RemainingItemCount = ItemCount;
	const int32 MaxStackCount = GetItemMaxStackCount(ItemUID);

	while (RemainingItemCount > 0)
	{
		const int32 StackableSlotIndex = FindStackableSlotIndex(ItemUID);
		if (StackableSlotIndex != INDEX_NONE)
		{
			FInventorySlotData& StackableSlotData = InventorySlotDataList[StackableSlotIndex];
			const int32 AddedItemCount = FMath::Min(RemainingItemCount, MaxStackCount - StackableSlotData.ItemCount);
			StackableSlotData.ItemCount += AddedItemCount;
			RemainingItemCount -= AddedItemCount;
			continue;
		}

		const int32 EmptySlotIndex = FindEmptySlotIndex();
		if (EmptySlotIndex == INDEX_NONE)
		{
			break;
		}

		FInventorySlotData& EmptySlotData = InventorySlotDataList[EmptySlotIndex];
		EmptySlotData.ItemUID = ItemUID;
		EmptySlotData.ItemCount = FMath::Min(RemainingItemCount, MaxStackCount);
		RemainingItemCount -= EmptySlotData.ItemCount;
	}

	OnInventoryUpdated();
}

void UInventoryComponent::SpendItem(FName ItemUID, int32 ItemCount)
{
	if (ItemUID.IsNone() || ItemCount <= 0)
	{
		return;
	}

	UpdateInventorySlotDataList();

	int32 RemainingItemCount = ItemCount;
	for (FInventorySlotData& SlotData : InventorySlotDataList)
	{
		if (SlotData.ItemUID != ItemUID || SlotData.ItemCount <= 0)
		{
			continue;
		}

		const int32 SpentItemCount = FMath::Min(RemainingItemCount, SlotData.ItemCount);
		SlotData.ItemCount -= SpentItemCount;
		RemainingItemCount -= SpentItemCount;
		if (SlotData.ItemCount <= 0)
		{
			SlotData.Clear();
		}

		if (RemainingItemCount <= 0)
		{
			break;
		}
	}

	OnInventoryUpdated();
}

void UInventoryComponent::OnInventoryUpdated()
{
	UpdateInventorySlotDataList();
	NormalizeInventorySlotDataList();
	UpdateInventoryItemMap();
	OnInventoryChangedDelegate.Broadcast();
}

const UAttributeSet_Character* UInventoryComponent::GetCharacterAttributeSet() const
{
	const AActor* Owner = GetOwner();
	if (::IsValid(Owner) == false)
	{
		return nullptr;
	}

	const UAbilitySystemComponent* OwnerASC = Owner->GetComponentByClass<UAbilitySystemComponent>();
	if (::IsValid(OwnerASC) == false)
	{
		return nullptr;
	}

	return Cast<UAttributeSet_Character>(OwnerASC->GetAttributeSet(UAttributeSet_Character::StaticClass()));
}

void UInventoryComponent::UpdateInventorySlotDataList()
{
	const int32 DesiredSlotCount = GetMaxInventoryCount();
	if (InventorySlotDataList.Num() == DesiredSlotCount)
	{
		return;
	}

	TArray<FInventorySlotData> PreviousSlotDataList = InventorySlotDataList;
	InventorySlotDataList.Init(FInventorySlotData(), DesiredSlotCount);
	for (int32 SlotIndex = 0; SlotIndex < PreviousSlotDataList.Num() && SlotIndex < DesiredSlotCount; ++SlotIndex)
	{
		InventorySlotDataList[SlotIndex] = PreviousSlotDataList[SlotIndex];
	}
}

int32 UInventoryComponent::FindEmptySlotIndex() const
{
	for (int32 SlotIndex = 0; SlotIndex < InventorySlotDataList.Num(); ++SlotIndex)
	{
		if (InventorySlotDataList[SlotIndex].IsEmpty())
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

bool UInventoryComponent::IsValidSlotIndex(int32 SlotIndex) const
{
	return InventorySlotDataList.IsValidIndex(SlotIndex);
}

const TArray<FInventorySlotData>& UInventoryComponent::GetInventorySlotDataList()
{
	UpdateInventorySlotDataList();
	return InventorySlotDataList;
}

void UInventoryComponent::UpdateInventoryItemMap()
{
	InventoryItemMap.Reset();

	for (const FInventorySlotData& SlotData : InventorySlotDataList)
	{
		if (SlotData.IsEmpty())
		{
			continue;
		}

		InventoryItemMap.FindOrAdd(SlotData.ItemUID) += SlotData.ItemCount;
	}
}

void UInventoryComponent::NormalizeInventorySlotDataList()
{
	for (int32 SourceSlotIndex = 0; SourceSlotIndex < InventorySlotDataList.Num(); ++SourceSlotIndex)
	{
		FInventorySlotData& SourceSlotData = InventorySlotDataList[SourceSlotIndex];
		if (SourceSlotData.IsEmpty())
		{
			continue;
		}

		const int32 MaxStackCount = GetItemMaxStackCount(SourceSlotData.ItemUID);
		if (SourceSlotData.ItemCount > MaxStackCount)
		{
			SourceSlotData.ItemCount = MaxStackCount;
		}

		for (int32 TargetSlotIndex = 0; TargetSlotIndex < SourceSlotIndex; ++TargetSlotIndex)
		{
			FInventorySlotData& TargetSlotData = InventorySlotDataList[TargetSlotIndex];
			if (TargetSlotData.ItemUID != SourceSlotData.ItemUID || TargetSlotData.ItemCount >= MaxStackCount)
			{
				continue;
			}

			const int32 MergeItemCount = FMath::Min(SourceSlotData.ItemCount, MaxStackCount - TargetSlotData.ItemCount);
			if (MergeItemCount <= 0)
			{
				continue;
			}

			TargetSlotData.ItemCount += MergeItemCount;
			SourceSlotData.ItemCount -= MergeItemCount;
			if (SourceSlotData.ItemCount <= 0)
			{
				SourceSlotData.Clear();
				break;
			}
		}
	}
}

int32 UInventoryComponent::GetItemMaxStackCount(FName ItemUID) const
{
	UDataManager* DataManager = UDataManager::Get(const_cast<UInventoryComponent*>(this));
	if (::IsValid(DataManager) == false)
	{
		return 99;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemUID);
	if (ItemDataTableRow == nullptr)
	{
		return 99;
	}

	return FMath::Max(1, ItemDataTableRow->MaxStackCount);
}

int32 UInventoryComponent::FindStackableSlotIndex(FName ItemUID) const
{
	const int32 MaxStackCount = GetItemMaxStackCount(ItemUID);
	for (int32 SlotIndex = 0; SlotIndex < InventorySlotDataList.Num(); ++SlotIndex)
	{
		const FInventorySlotData& SlotData = InventorySlotDataList[SlotIndex];
		if (SlotData.ItemUID == ItemUID && SlotData.ItemCount < MaxStackCount)
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

int32 UInventoryComponent::CalculateAvailableItemCapacity(FName ItemUID) const
{
	if (ItemUID.IsNone())
	{
		return 0;
	}

	const int32 MaxStackCount = GetItemMaxStackCount(ItemUID);
	int32 AvailableCapacity = 0;

	for (const FInventorySlotData& SlotData : InventorySlotDataList)
	{
		if (SlotData.IsEmpty())
		{
			AvailableCapacity += MaxStackCount;
			continue;
		}

		if (SlotData.ItemUID == ItemUID)
		{
			AvailableCapacity += FMath::Max(0, MaxStackCount - SlotData.ItemCount);
		}
	}

	return AvailableCapacity;
}

bool UInventoryComponent::CanCollectWorldItem(AItemBase* ItemActor) const
{
	if (::IsValid(ItemActor) == false)
	{
		return false;
	}

	UDataManager* DataManager = UDataManager::Get(const_cast<UInventoryComponent*>(this));
	if (::IsValid(DataManager) == false)
	{
		return false;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemActor->GetItemKey());
	if (ItemDataTableRow == nullptr)
	{
		return false;
	}

	const int32 ItemCount = FMath::Max(1, ItemActor->GetItemCount());
	if (ItemDataTableRow->bIsInventoryItem)
	{
		return const_cast<UInventoryComponent*>(this)->CanGainItem(ItemActor->GetItemKey(), ItemCount);
	}

	return CanGrantNonInventoryItem(ItemActor->GetItemKey(), ItemCount);
}

bool UInventoryComponent::CanGrantNonInventoryItem(FName ItemKey, int32 ItemCount) const
{
	if (ItemCount <= 0)
	{
		return false;
	}

	AMKCharacter* OwnerCharacter = Cast<AMKCharacter>(GetOwner());
	if (::IsValid(OwnerCharacter) == false || ::IsValid(OwnerCharacter->GetSkillComponent()) == false)
	{
		return false;
	}

	return ItemKey == InventoryCoinItemKey || ItemKey == InventoryOxygenItemKey;
}

bool UInventoryComponent::TryGrantNonInventoryItem(FName ItemKey, int32 ItemCount)
{
	AMKCharacter* OwnerCharacter = Cast<AMKCharacter>(GetOwner());
	if (::IsValid(OwnerCharacter) == false || ItemCount <= 0)
	{
		return false;
	}

	USkillComponent* SkillComponent = OwnerCharacter->GetSkillComponent();
	if (::IsValid(SkillComponent) == false)
	{
		return false;
	}

	if (ItemKey == InventoryCoinItemKey)
	{
		return SkillComponent->TryAddCoin(ItemCount);
	}

	if (ItemKey == InventoryOxygenItemKey)
	{
		return SkillComponent->TryAddOxygen(ItemCount);
	}

	return false;
}

void UInventoryComponent::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (::IsValid(OtherActor) == false || OtherActor == GetOwner())
	{
		return;
	}

	AItemBase* GainableItem = Cast<AItemBase>(OtherActor);
	if (::IsValid(GainableItem) == false)
	{
		return;
	}

	const bool bCanCollectItem = CanCollectWorldItem(GainableItem);
	if (GainableItem->IsOccupied() == false && bCanCollectItem)
	{
		GainableItem->TryLoot(Cast<AMKCharacter>(GetOwner()));
	}
}
