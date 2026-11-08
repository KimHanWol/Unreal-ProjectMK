#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/MKSaveGame.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"

namespace
{
	class FInventoryTransferCommand : public IAutomationLatentCommand
	{
	public:
		explicit FInventoryTransferCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			if (Step == 0)
			{
				UWorld* World = GEditor->PlayWorld;
				Player = ::IsValid(World) ? Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)) : nullptr;
				if (Player.IsValid() == false)
				{
					if (Elapsed() < 15.0) { return false; }
					Test->AddError(TEXT("Inventory fixture could not find the player."));
					return true;
				}
				if (Test->TestTrue(TEXT("Save files are protected"), USaveGameSubsystem::Get(World)->StartPlaytestSession()) == false) { return true; }
				Inventory = Player->FindComponentByClass<UInventoryComponent>();
				SavedInventory = Inventory->GetInventorySlotDataList();
				SavedQuick = Inventory->GetItemSlots(EItemContainer::QuickSlot);
				SavedTransform = Player->GetActorTransform();
				bCollectionWasActive = Inventory->IsItemCollectionActive();
				Inventory->SetItemCollectionActive(false);
				Inventory->ApplySavedInventorySlots({});
				Inventory->ApplySavedQuickSlots({});
				Test->TestNotNull(TEXT("Character has a static representative sprite"), Player->GetPortraitSprite());
				const UDataManager* Data = UDataManager::Get(World);
				for (FName Key : Data->GetDataTable(EDataTableType::Item)->GetRowNames())
				{
					const FItemDataTableRow* Item = Data->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, Key);
					if (Item->ConsumableEffect == EConsumableEffectType::RestoreHealth && Item->MaxStackCount >= 8) { UsableKey = Key; }
					if (Item->bIsOre && Item->bIsInventoryItem) { OreKey = Key; }
				}
				if (UsableKey.IsNone() || OreKey.IsNone()) { Test->AddError(TEXT("Required consumable and ore rows are missing.")); return Finish(); }
				Test->TestEqual(TEXT("Quick storage starts at eight independent slots"), Inventory->GetItemSlots(EItemContainer::QuickSlot).Num(), 8);
				Test->TestTrue(TEXT("Inventory has its own starting capacity"), Inventory->GetInventorySlotDataList().Num() >= 8);
				Inventory->AddItem(UsableKey, 5);
				Inventory->AddItem(OreKey, 3);
				Test->TestTrue(TEXT("Consumable transfers from inventory to quick storage"), Inventory->TryMoveItemSlot(0, 0, EItemContainer::Inventory, EItemContainer::QuickSlot));
				Test->TestEqual(TEXT("Moving to quick storage does not duplicate the stack"), Inventory->GetItemCount(UsableKey), 5);
				Test->TestTrue(TEXT("Source inventory slot is cleared"), Inventory->GetInventorySlotDataList()[0].IsEmpty());
				Test->TestFalse(TEXT("Ore cannot be equipped"), Inventory->TryMoveItemSlot(1, 1, EItemContainer::Inventory, EItemContainer::QuickSlot));
				Test->TestFalse(TEXT("Swapping with an ore cannot smuggle it into quick storage"), Inventory->TryMoveItemSlot(0, 1, EItemContainer::QuickSlot, EItemContainer::Inventory));
				Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UAttributeSet_Character::GetCurrentHealthAttribute(), 20.f);
				Test->TestTrue(TEXT("Quick key consumes a usable item"), Player->GetConsumableComponent()->TryUseQuickSlot(0));
				Test->TestEqual(TEXT("Using a quick item decrements the equipped stack"), Inventory->GetItemSlots(EItemContainer::QuickSlot)[0].ItemCount, 4);
				Inventory->AddItem(UsableKey, 3);
				Test->TestEqual(TEXT("New loot goes into inventory, not the quick stack"), Inventory->GetItemSlots(EItemContainer::QuickSlot)[0].ItemCount, 4);
				Inventory->TryMoveItemSlot(0, 0, EItemContainer::Inventory, EItemContainer::QuickSlot);
				Test->TestEqual(TEXT("Explicit transfer merges matching stacks"), Inventory->GetItemSlots(EItemContainer::QuickSlot)[0].ItemCount, 7);
				Test->TestTrue(TEXT("Quick items transfer back to inventory"), Inventory->TryMoveItemSlot(0, 3, EItemContainer::QuickSlot, EItemContainer::Inventory));
				Test->TestEqual(TEXT("Player arrangement is preserved"), Inventory->GetInventorySlotDataList()[3].ItemCount, 7);
				Test->TestTrue(TEXT("A partial stack drops into the field"), Inventory->TryDropItemSlot(EItemContainer::Inventory, 3, 2));
				Test->TestEqual(TEXT("Only the requested amount leaves storage"), Inventory->GetItemCount(UsableKey), 5);
				for (TActorIterator<AItemBase> It(World); It; ++It)
				{
					if (It->GetItemKey() == UsableKey && It->CanBeCollectedBy(Player.Get()) == false) { DroppedItem = *It; break; }
				}
				if (Test->TestTrue(TEXT("Field stack records its drop owner"), DroppedItem.IsValid()) == false) { return Finish(); }
				UPaperSpriteComponent* FieldSprite = DroppedItem->FindComponentByClass<UPaperSpriteComponent>();
				Test->TestTrue(TEXT("Field item image is in front of the player plane"), FieldSprite->GetComponentLocation().Y < Player->GetActorLocation().Y);
				Test->TestTrue(TEXT("Field item sorts above character sprite accessories"), FieldSprite->TranslucencySortPriority > 1);
				AItemBase* CoinProbe = World->SpawnActor<AItemBase>(SavedTransform.GetLocation() + FVector(5000.f, 0.f, 100.f), FRotator::ZeroRotator);
				CoinProbe->InitializeItemBase(TEXT("Coin"));
				UPaperSpriteComponent* CoinSprite = CoinProbe->FindComponentByClass<UPaperSpriteComponent>();
				const float CoinWidth = CoinSprite->GetSprite()->GetSourceSize().X * CoinSprite->GetRelativeScale3D().X / CoinSprite->GetSprite()->GetPixelsPerUnrealUnit();
				const float ItemWidth = FieldSprite->GetSprite()->GetSourceSize().X * FieldSprite->GetRelativeScale3D().X / FieldSprite->GetSprite()->GetPixelsPerUnrealUnit();
				Test->TestTrue(TEXT("Dropped coin is no larger than a normal item"), CoinWidth <= ItemWidth);
				CoinProbe->Destroy();
				Inventory->SetItemCollectionActive(true);
				Test->TestFalse(TEXT("Dropped items cannot be collected immediately"), Inventory->TryCollectWorldItem(DroppedItem.Get()));
				DroppedItem->TryLoot(Player);
				Test->TestFalse(TEXT("Drop owner cannot magnetize the item"), DroppedItem->IsOccupied());
				Inventory->SetItemCollectionActive(false);
				Player->SetActorLocation(SavedTransform.GetLocation() + FVector(Inventory->GetItemGainRange() + 100.f, 0.f, 32.f), false, nullptr, ETeleportType::TeleportPhysics);
				BlinkItem = World->SpawnActor<AItemBase>(SavedTransform.GetLocation() + FVector(5000.f, 0.f, 100.f), FRotator::ZeroRotator);
				BlinkItem->InitializeItemBase(UsableKey);
				Test->TestTrue(TEXT("New item begins at the bottom of its hover motion"), FMath::IsNearlyZero(BlinkItem->FindComponentByClass<UPaperSpriteComponent>()->GetRelativeLocation().Z));
				BlinkItem->SetLifeSpan(1.4f);
				++Step;
				Started = FPlatformTime::Seconds();
				return false;
			}
			if (BlinkItem.IsValid())
			{
				bSawHiddenBlink |= BlinkItem->FindComponentByClass<UPaperSpriteComponent>()->IsVisible() == false;
				MaximumHoverOffset = FMath::Max(MaximumHoverOffset, BlinkItem->FindComponentByClass<UPaperSpriteComponent>()->GetRelativeLocation().Z);
			}
			if (Step == 1 && Elapsed() > 0.2)
			{
				Test->TestTrue(TEXT("Hover first rises from its bottom position"), BlinkItem->FindComponentByClass<UPaperSpriteComponent>()->GetRelativeLocation().Z > 0.f);
				Test->TestTrue(TEXT("Leaving the pickup radius releases the owner lock"), DroppedItem->CanBeCollectedBy(Player.Get()));
				Player->SetActorTransform(SavedTransform, false, nullptr, ETeleportType::TeleportPhysics);
				Inventory->SetItemCollectionActive(true);
				Test->TestTrue(TEXT("Returning can collect the dropped stack"), Inventory->TryCollectWorldItem(DroppedItem.Get()));
				Test->TestEqual(TEXT("Collected stack restores the conserved total"), Inventory->GetItemCount(UsableKey), 7);
				Inventory->TryMoveItemSlot(3, 2, EItemContainer::Inventory, EItemContainer::QuickSlot);
				UMKSaveGame* Save = NewObject<UMKSaveGame>();
				Save->InventorySlotDataList = Inventory->GetInventorySlotDataList();
				Save->QuickSlotDataList = Inventory->GetItemSlots(EItemContainer::QuickSlot);
				TArray<uint8> Bytes;
				UGameplayStatics::SaveGameToMemory(Save, Bytes);
				UMKSaveGame* Restored = Cast<UMKSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
				if (Test->TestNotNull(TEXT("Both containers serialize through SaveGame"), Restored))
				{
					Test->TestEqual(TEXT("Quick slot position and quantity survive serialization"), Restored->QuickSlotDataList[2].ItemCount, 7);
					Test->TestEqual(TEXT("Ore remains in the inventory container"), Restored->InventorySlotDataList[1].ItemUID, OreKey);
				}
				++Step;
			}
			if (Step == 2 && Elapsed() > 1.8)
			{
				Test->TestTrue(TEXT("Items blink before expiration"), bSawHiddenBlink);
				Test->TestFalse(TEXT("Expired field item is destroyed"), BlinkItem.IsValid());
				Test->TestTrue(TEXT("Hover range is half the previous ten-unit range"), MaximumHoverOffset > 4.f && MaximumHoverOffset <= 5.01f);
				return Finish();
			}
			return false;
		}

	private:
		double Elapsed() const { return FPlatformTime::Seconds() - Started; }
		bool Finish()
		{
			if (DroppedItem.IsValid()) { DroppedItem->Destroy(); }
			if (BlinkItem.IsValid()) { BlinkItem->Destroy(); }
			if (Inventory.IsValid())
			{
				Inventory->ApplySavedInventorySlots(SavedInventory);
				Inventory->ApplySavedQuickSlots(SavedQuick);
				Inventory->SetItemCollectionActive(bCollectionWasActive);
			}
			if (Player.IsValid()) { Player->SetActorTransform(SavedTransform, false, nullptr, ETeleportType::TeleportPhysics); }
			return true;
		}
		FAutomationTestBase* Test;
		float MaximumHoverOffset = 0.f;
		TWeakObjectPtr<AMKCharacter> Player;
		TWeakObjectPtr<UInventoryComponent> Inventory;
		TWeakObjectPtr<AItemBase> DroppedItem;
		TWeakObjectPtr<AItemBase> BlinkItem;
		TArray<FInventorySlotData> SavedInventory;
		TArray<FInventorySlotData> SavedQuick;
		FTransform SavedTransform;
		FName UsableKey;
		FName OreKey;
		bool bCollectionWasActive = true;
		bool bSawHiddenBlink = false;
		int32 Step = 0;
		double Started;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryTransferTest, "ProjectMK.PIE.Inventory.SeparateQuickSlotsAndWorldDrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryTransferTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FInventoryTransferCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
