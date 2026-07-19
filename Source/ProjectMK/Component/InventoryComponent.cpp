// LINK

#include "ProjectMK/Component/InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Data/DataTable/ShopRecipeDataTableRow.h"

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
	if (InventoryItemMap.Contains(ItemUID) == false && InventoryItemMap.Num() >= GetMaxInventoryCount())
	{
		return false;
	}

	return true;
}

void UInventoryComponent::SetGainRadius(float NewRadius)
{
	SetSphereRadius(NewRadius);
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

	if (SimulatedInventory.Contains(ShopRecipeData.GetResultItemKey()) == false && SimulatedInventory.Num() >= GetMaxInventoryCount())
	{
		return false;
	}

	return true;
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

	if (GainableItem->IsOccupied() == false && CanGainItem(GainableItem->GetItemKey(), 1))
	{
		GainableItem->TryLoot(Cast<AMKCharacter>(GetOwner()));
	}
}
