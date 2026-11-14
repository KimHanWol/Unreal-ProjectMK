#include "ProjectMK/Core/Subsystem/ShopTradeSubsystem.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"

void UShopTradeSubsystem::OnWorldBeginPlay(UWorld& World)
{
	Super::OnWorldBeginPlay(World);
	World.GetTimerManager().SetTimer(ExpirationTimer, this, &UShopTradeSubsystem::RemoveExpiredEntries, 1.f, true);
}

void UShopTradeSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld()) { World->GetTimerManager().ClearTimer(ExpirationTimer); }
	Super::Deinitialize();
}

bool UShopTradeSubsystem::TrySell(AMKCharacter* Player, EItemContainer Container, int32 SlotIndex, int32 Count)
{
	if (::IsValid(Player) == false || bTransactionActive) { return false; }
	UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
	const TArray<FInventorySlotData>& Slots = Inventory->GetItemSlots(Container);
	if (Slots.IsValidIndex(SlotIndex) == false || Count <= 0 || Count > Slots[SlotIndex].ItemCount) { return false; }
	FShopBuybackEntry Entry;
	Entry.Id = FGuid::NewGuid();
	Entry.ItemKey = Slots[SlotIndex].ItemUID;
	Entry.Count = Count;
	Entry.UnitPrice = Inventory->CalculateSellPrice(Entry.ItemKey, 1);
	if (Entry.UnitPrice <= 0 || static_cast<int64>(Entry.UnitPrice) * Count > MAX_int32) { return false; }
	TGuardValue<bool> Guard(bTransactionActive, true);
	if (Inventory->TrySellItemSlot(Container, SlotIndex, Count) == false) { return false; }
	const UGameSettingDataAsset* Settings = UDataManager::Get(this)->GetGameSettingDataAsset();
	Entry.ExpiresAt = GetWorld()->GetTimeSeconds() + FMath::Max(1.f, Settings->ShopBuybackLifetime);
	Entries.Insert(Entry, 0);
	if (Entries.Num() > FMath::Max(1, Settings->ShopBuybackMaxEntries)) { Entries.SetNum(FMath::Max(1, Settings->ShopBuybackMaxEntries)); }
	OnStockChanged.Broadcast();
	Player->TrySaveGame();
	return true;
}

bool UShopTradeSubsystem::TryBuyback(AMKCharacter* Player, FGuid Id, int32 Count)
{
	if (bTransactionActive || Count <= 0 || Count > GetMaximumBuybackCount(Player, Id)) { return false; }
	const int32 Index = Entries.IndexOfByPredicate([Id](const FShopBuybackEntry& Entry) { return Entry.Id == Id; });
	if (Index == INDEX_NONE) { return false; }
	TGuardValue<bool> Guard(bTransactionActive, true);
	const FShopBuybackEntry Entry = Entries[Index];
	UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	const float Before = ASC->GetNumericAttribute(UAttributeSet_Character::GetCoinAttribute());
	ASC->SetNumericAttributeBase(UAttributeSet_Character::GetCoinAttribute(), Before - Entry.UnitPrice * Count);
	if (Inventory->AddItem(Entry.ItemKey, Count) == false)
	{
		ASC->SetNumericAttributeBase(UAttributeSet_Character::GetCoinAttribute(), Before);
		return false;
	}
	Entries[Index].Count -= Count;
	if (Entries[Index].Count == 0) { Entries.RemoveAt(Index); }
	OnStockChanged.Broadcast();
	Player->TrySaveGame();
	return true;
}

const TArray<FShopBuybackEntry>& UShopTradeSubsystem::GetBuybackEntries() const
{
	return Entries;
}

const FShopBuybackEntry* UShopTradeSubsystem::FindBuyback(FGuid Id) const
{
	return Entries.FindByPredicate([this, Id](const FShopBuybackEntry& Entry) { return Entry.Id == Id && Entry.ExpiresAt > GetWorld()->GetTimeSeconds(); });
}

int32 UShopTradeSubsystem::GetMaximumBuybackCount(AMKCharacter* Player, FGuid Id) const
{
	const FShopBuybackEntry* Entry = FindBuyback(Id);
	if (::IsValid(Player) == false || Entry == nullptr || Entry->UnitPrice <= 0) { return 0; }
	UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
	const float Coin = Player->GetAbilitySystemComponent()->GetNumericAttribute(UAttributeSet_Character::GetCoinAttribute());
	int32 Count = FMath::Min(Entry->Count, FMath::FloorToInt(Coin / Entry->UnitPrice));
	while (Count > 0 && Inventory->CanGainItem(Entry->ItemKey, Count) == false) { --Count; }
	return Count;
}

void UShopTradeSubsystem::RemoveExpiredEntries()
{
	if (bTransactionActive) { return; }
	const double Now = GetWorld()->GetTimeSeconds();
	if (Entries.RemoveAll([Now](const FShopBuybackEntry& Entry) { return Entry.ExpiresAt <= Now; }) > 0) { OnStockChanged.Broadcast(); }
}
