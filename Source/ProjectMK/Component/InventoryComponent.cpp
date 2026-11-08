// LINK

#include "ProjectMK/Component/InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Manager/SoundManager.h"
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
	SetHiddenInGame(true);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	SetHiddenInGame(true);

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

	if (bIsItemCollectionActive == false || ItemGainRange <= 0.f)
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

float UInventoryComponent::GetItemGainRange() const
{
	return ItemGainRange;
}

bool UInventoryComponent::IsItemCollectionActive() const
{
	return bIsItemCollectionActive;
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

bool UInventoryComponent::TryRemoveItem(FName ItemUID, int32 ItemCount)
{
	if (ItemUID.IsNone() || ItemCount <= 0 || GetItemCount(ItemUID) < ItemCount)
	{
		return false;
	}

	SpendItem(ItemUID, ItemCount);
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

void UInventoryComponent::ApplySavedInventorySlots(const TArray<FInventorySlotData>& SavedSlotDataList)
{
	InventorySlotDataList = SavedSlotDataList;
	OnInventoryUpdated();
}

void UInventoryComponent::ApplySavedQuickSlots(const TArray<FInventorySlotData>& SavedSlots)
{
	QuickSlotDataList = SavedSlots;
	OnInventoryUpdated();
}

bool UInventoryComponent::CanGainItem(FName ItemUID, int32 ItemCount)
{
	const_cast<UInventoryComponent*>(this)->UpdateInventorySlotDataList();
	return CalculateAvailableItemCapacity(ItemUID) >= ItemCount;
}

bool UInventoryComponent::TryCollectWorldItem(AItemBase* ItemActor)
{
	if (bIsItemCollectionActive == false || ::IsValid(ItemActor) == false)
	{
		return false;
	}

	const FName ItemKey = ItemActor->GetItemKey();
	if (ItemActor->CanBeCollectedBy(GetOwner()) == false) { return false; }
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
			TryPlayInventoryFullSFX();
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
	USoundManager::TryPlaySFX(GetOwner(), ESFXType::Item_Pickup);
	return true;
}

bool UInventoryComponent::TryMoveItemSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, EItemContainer SourceContainer, EItemContainer TargetContainer)
{
	UpdateInventorySlotDataList();
	TArray<FInventorySlotData>& SourceSlots = SourceContainer == EItemContainer::QuickSlot ? QuickSlotDataList : InventorySlotDataList;
	TArray<FInventorySlotData>& TargetSlots = TargetContainer == EItemContainer::QuickSlot ? QuickSlotDataList : InventorySlotDataList;
	if (SourceSlots.IsValidIndex(SourceSlotIndex) == false || TargetSlots.IsValidIndex(TargetSlotIndex) == false
		|| (SourceContainer == TargetContainer && SourceSlotIndex == TargetSlotIndex)) { return false; }
	FInventorySlotData& Source = SourceSlots[SourceSlotIndex];
	FInventorySlotData& Target = TargetSlots[TargetSlotIndex];
	if (Source.IsEmpty() || (TargetContainer == EItemContainer::QuickSlot && CanEquipQuickSlot(Source.ItemUID) == false)
		|| (SourceContainer == EItemContainer::QuickSlot && Target.IsEmpty() == false && CanEquipQuickSlot(Target.ItemUID) == false)) { return false; }
	if (Target.IsEmpty())
	{
		Target = Source;
		Source.Clear();
	}
	else if (Source.ItemUID == Target.ItemUID)
	{
		const int32 Count = FMath::Min(Source.ItemCount, GetItemMaxStackCount(Source.ItemUID) - Target.ItemCount);
		if (Count <= 0) { return false; }
		Target.ItemCount += Count;
		Source.ItemCount -= Count;
		if (Source.ItemCount <= 0) { Source.Clear(); }
	}
	else { Swap(Source, Target); }
	OnInventoryUpdated();
	return true;
}

bool UInventoryComponent::TryRemoveItemFromSlot(EItemContainer Container, int32 SlotIndex, int32 Count)
{
	UpdateInventorySlotDataList();
	TArray<FInventorySlotData>& Slots = Container == EItemContainer::QuickSlot ? QuickSlotDataList : InventorySlotDataList;
	if (Slots.IsValidIndex(SlotIndex) == false || Count <= 0 || Slots[SlotIndex].ItemCount < Count) { return false; }
	Slots[SlotIndex].ItemCount -= Count;
	if (Slots[SlotIndex].ItemCount <= 0) { Slots[SlotIndex].Clear(); }
	OnInventoryUpdated();
	return true;
}

bool UInventoryComponent::TryDropItemSlot(EItemContainer Container, int32 SlotIndex, int32 Count)
{
	const TArray<FInventorySlotData>& Slots = GetItemSlots(Container);
	AMKCharacter* Character = Cast<AMKCharacter>(GetOwner());
	if (::IsValid(Character) == false || Slots.IsValidIndex(SlotIndex) == false || Slots[SlotIndex].IsEmpty()
		|| Count <= 0 || Count > Slots[SlotIndex].ItemCount) { return false; }
	const FName Key = Slots[SlotIndex].ItemUID;
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AItemBase* Item = GetWorld()->SpawnActor<AItemBase>(AItemBase::StaticClass(), Character->GetActorLocation(), FRotator::ZeroRotator, Parameters);
	if (::IsValid(Item) == false) { return false; }
	Item->InitializeItemBase(Key, Count);
	Item->SetDropOwner(Character);
	if (Item->IsInitialized() == false || TryRemoveItemFromSlot(Container, SlotIndex, Count) == false)
	{
		Item->Destroy();
		return false;
	}
	Character->TrySaveGame();
	return true;
}

bool UInventoryComponent::CanEquipQuickSlot(FName ItemKey) const
{
	const UDataManager* Data = UDataManager::Get(const_cast<UInventoryComponent*>(this));
	const FItemDataTableRow* Item = ::IsValid(Data) && ItemKey.IsNone() == false
		? Data->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemKey) : nullptr;
	return Item != nullptr && Item->bIsInventoryItem && Item->bIsOre == false
		&& Item->ConsumableEffect != EConsumableEffectType::None && Item->ConsumableEffect != EConsumableEffectType::Insurance;
}

