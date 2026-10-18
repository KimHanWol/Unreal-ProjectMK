#include "ProjectMK/Component/ConsumableComponent.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "PaperSprite.h"
#include "TimerManager.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Spawnable/BlastChargeActor.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Manager/SoundManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"

void UConsumableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRecall();

	Super::EndPlay(EndPlayReason);
}

bool UConsumableComponent::TryUseInventorySlot(int32 SlotIndex)
{
	UInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (::IsValid(InventoryComponent) == false)
	{
		return false;
	}

	const TArray<FInventorySlotData>& SlotDataList = InventoryComponent->GetInventorySlotDataList();
	if (SlotDataList.IsValidIndex(SlotIndex) == false || SlotDataList[SlotIndex].IsEmpty())
	{
		return false;
	}

	return TryUseItem(SlotDataList[SlotIndex].ItemUID);
}

bool UConsumableComponent::TryUseItem(FName ItemUID)
{
	UInventoryComponent* InventoryComponent = GetInventoryComponent();
	const UDataManager* DataManager = UDataManager::Get(this);
	if (ItemUID.IsNone() || ::IsValid(InventoryComponent) == false || ::IsValid(DataManager) == false || InventoryComponent->GetItemCount(ItemUID) <= 0)
	{
		return false;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemUID);
	if (ItemDataTableRow == nullptr || ItemDataTableRow->ConsumableEffect == EConsumableEffectType::None)
	{
		return false;
	}

	// 보험 증서는 소지형이라 직접 사용하지 않는다.
	if (ItemDataTableRow->ConsumableEffect == EConsumableEffectType::Insurance || bIsRecalling)
	{
		USoundManager::TryPlaySFX(GetOwner(), ESFXType::Item_UseFailed);
		return false;
	}

	if (TryApplyConsumableEffect(ItemUID, *ItemDataTableRow) == false)
	{
		USoundManager::TryPlaySFX(GetOwner(), ESFXType::Item_UseFailed);
		return false;
	}

	// 귀환은 대기 시간이 끝났을 때 소모한다.
	if (ItemDataTableRow->ConsumableEffect != EConsumableEffectType::Recall)
	{
		InventoryComponent->TryRemoveItem(ItemUID, 1);
		OnConsumableUsedDelegate.Broadcast(ItemUID);
	}

	USoundManager::TryPlaySFX(GetOwner(), ESFXType::Item_Use);
	return true;
}

bool UConsumableComponent::TryConsumeInsurance()
{
	UInventoryComponent* InventoryComponent = GetInventoryComponent();
	const UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(InventoryComponent) == false || ::IsValid(DataManager) == false)
	{
		return false;
	}

	for (const TPair<FName, int32>& InventoryItem : InventoryComponent->GetInventoryItems())
	{
		if (InventoryItem.Value <= 0)
		{
			continue;
		}

		const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, InventoryItem.Key);
		if (ItemDataTableRow != nullptr && ItemDataTableRow->ConsumableEffect == EConsumableEffectType::Insurance)
		{
			InventoryComponent->TryRemoveItem(InventoryItem.Key, 1);
			OnConsumableUsedDelegate.Broadcast(InventoryItem.Key);
			return true;
		}
	}

	return false;
}

void UConsumableComponent::ClearRecall()
{
	if (bIsRecalling == false)
	{
		return;
	}

	bIsRecalling = false;
	RecallItemUID = NAME_None;
	RecallRemainingTime = 0.f;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecallTimerHandle);
	}

	USoundManager::TryStopSFX(GetOwner(), ESFXType::Recall_Start);
	OnRecallStateChangedDelegate.Broadcast(false);
}

