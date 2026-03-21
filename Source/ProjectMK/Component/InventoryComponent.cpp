// LINK

#include "ProjectMK/Component/InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/ShopRecipeDataTableRow.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (::IsValid(Owner) == false)
	{
		return;
	}

	UAbilitySystemComponent* OwnerASC = Owner->GetComponentByClass<UAbilitySystemComponent>();
	if (::IsValid(OwnerASC) == false)
	{
		return;
	}

	const UAttributeSet_Character* AttributeSet_Character = Cast<UAttributeSet_Character>(OwnerASC->GetAttributeSet(UAttributeSet_Character::StaticClass()));
	if (::IsValid(AttributeSet_Character) == false)
	{
		return;
	}

	ItemCollectRange = AttributeSet_Character->GetItemCollectRange();
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

		if (OverlappedItem->IsInitialized() == false)
		{
			continue;
		}

		const FName& ItemKey = OverlappedItem->GetItemKey();
		GainItem(ItemKey, 1);
		OverlappedItem->OnLootFinished();
	}
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

bool UInventoryComponent::IsItemEquipped(FName ItemUID) const
{
	if (ItemUID.IsNone())
	{
		return false;
	}

	for (const TPair<EEuipmentType, FName>& EquippedItem : EquipmentItemMap)
	{
		if (EquippedItem.Value == ItemUID)
		{
			return true;
		}
	}

	return false;
}

FName UInventoryComponent::GetEquippedItem(EEuipmentType EquipmentType) const
{
	if (const FName* EquippedItemPtr = EquipmentItemMap.Find(EquipmentType))
	{
		return *EquippedItemPtr;
	}

	return NAME_None;
}

void UInventoryComponent::AddItemOrder(FName ItemUID)
{
	if (ItemUID.IsNone() || InventoryItemOrder.Contains(ItemUID))
	{
		return;
	}

	InventoryItemOrder.Add(ItemUID);
}

void UInventoryComponent::RemoveItemOrder(FName ItemUID)
{
	if (ItemUID.IsNone())
	{
		return;
	}

	InventoryItemOrder.Remove(ItemUID);
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
		RemoveItemOrder(ItemUID);
	}

	OnInventoryUpdated();
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

bool UInventoryComponent::CanGainItem(FName ItemUID, int32 ItemCount)
{
	if (InventoryItemMap.Contains(ItemUID) == false && InventoryItemMap.Num() >= MaxInventoryCount)
	{
		return false;
	}

	return true;
}

void UInventoryComponent::SetGainRadius(float NewRadius)
{
	SetSphereRadius(NewRadius);
}

bool UInventoryComponent::EquipItem(FName EquipmentKey)
{
	if (EquipmentKey.IsNone())
	{
		return false;
	}

	const int32* ItemCountPtr = InventoryItemMap.Find(EquipmentKey);
	if (ItemCountPtr == nullptr || (*ItemCountPtr) <= 0)
	{
		return false;
	}

	const FEquipmentItemDataTableRow* EquipmentItemDataTableRow = GetEquipmentItemData(EquipmentKey);
	if (EquipmentItemDataTableRow == nullptr)
	{
		return false;
	}

	if (GetEquippedItem(EquipmentItemDataTableRow->EquipmentType) == EquipmentKey)
	{
		return true;
	}

	const FName EquippedItemKey = GetEquippedItem(EquipmentItemDataTableRow->EquipmentType);
	if (EquippedItemKey.IsNone() == false && UnEquipItem(EquippedItemKey) == false)
	{
		return false;
	}

	if (ApplyEquipmentEffects(EquipmentKey, EquipmentItemDataTableRow->EqiupEffectClasses) == false)
	{
		return false;
	}

	EquipmentItemMap.FindOrAdd(EquipmentItemDataTableRow->EquipmentType) = EquipmentKey;
	OnInventoryUpdated();

	return true;
}

bool UInventoryComponent::UnEquipItem(FName ItemUID)
{
	if (ItemUID.IsNone())
	{
		return false;
	}

	const FEquipmentItemDataTableRow* EquipmentItemDataTableRow = GetEquipmentItemData(ItemUID);
	if (EquipmentItemDataTableRow == nullptr)
	{
		return false;
	}

	FName* EquippedItemPtr = EquipmentItemMap.Find(EquipmentItemDataTableRow->EquipmentType);
	if (EquippedItemPtr == nullptr || (*EquippedItemPtr) != ItemUID)
	{
		return false;
	}

	RemoveEquipmentEffects(ItemUID);
	EquipmentItemMap.Remove(EquipmentItemDataTableRow->EquipmentType);
	OnInventoryUpdated();

	return true;
}