const TArray<FInventorySlotData>& UInventoryComponent::GetItemSlots(EItemContainer Container)
{
	UpdateInventorySlotDataList();
	return Container == EItemContainer::QuickSlot ? QuickSlotDataList : InventorySlotDataList;
}

void UInventoryComponent::SetGainRadius(float NewRadius)
{
	ItemGainRange = FMath::Max(0.f, NewRadius);
	SetSphereRadius(ItemGainRange);
}

void UInventoryComponent::SetItemCollectionActive(bool bActive)
{
	bIsItemCollectionActive = bActive;
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
	AddItem(ShopRecipeData.GetResultItemKey(), FMath::Max(1, ShopRecipeData.ResultItemCount));

	return true;
}

bool UInventoryComponent::CanCraftShopRecipe(const FShopRecipeDataTableRow& Recipe) const
{
	const_cast<UInventoryComponent*>(this)->UpdateInventorySlotDataList();
	if (Recipe.GetResultItemKey().IsNone()) { return false; }
	TArray<FInventorySlotData> Inventory = InventorySlotDataList;
	TArray<FInventorySlotData> Quick = QuickSlotDataList;
	for (const FShopRecipeItem& Required : Recipe.RequiredItems)
	{
		if (Required.GetItemKey().IsNone() || Required.ItemCount <= 0) { return false; }
		int32 Remaining = Required.ItemCount;
		for (TArray<FInventorySlotData>* Slots : { &Inventory, &Quick })
		{
			for (FInventorySlotData& Slot : *Slots)
			{
				if (Slot.ItemUID != Required.GetItemKey()) { continue; }
				const int32 Used = FMath::Min(Remaining, Slot.ItemCount);
				Slot.ItemCount -= Used;
				Remaining -= Used;
				if (Slot.ItemCount <= 0) { Slot.Clear(); }
				if (Remaining == 0) { break; }
			}
			if (Remaining == 0) { break; }
		}
		if (Remaining > 0) { return false; }
	}
	const int32 Maximum = GetItemMaxStackCount(Recipe.GetResultItemKey());
	int32 Capacity = 0;
	for (const FInventorySlotData& Slot : Inventory)
	{
		if (Slot.IsEmpty()) { Capacity += Maximum; }
		else if (Slot.ItemUID == Recipe.GetResultItemKey()) { Capacity += FMath::Max(0, Maximum - Slot.ItemCount); }
	}
	return Capacity >= FMath::Max(1, Recipe.ResultItemCount);
}

bool UInventoryComponent::TrySellItem(FName ItemUID, int32 ItemCount)
{
	const int32 SellItemCount = FMath::Min(ItemCount, GetItemCount(ItemUID));
	const int32 SellPrice = CalculateSellPrice(ItemUID, SellItemCount);
	if (SellPrice <= 0)
	{
		return false;
	}

	AMKCharacter* OwnerCharacter = Cast<AMKCharacter>(GetOwner());
	USkillComponent* SkillComponent = ::IsValid(OwnerCharacter) ? OwnerCharacter->GetSkillComponent() : nullptr;
	if (::IsValid(SkillComponent) == false)
	{
		return false;
	}

	SpendItem(ItemUID, SellItemCount);
	// 코인 획득 배율은 채굴 중 코인 드롭량에만 적용하고 판매 대금에는 적용하지 않는다.
	SkillComponent->TryAddCoin(SellPrice, false);
	return true;
}

