#include "ProjectMK/Actor/Shop/StorageChestActor.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "TimerManager.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/UI/HUDWidget.h"

bool AStorageChestActor::CanInteract(AActor* Interactor)
{
	return bIsBuried == false && bIsBreaking == false && Super::CanInteract(Interactor);
}

void AStorageChestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(CoverTimer);
	Super::EndPlay(EndPlayReason);
}

bool AStorageChestActor::OnInteract(AActor* Interactor)
{
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UHUDWidget::StaticClass(), false);
	return Widgets.Num() > 0 && CastChecked<UHUDWidget>(Widgets[0])->OpenStorageChest(this);
}

void AStorageChestActor::InitializeChest(FGuid Id, FName ItemKey, bool bBuried)
{
	ChestId = Id;
	GetWorldTimerManager().ClearTimer(CoverTimer);
	ChestItemKey = ItemKey;
	ItemSlots.SetNum(SlotCount);
	const FItemDataTableRow* Item = UDataManager::Get(this)->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemKey);
	UPaperSprite* Sprite = Item != nullptr ? Item->ItemIcon.LoadSynchronous() : nullptr;
	ShopSprite->SetSprite(Sprite);
	if (Sprite != nullptr) { ShopSprite->SetRelativeLocation(FVector(0.f, -12.f, -8.f + Sprite->GetSourceSize().Y / Sprite->GetPixelsPerUnrealUnit() * 0.5f)); }
	InteractionBox->SetBoxExtent(FVector(32.f, 32.f, 24.f));
	bIsBuried = bBuried;
	SetActorHiddenInGame(bBuried);
	if (bBuried) { GetWorldTimerManager().SetTimer(CoverTimer, this, &AStorageChestActor::UpdateBuriedCover, 0.2f, true); }
}

void AStorageChestActor::ApplySavedContents(const TArray<FInventorySlotData>& Contents)
{
	ItemSlots = Contents;
	ItemSlots.SetNum(SlotCount);
	for (FInventorySlotData& Slot : ItemSlots)
	{
		const FItemDataTableRow* Item = Slot.IsEmpty() == false ? UDataManager::Get(this)->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, Slot.ItemUID) : nullptr;
		if (Item == nullptr || Item->bIsInventoryItem == false) { Slot.Clear(); }
		else { Slot.ItemCount = FMath::Clamp(Slot.ItemCount, 1, FMath::Max(1, Item->MaxStackCount)); }
	}
	NotifyContentsChanged();
}

void AStorageChestActor::RevealChest()
{
	if (bIsBuried == false) { return; }
	bIsBuried = false;
	SetActorHiddenInGame(false);
	GetWorldTimerManager().ClearTimer(CoverTimer);
	NotifyContentsChanged();
	if (AMKCharacter* Player = Cast<AMKCharacter>(GetWorld()->GetFirstPlayerController() != nullptr ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr)) { Player->TrySaveGame(); }
}

bool AStorageChestActor::TryBreakByExplosion()
{
	if (bIsBreaking) { return false; }
	TArray<FInventorySlotData> Drops = ItemSlots;
	FInventorySlotData ChestDrop;
	ChestDrop.ItemUID = ChestItemKey;
	ChestDrop.ItemCount = 1;
	Drops.Add(ChestDrop);
	TArray<AItemBase*> Spawned;
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const FInventorySlotData& Slot : Drops)
	{
		if (Slot.IsEmpty()) { continue; }
		const FVector Offset((Spawned.Num() % 5 - 2) * 3.f, 0.f, 3.f + Spawned.Num() / 5 * 2.f);
		AItemBase* Drop = GetWorld()->SpawnActor<AItemBase>(AItemBase::StaticClass(), GetActorLocation() + Offset, FRotator::ZeroRotator, Parameters);
		if (Drop != nullptr) { Drop->InitializeItemBase(Slot.ItemUID, Slot.ItemCount); Spawned.Add(Drop); }
		if (Drop == nullptr || Drop->IsInitialized() == false)
		{
			for (AItemBase* Staged : Spawned) { Staged->Destroy(); }
			return false;
		}
	}
	bIsBreaking = true;
	ItemSlots.Empty();
	Destroy();
	return true;
}

void AStorageChestActor::NotifyContentsChanged() { OnContentsChanged.Broadcast(); }
const TArray<FInventorySlotData>& AStorageChestActor::GetItemSlots() const { return ItemSlots; }

FStorageChestSaveData AStorageChestActor::GetSaveData() const
{
	FStorageChestSaveData Data;
	Data.Id = ChestId;
	Data.Location = GetActorLocation();
	Data.ItemKey = ChestItemKey;
	Data.bBuried = bIsBuried;
	Data.Slots = ItemSlots;
	return Data;
}

FGuid AStorageChestActor::GetChestId() const { return ChestId; }
bool AStorageChestActor::IsBuried() const { return bIsBuried; }

void AStorageChestActor::UpdateBuriedCover()
{
	const ABlockBase* Cover = GetWorld()->GetSubsystem<ULevelManagerSubsystem>()->GetBlockAtWorldLocation(GetActorLocation());
	const FVector Offset = Cover != nullptr ? Cover->GetActorLocation() - GetActorLocation() : FVector::ZeroVector;
	if (Cover == nullptr || Cover->IsBreaking() || FMath::Square(Offset.X) + FMath::Square(Offset.Z) > FMath::Square(12.f)) { RevealChest(); }
}