bool UConsumableComponent::TryApplyConsumableEffect(FName ItemUID, const FItemDataTableRow& ItemDataTableRow)
{
	switch (ItemDataTableRow.ConsumableEffect)
	{
	case EConsumableEffectType::RestoreOxygen:
		return TryRestoreOxygen(ItemDataTableRow.EffectValue);
	case EConsumableEffectType::RestoreHealth:
		return TryRestoreHealth(ItemDataTableRow.EffectValue);
	case EConsumableEffectType::Recall:
		return TryStartRecall(ItemUID, ItemDataTableRow.EffectDuration);
	case EConsumableEffectType::PlaceBlock:
		return TryPlaceBlock(ItemDataTableRow.PlaceBlockKey);
	case EConsumableEffectType::ApplyCushion:
		return TryApplyCushion();
	case EConsumableEffectType::PlaceBlastCharge:
		return TryPlaceBlastCharge(ItemDataTableRow);
	default:
		return false;
	}
}

bool UConsumableComponent::TryRestoreOxygen(float RestoreRatio)
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	UAbilitySystemComponent* AbilitySystemComponent = ::IsValid(OwnerCharacter) ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;
	const UAttributeSet_Character* AttributeSet = ::IsValid(AbilitySystemComponent) ? AbilitySystemComponent->GetSet<UAttributeSet_Character>() : nullptr;
	if (::IsValid(AttributeSet) == false || RestoreRatio <= 0.f || AttributeSet->GetCurrentOxygen() >= AttributeSet->GetMaxOxygen())
	{
		return false;
	}

	FGameplayAbilityUtils::ApplyOxygen(AbilitySystemComponent, AttributeSet->GetMaxOxygen() * RestoreRatio);
	USoundManager::TryPlaySFX(GetOwner(), ESFXType::Oxygen_Refill);
	return true;
}

bool UConsumableComponent::TryRestoreHealth(float RestoreAmount)
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	UAbilitySystemComponent* AbilitySystemComponent = ::IsValid(OwnerCharacter) ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;
	const UAttributeSet_Character* AttributeSet = ::IsValid(AbilitySystemComponent) ? AbilitySystemComponent->GetSet<UAttributeSet_Character>() : nullptr;
	if (::IsValid(AttributeSet) == false || RestoreAmount <= 0.f || AttributeSet->GetCurrentHealth() >= AttributeSet->GetMaxHealth())
	{
		return false;
	}

	FGameplayAbilityUtils::ApplyDamage(AbilitySystemComponent, nullptr, -RestoreAmount);
	return true;
}

bool UConsumableComponent::TryStartRecall(FName ItemUID, float RecallDuration)
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	UWorld* World = GetWorld();
	if (::IsValid(OwnerCharacter) == false || ::IsValid(World) == false)
	{
		return false;
	}

	bIsRecalling = true;
	RecallItemUID = ItemUID;
	RecallStartLocation = OwnerCharacter->GetActorLocation();
	RecallRemainingTime = FMath::Max(0.f, RecallDuration);
	World->GetTimerManager().SetTimer(RecallTimerHandle, this, &UConsumableComponent::UpdateRecall, RecallCheckInterval, true);
	USoundManager::TryPlaySFX(GetOwner(), ESFXType::Recall_Start);
	OnRecallStateChangedDelegate.Broadcast(true);
	return true;
}

bool UConsumableComponent::TryPlaceBlock(FName BlockRowName)
{
	const AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	UWorld* World = GetWorld();
	ULevelManagerSubsystem* LevelManagerSubsystem = ::IsValid(World) ? World->GetSubsystem<ULevelManagerSubsystem>() : nullptr;
	if (BlockRowName.IsNone() || ::IsValid(OwnerCharacter) == false || IsOwnerOnGround() == false || ::IsValid(LevelManagerSubsystem) == false)
	{
		return false;
	}

	// 캐릭터가 서 있는 칸에 설치한다.
	return LevelManagerSubsystem->TryPlaceBlock(BlockRowName, OwnerCharacter->GetActorLocation());
}