bool UInventoryComponent::TrySellItemSlot(EItemContainer Container, int32 SlotIndex, int32 Count)
{
	const TArray<FInventorySlotData>& Slots = GetItemSlots(Container);
	if (Slots.IsValidIndex(SlotIndex) == false || Slots[SlotIndex].IsEmpty() || Count <= 0 || Count > Slots[SlotIndex].ItemCount) { return false; }
	const int32 Price = CalculateSellPrice(Slots[SlotIndex].ItemUID, Count);
	AMKCharacter* Character = Cast<AMKCharacter>(GetOwner());
	USkillComponent* Skills = ::IsValid(Character) ? Character->GetSkillComponent() : nullptr;
	if (Price <= 0 || ::IsValid(Skills) == false) { return false; }
	if (TryRemoveItemFromSlot(Container, SlotIndex, Count) == false) { return false; }
	Skills->TryAddCoin(Price, false);
	return true;
}

bool UInventoryComponent::TrySellAllItems()
{
	bool bSoldAnyItem = false;
	for (const TPair<FName, int32>& InventoryItemPair : GetInventoryItems())
	{
		bSoldAnyItem |= TrySellItem(InventoryItemPair.Key, InventoryItemPair.Value);
	}

	return bSoldAnyItem;
}

int32 UInventoryComponent::CalculateSellPrice(FName ItemUID, int32 ItemCount) const
{
	if (ItemUID.IsNone() || ItemCount <= 0)
	{
		return 0;
	}

	UDataManager* DataManager = UDataManager::Get(const_cast<UInventoryComponent*>(this));
	if (::IsValid(DataManager) == false)
	{
		return 0;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemUID);
	// SellPrice 가 0 인 아이템(소모품 등)은 판매하지 않는다.
	if (ItemDataTableRow == nullptr)
	{
		return 0;
	}

	return FMath::Max(0, ItemDataTableRow->SellPrice) * ItemCount;
}

const TArray<FInventorySlotData>& UInventoryComponent::GetInventorySlotDataList()
{
	UpdateInventorySlotDataList();
	return InventorySlotDataList;
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
	if (ItemUID.IsNone() || ItemCount <= 0) { return; }
	UpdateInventorySlotDataList();
	int32 Remaining = ItemCount;
	for (TArray<FInventorySlotData>* Slots : { &InventorySlotDataList, &QuickSlotDataList })
	{
		for (FInventorySlotData& Slot : *Slots)
		{
			if (Slot.ItemUID != ItemUID || Slot.IsEmpty()) { continue; }
			const int32 Spent = FMath::Min(Remaining, Slot.ItemCount);
			Slot.ItemCount -= Spent;
			Remaining -= Spent;
			if (Slot.ItemCount <= 0) { Slot.Clear(); }
			if (Remaining <= 0) { break; }
		}
		if (Remaining <= 0) { break; }
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

void UInventoryComponent::UpdateInventorySlotDataList()
{
	QuickSlotDataList.SetNum(QuickSlotCount);
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

void UInventoryComponent::UpdateInventoryItemMap()
{
	InventoryItemMap.Reset();
	for (const TArray<FInventorySlotData>* Slots : { &InventorySlotDataList, &QuickSlotDataList })
	{
		for (const FInventorySlotData& Slot : *Slots)
		{
			if (Slot.IsEmpty() == false) { InventoryItemMap.FindOrAdd(Slot.ItemUID) += Slot.ItemCount; }
		}
	}
}

void UInventoryComponent::NormalizeInventorySlotDataList()
{
	// Slot moves must preserve the player's arrangement; only explicit moves merge stacks.
	for (TArray<FInventorySlotData>* Slots : { &InventorySlotDataList, &QuickSlotDataList })
	{
		for (FInventorySlotData& Slot : *Slots)
		{
			if (Slot.IsEmpty()) { Slot.Clear(); }
		}
	}
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
	if (::IsValid(ItemActor) == false || ItemActor->IsInitialized() == false || ItemActor->CanBeCollectedBy(GetOwner()) == false)
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

void UInventoryComponent::TryPlayInventoryFullSFX()
{
	const UWorld* World = GetWorld();
	if (::IsValid(World) == false)
	{
		return;
	}

	// 자석으로 끌려온 아이템이 매 프레임 수집에 실패할 수 있어 재생 간격을 제한한다.
	const float CurrentTime = World->GetTimeSeconds();
	if (LastInventoryFullSFXTime >= 0.f && CurrentTime - LastInventoryFullSFXTime < InventoryFullSFXCooldown)
	{
		return;
	}

	LastInventoryFullSFXTime = CurrentTime;
	USoundManager::TryPlaySFX(GetOwner(), ESFXType::Inventory_Full);
}

void UInventoryComponent::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsItemCollectionActive == false || ::IsValid(OtherActor) == false || OtherActor == GetOwner())
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
