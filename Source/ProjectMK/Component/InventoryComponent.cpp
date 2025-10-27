// LINK

#include "ProjectMK/Component/InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "Kismet/KismetSystemLibrary.h"

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

	OnComponentBeginOverlap.AddDynamic(this, &UInventoryComponent::OnSphereOverlap);
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ItemCollectRange <= 0.f)
	{
		return;
	}

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(GetOwner());

	TArray<AActor*> OverlappedActors;
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		GetOwner()->GetActorLocation(),
		ItemCollectRange,
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

void UInventoryComponent::GainItem(FName ItemUID, int32 ItemCount)
{
	if (CanGainItem(ItemUID, ItemCount))
	{
		ensure(false);
		return;
	}

	InventoryItemMap.FindOrAdd(ItemUID) += ItemCount;

	OnInventoryUpdated();
}

void UInventoryComponent::SpendItem(FName ItemUID, int32 ItemCount)
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