bool UConsumableComponent::TryApplyCushion()
{
	const AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	UWorld* World = GetWorld();
	ULevelManagerSubsystem* LevelManagerSubsystem = ::IsValid(World) ? World->GetSubsystem<ULevelManagerSubsystem>() : nullptr;
	if (::IsValid(OwnerCharacter) == false || IsOwnerOnGround() == false || ::IsValid(LevelManagerSubsystem) == false)
	{
		return false;
	}

	// 캐릭터가 밟고 있는 블록에만 바른다.
	ABlockBase* TargetBlock = LevelManagerSubsystem->GetBlockBelowWorldLocation(OwnerCharacter->GetActorLocation());
	if (::IsValid(TargetBlock) == false || TargetBlock->IsMineable() == false || TargetBlock->IsCushioned())
	{
		return false;
	}

	TargetBlock->ApplyCushion();
	return true;
}

bool UConsumableComponent::TryPlaceBlastCharge(const FItemDataTableRow& ItemDataTableRow)
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	UWorld* World = GetWorld();
	if (::IsValid(OwnerCharacter) == false || ::IsValid(World) == false || IsOwnerOnGround() == false)
	{
		return false;
	}

	// 폭약은 캐릭터가 서 있는 칸에 둔다. 주변 반경 안의 블록이 함께 터진다.
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = OwnerCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABlastChargeActor* BlastCharge = World->SpawnActor<ABlastChargeActor>(ABlastChargeActor::StaticClass(), OwnerCharacter->GetActorLocation(), FRotator::ZeroRotator, SpawnParameters);
	if (::IsValid(BlastCharge) == false)
	{
		return false;
	}

	BlastCharge->InitializeBlastCharge(
		ItemDataTableRow.EffectDuration,
		FMath::Max(0, FMath::RoundToInt(ItemDataTableRow.EffectValue)),
		OwnerCharacter->GetAbilitySystemComponent(),
		ItemDataTableRow.ItemIcon.LoadSynchronous());
	return true;
}

void UConsumableComponent::UpdateRecall()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (bIsRecalling == false || ::IsValid(OwnerCharacter) == false)
	{
		ClearRecall();
		return;
	}

	if (FVector::DistSquared(RecallStartLocation, OwnerCharacter->GetActorLocation()) > FMath::Square(RecallCancelMoveDistance))
	{
		ClearRecall();
		USoundManager::TryPlaySFX(GetOwner(), ESFXType::Item_UseFailed);
		return;
	}

	RecallRemainingTime -= RecallCheckInterval;
	if (RecallRemainingTime <= 0.f)
	{
		ApplyRecallComplete();
	}
}

void UConsumableComponent::ApplyRecallComplete()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	UInventoryComponent* InventoryComponent = GetInventoryComponent();
	const FName UsedItemUID = RecallItemUID;
	ClearRecall();

	if (::IsValid(OwnerCharacter) == false || ::IsValid(InventoryComponent) == false || InventoryComponent->TryRemoveItem(UsedItemUID, 1) == false)
	{
		return;
	}

	OnConsumableUsedDelegate.Broadcast(UsedItemUID);
	OwnerCharacter->RespawnAtSpawnLocation();
	USoundManager::TryPlaySFX(GetOwner(), ESFXType::Recall_Complete);
	OwnerCharacter->TrySaveGame();
}

bool UConsumableComponent::IsRecalling() const
{
	return bIsRecalling;
}

float UConsumableComponent::GetRecallRemainingTime() const
{
	return bIsRecalling ? RecallRemainingTime : 0.f;
}

AMKCharacter* UConsumableComponent::GetOwnerCharacter() const
{
	return Cast<AMKCharacter>(GetOwner());
}

UInventoryComponent* UConsumableComponent::GetInventoryComponent() const
{
	const AActor* Owner = GetOwner();
	return ::IsValid(Owner) ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
}

bool UConsumableComponent::IsOwnerOnGround() const
{
	const AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	return ::IsValid(OwnerCharacter) && OwnerCharacter->IsOnGround();
}
