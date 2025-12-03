// LINK

#include "ProjectMK/Component/InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"

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

bool UInventoryComponent::EquipItem(FName EquipmentKey)
{
	int32 *bItemCountPtr = InventoryItemMap.Find(EquipmentKey);
	if (bItemCountPtr == nullptr || (*bItemCountPtr) > 0)
	{
		return false;
	}

	(*bItemCountPtr) = true;

	UDataManager *DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return false;
	}

	const FEquipmentItemDataTableRow* EquipmentItemDataTableRow = DataManager->GetDataTableRow<FEquipmentItemDataTableRow>(EDataTableType::EquipmentItem, EquipmentKey);
	if (EquipmentItemDataTableRow == nullptr)
	{
		return false;
	}

	IAbilitySystemInterface* OwnerAbilitySystemInterface = Cast <IAbilitySystemInterface>(GetOwner());
	if (OwnerAbilitySystemInterface == nullptr)
	{
		return false;
	}

	UAbilitySystemComponent* OwnerASC = OwnerAbilitySystemInterface->GetAbilitySystemComponent();
	if(OwnerASC == nullptr)
	{
		return false;
	}

	FName *EquipmentNamePtr = EquipmentItemMap.Find(EquipmentItemDataTableRow->EquipmentType);
	if (EquipmentNamePtr)
	{
		UnEquipItem(*EquipmentNamePtr);
	}

	for (const auto& EquipEffectClass : EquipmentItemDataTableRow->EqiupEffectClasses)
	{
		FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(EquipEffectClass, 1.f, OwnerASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			FActiveGameplayEffectHandle Handle = OwnerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			ActivatedEquipementEffects.FindOrAdd(EquipmentKey).Add(Handle);
		}
	}

	(*EquipmentNamePtr) = EquipmentKey;

	return true;
}

bool UInventoryComponent::UnEquipItem(FName ItemUID)
{
	IAbilitySystemInterface *OwnerAbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	if (OwnerAbilitySystemInterface == nullptr)
	{
		return false;
	}

	UAbilitySystemComponent *OwnerASC = OwnerAbilitySystemInterface->GetAbilitySystemComponent();
	if (OwnerASC == nullptr)
	{
		return false;
	}

	TArray<FActiveGameplayEffectHandle>* ActiveEffectHandlePtr = ActivatedEquipementEffects.Find(ItemUID);
	if (ActiveEffectHandlePtr == nullptr)
	{
		return false;
	}

	for(const auto& ActiveEffectHandle : (*ActiveEffectHandlePtr))
	{
		OwnerASC->RemoveActiveGameplayEffect(ActiveEffectHandle);
	}

	ActivatedEquipementEffects.Remove(ItemUID);

	return true;
}

bool UInventoryComponent::CraftEquipmentItem(FName EquipmentItemKey)
{
	if (IsCraftable(EquipmentItemKey) == false)
	{
		return false;
	}

	const UDataManager *DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return false;
	}

	const FEquipmentItemDataTableRow *EquipmentItemDataTableRow = DataManager->GetDataTableRow<FEquipmentItemDataTableRow>(EDataTableType::EquipmentItem, EquipmentItemKey);
	if (EquipmentItemDataTableRow == nullptr)
	{
		return false;
	}

	for (const auto &CraftRecipeMaterial : EquipmentItemDataTableRow->CraftRecipe)
	{
		int32* InventoryItemCountPtr = InventoryItemMap.Find(CraftRecipeMaterial.MaterialKey);
		(*InventoryItemCountPtr) -= CraftRecipeMaterial.MaterialCount;
	}

	InventoryItemMap.FindOrAdd(EquipmentItemKey) += 1;
	return true;
}

bool UInventoryComponent::IsCraftable(FName EquipmentItemKey)
{
	if(GetItemCount(EquipmentItemKey) > 0)
	{
		return false;
	}

	UDataManager *DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return false;
	}

	const FEquipmentItemDataTableRow *EquipmentItemDataTableRow = DataManager->GetDataTableRow<FEquipmentItemDataTableRow>(EDataTableType::EquipmentItem, EquipmentItemKey);
	if (EquipmentItemDataTableRow == nullptr)
	{
		return false;
	}

	for (const auto &CraftRecipeMaterial : EquipmentItemDataTableRow->CraftRecipe)
	{
		const int32 *InventoryItemCountPtr = InventoryItemMap.Find(CraftRecipeMaterial.MaterialKey);
		if (InventoryItemCountPtr == nullptr || (*InventoryItemCountPtr) < CraftRecipeMaterial.MaterialCount)
		{
			return false;
		}
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
	OnInventoryChangedDelegate.Broadcast();
}