bool UInventoryComponent::CraftShopRecipe(const FShopRecipeDataTableRow& ShopRecipeData)
{
	if (CanCraftShopRecipe(ShopRecipeData) == false)
	{
		return false;
	}

	for (const FShopRecipeItem& RequiredItem : ShopRecipeData.RequiredItems)
	{
		InventoryItemMap.FindOrAdd(RequiredItem.GetItemKey()) -= RequiredItem.ItemCount;
	}

	for (const FShopRecipeItem& RequiredItem : ShopRecipeData.RequiredItems)
	{
		const FName RequiredItemKey = RequiredItem.GetItemKey();
		const int32* InventoryItemCountPtr = InventoryItemMap.Find(RequiredItemKey);
		if (InventoryItemCountPtr != nullptr && (*InventoryItemCountPtr) <= 0)
		{
			InventoryItemMap.Remove(RequiredItemKey);
			RemoveItemOrder(RequiredItemKey);
		}
	}

	AddItemOrder(ShopRecipeData.GetResultItemKey());
	InventoryItemMap.FindOrAdd(ShopRecipeData.GetResultItemKey()) += 1;
	OnInventoryUpdated();

	return true;
}

bool UInventoryComponent::CanCraftShopRecipe(const FShopRecipeDataTableRow& ShopRecipeData) const
{
	if (ShopRecipeData.GetResultItemKey().IsNone())
	{
		return false;
	}

	TMap<FName, int32> SimulatedInventory = InventoryItemMap;
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

	if (SimulatedInventory.Contains(ShopRecipeData.GetResultItemKey()) == false && SimulatedInventory.Num() >= MaxInventoryCount)
	{
		return false;
	}

	return true;
}

void UInventoryComponent::OnSphereOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
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

	if (GainableItem->IsOccupied() == false && CanGainItem(GainableItem->GetItemKey(), 1))
	{
		GainableItem->TryLoot(Cast<AMKCharacter>(GetOwner()));
	}
}

void UInventoryComponent::GainItem(FName ItemUID, int32 ItemCount)
{
	if (CanGainItem(ItemUID, ItemCount) == false)
	{
		ensure(false);
		return;
	}

	InventoryItemMap.FindOrAdd(ItemUID) += ItemCount;
	AddItemOrder(ItemUID);

	OnInventoryUpdated();
}

bool UInventoryComponent::ApplyEquipmentEffects(FName EquipmentKey, const TArray<FEquipmentEffectEntry>& EffectClasses)
{
	IAbilitySystemInterface* OwnerAbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	if (OwnerAbilitySystemInterface == nullptr)
	{
		return false;
	}

	UAbilitySystemComponent* OwnerASC = OwnerAbilitySystemInterface->GetAbilitySystemComponent();
	if (OwnerASC == nullptr)
	{
		return false;
	}

	TArray<FActiveGameplayEffectHandle> AppliedHandles;
	for (const FEquipmentEffectEntry& EquipEffectEntry : EffectClasses)
	{
		if (::IsValid(EquipEffectEntry.EffectClass) == false)
		{
			continue;
		}

		FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(EquipEffectEntry.EffectClass, 1.f, OwnerASC->MakeEffectContext());
		if (SpecHandle.IsValid() == false)
		{
			continue;
		}

		if (EquipEffectEntry.SetByCallerTag.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(EquipEffectEntry.SetByCallerTag, EquipEffectEntry.SetByCallerValue);
		}

		const FActiveGameplayEffectHandle Handle = OwnerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		if (Handle.WasSuccessfullyApplied())
		{
			AppliedHandles.Add(Handle);
		}
	}

	ActivatedEquipementEffects.FindOrAdd(EquipmentKey) = MoveTemp(AppliedHandles);
	return true;
}

void UInventoryComponent::RemoveEquipmentEffects(FName EquipmentKey)
{
	IAbilitySystemInterface* OwnerAbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	if (OwnerAbilitySystemInterface == nullptr)
	{
		return;
	}

	UAbilitySystemComponent* OwnerASC = OwnerAbilitySystemInterface->GetAbilitySystemComponent();
	if (OwnerASC == nullptr)
	{
		return;
	}

	TArray<FActiveGameplayEffectHandle>* ActiveEffectHandlePtr = ActivatedEquipementEffects.Find(EquipmentKey);
	if (ActiveEffectHandlePtr == nullptr)
	{
		return;
	}

	for (const FActiveGameplayEffectHandle& ActiveEffectHandle : (*ActiveEffectHandlePtr))
	{
		if (ActiveEffectHandle.IsValid())
		{
			OwnerASC->RemoveActiveGameplayEffect(ActiveEffectHandle);
		}
	}

	ActivatedEquipementEffects.Remove(EquipmentKey);
}

const FEquipmentItemDataTableRow* UInventoryComponent::GetEquipmentItemData(FName EquipmentKey) const
{
	UDataManager* DataManager = UDataManager::Get(const_cast<UInventoryComponent*>(this));
	if (::IsValid(DataManager) == false)
	{
		return nullptr;
	}

	return DataManager->GetDataTableRow<FEquipmentItemDataTableRow>(EDataTableType::EquipmentItem, EquipmentKey);
}

void UInventoryComponent::SpendItem(FName ItemUID, int32 ItemCount)
{
	InventoryItemMap.FindOrAdd(ItemUID) -= ItemCount;
	
	if (InventoryItemMap[ItemUID] == 0)
	{
		InventoryItemMap.Remove(ItemUID);
		RemoveItemOrder(ItemUID);
	}

	OnInventoryUpdated();
}

void UInventoryComponent::OnInventoryUpdated()
{
	OnInventoryChangedDelegate.Broadcast();
}
