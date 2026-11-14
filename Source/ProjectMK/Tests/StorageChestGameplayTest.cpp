#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/GridPanel.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/StorageChestActor.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Core/MKSaveGame.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/UI/StorageChestWidget.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "ProjectMK/UI/QuickInventoryWidget.h"
#include "ProjectMK/UI/InventoryWidget.h"
#include "ProjectMK/UI/ItemSlotWidget.h"
#include "ProjectMK/UI/ItemSlotDragDropOperation.h"
#include "ProjectMK/UI/MKMenuBase.h"
#include "Slate/UMGDragDropOp.h"

namespace
{
	class FStorageChestCommand : public IAutomationLatentCommand
	{
	public:
		explicit FStorageChestCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			AMKCharacter* Player = World != nullptr ? Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)) : nullptr;
			if (Player == nullptr || (Step == 0 && (Player->IsOnGround() == false || World->GetTimeSeconds() < 0.5f)))
			{
				if (FPlatformTime::Seconds() - Started > 25.0) { Test->AddError(TEXT("Chest fixture did not reach a settled floor.")); return true; }
				return false;
			}
			USaveGameSubsystem::Get(World)->StartPlaytestSession();
			UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
			ULevelManagerSubsystem* Level = World->GetSubsystem<ULevelManagerSubsystem>();
			if (Step == 0)
			{
				SavedPlayerLocation = Player->GetActorLocation();
				SavedInventory = Inventory->GetItemSlots(EItemContainer::Inventory);
				SavedQuick = Inventory->GetItemSlots(EItemContainer::QuickSlot);
				Inventory->ApplySavedInventorySlots({});
				Inventory->ApplySavedQuickSlots({});
				Inventory->SetItemCollectionActive(false);
				Test->TestTrue(TEXT("Chest fixture fits inventory"), Inventory->AddItem(TEXT("StorageChest"), 2) && Inventory->AddItem(TEXT("Ore_Iron"), 5) && Inventory->AddItem(TEXT("OxygenCapsule"), 2));
				Test->TestTrue(TEXT("Chest item installs on a floor"), Player->GetConsumableComponent()->TryUseItem(TEXT("StorageChest")));
				Test->TestEqual(TEXT("Placement consumes one chest"), Inventory->GetItemCount(TEXT("StorageChest")), 1);
				Test->TestFalse(TEXT("Duplicate chest placement is rejected"), Player->GetConsumableComponent()->TryUseItem(TEXT("StorageChest")));
				for (TActorIterator<AStorageChestActor> It(World); It; ++It) { if (It->IsBuried() == false && FVector::DistSquared(It->GetActorLocation(), Player->GetActorLocation()) < FMath::Square(16.f)) { Chest = *It; break; } }
				if (Test->TestNotNull(TEXT("An installed chest exists"), Chest.Get()) == false) { return true; }
				Test->TestEqual(TEXT("Chest exposes exactly ten slots"), Chest->GetItemSlots().Num(), 10);
				Test->TestTrue(TEXT("F interaction opens chest"), Chest->TryInteract(Player));
				Test->TestTrue(TEXT("Minerals move into storage"), Inventory->TryMoveItemSlot(1, 0, EItemContainer::Inventory, EItemContainer::Storage, nullptr, Chest.Get()));
				Test->TestEqual(TEXT("Stored ore is no longer carried or available to crafting"), Inventory->GetItemCount(TEXT("Ore_Iron")), 0);
				Test->TestFalse(TEXT("Stored ore cannot enter a quick slot"), Inventory->TryMoveItemSlot(0, 0, EItemContainer::Storage, EItemContainer::QuickSlot, Chest.Get()));
				Test->TestTrue(TEXT("Usable items move into storage"), Inventory->TryMoveItemSlot(2, 1, EItemContainer::Inventory, EItemContainer::Storage, nullptr, Chest.Get()));
				Test->TestTrue(TEXT("Stored usable items can enter quick slots"), Inventory->TryMoveItemSlot(1, 0, EItemContainer::Storage, EItemContainer::QuickSlot, Chest.Get()));
				Test->TestTrue(TEXT("Quick slot items can return to storage"), Inventory->TryMoveItemSlot(0, 1, EItemContainer::QuickSlot, EItemContainer::Storage, nullptr, Chest.Get()));
				const FVector Origin = Player->GetActorLocation();
				Player->SetActorLocation(Origin + FVector(200.f, 0.f, 0.f));
				Test->TestFalse(TEXT("Stale distant chest transfers cannot mutate contents"), Inventory->TryMoveItemSlot(0, 3, EItemContainer::Storage, EItemContainer::Inventory, Chest.Get()));
				Player->SetActorLocation(Origin);
				ChangedAt = World->GetTimeSeconds();
				++Step;
				return false;
			}
			if (World->GetTimeSeconds() - ChangedAt < 0.3f) { return false; }
			TArray<UUserWidget*> Found;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, UStorageChestWidget::StaticClass(), false);
			UStorageChestWidget* Contents = Found.Num() > 0 ? CastChecked<UStorageChestWidget>(Found[0]) : nullptr;
			if (Step == 2)
			{
				Test->TestTrue(TEXT("The nonmodal chest window closes after leaving interaction range"), Contents != nullptr && Contents->IsVisible() == false);
				Player->SetActorLocation(SavedPlayerLocation);
				Inventory->ApplySavedInventorySlots(SavedInventory);
				Inventory->ApplySavedQuickSlots(SavedQuick);
				return true;
			}
			TArray<UUserWidget*> HUDs;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, HUDs, UHUDWidget::StaticClass(), false);
			UHUDWidget* HUD = HUDs.Num() > 0 ? CastChecked<UHUDWidget>(HUDs[0]) : nullptr;
			Test->TestTrue(TEXT("Chest is a nonmodal viewport panel and never opens the modal menu"), Contents != nullptr && Contents->IsInViewport() && HUD != nullptr && HUD->IsMenuVisible() == false);
			bool bHasDuplicateInventory = false;
			if (Contents != nullptr) { Contents->WidgetTree->ForEachWidget([&](UWidget* Widget) { bHasDuplicateInventory |= Cast<UInventoryWidget>(Widget) != nullptr; }); }
			Test->TestFalse(TEXT("The chest panel contains no duplicated player inventory"), bHasDuplicateInventory);
			UGridPanel* Grid = Contents != nullptr ? Cast<UGridPanel>(Contents->WidgetTree->FindWidget(TEXT("StorageSlotGrid"))) : nullptr;
			const UImage* HeaderIcon = Contents != nullptr ? Cast<UImage>(Contents->WidgetTree->FindWidget(TEXT("IconStorage"))) : nullptr;
			Test->TestTrue(TEXT("Storage header resolves an actual common art brush instead of a white image"), HeaderIcon != nullptr && HeaderIcon->GetBrush().GetResourceObject() != nullptr);
			if (Test->TestTrue(TEXT("Popup renders ten shared inventory slot widgets"), Grid != nullptr && Grid->GetChildrenCount() == 10 && Cast<UItemSlotWidget>(Grid->GetChildAt(0)) != nullptr))
			{
				UItemSlotWidget* Slot = CastChecked<UItemSlotWidget>(Grid->GetChildAt(0));
				const FGeometry& Geometry = Slot->GetCachedGeometry();
				const FVector2D Position = Geometry.GetAbsolutePosition();
				const FPointerEvent Pointer(0, Position, Position, TSet<FKey>{ EKeys::LeftMouseButton }, EKeys::LeftMouseButton, 0.f, FModifierKeysState());
				const FReply Reply = Slot->TakeWidget()->OnDragDetected(Geometry, Pointer);
				const auto Drag = Reply.GetDragDropContent();
				if (Test->TestTrue(TEXT("Storage begins a real shared UMG item drag"), Drag.IsValid()))
				{
					const auto UMG = StaticCastSharedPtr<FUMGDragDropOp>(Drag);
					const auto Operation = Cast<UItemSlotDragDropOperation>(UMG->GetOperation());
					Test->TestTrue(TEXT("Storage drag carries its source chest address"), Operation != nullptr && Operation->SourceStorage == Chest && Operation->SourceContainer == EItemContainer::Storage);
					UInventoryWidget* PlayerGrid = HUD != nullptr ? Cast<UInventoryWidget>(HUD->WidgetTree->FindWidget(TEXT("Inventory"))) : nullptr;
					UGridPanel* Items = PlayerGrid != nullptr ? Cast<UGridPanel>(PlayerGrid->WidgetTree->FindWidget(TEXT("ItemSlotPanel"))) : nullptr;
					UItemSlotWidget* Target = Items != nullptr ? Cast<UItemSlotWidget>(Items->GetChildAt(3)) : nullptr;
					if (Test->TestNotNull(TEXT("The existing HUD inventory remains the drag target"), Target))
					{
						const auto Drop = Target->TakeWidget()->OnDrop(Target->GetCachedGeometry(), FDragDropEvent(Pointer, Drag.ToSharedRef()));
						Test->TestTrue(TEXT("The actual player slot accepts a storage drag"), Drop.IsEventHandled());
						Test->TestEqual(TEXT("A real storage drag conserves and transfers its stack"), Inventory->GetItemCount(TEXT("Ore_Iron")), 5);
						Inventory->TryMoveItemSlot(3, 0, EItemContainer::Inventory, EItemContainer::Storage, nullptr, Chest.Get());
					}
				}
				const FPointerEvent RightClick(0, Position, Position, TSet<FKey>{ EKeys::RightMouseButton }, EKeys::RightMouseButton, 0.f, FModifierKeysState());
				Slot->TakeWidget()->OnPreviewMouseButtonDown(Geometry, RightClick);
				Test->TestEqual(TEXT("Right click transfers the chest stack without using or selling it"), Inventory->GetItemCount(TEXT("Ore_Iron")), 5);
				const auto& Items = Inventory->GetItemSlots(EItemContainer::Inventory);
				for (int32 Index = 0; Index < Items.Num(); ++Index)
				{
					if (Items[Index].ItemUID != TEXT("Ore_Iron")) { continue; }
					UInventoryWidget* HUDInventory = Cast<UInventoryWidget>(HUD->WidgetTree->FindWidget(TEXT("Inventory")));
					UGridPanel* HUDGrid = HUDInventory != nullptr ? Cast<UGridPanel>(HUDInventory->WidgetTree->FindWidget(TEXT("ItemSlotPanel"))) : nullptr;
					if (HUDGrid != nullptr)
					{
						UItemSlotWidget* PlayerSlot = CastChecked<UItemSlotWidget>(HUDGrid->GetChildAt(Index));
						const auto PlayerDrag = PlayerSlot->TakeWidget()->OnDragDetected(PlayerSlot->GetCachedGeometry(), Pointer).GetDragDropContent();
						if (Test->TestTrue(TEXT("The existing HUD inventory starts a storage deposit drag"), PlayerDrag.IsValid())) { Slot->TakeWidget()->OnDrop(Geometry, FDragDropEvent(Pointer, PlayerDrag.ToSharedRef())); }
						Test->TestEqual(TEXT("HUD inventory deposits directly into the nonmodal storage window"), Inventory->GetItemCount(TEXT("Ore_Iron")), 0);
					}
					break;
				}
				UItemSlotWidget* CapsuleSlot = CastChecked<UItemSlotWidget>(Grid->GetChildAt(1));
				TArray<UUserWidget*> QuickBars;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, QuickBars, UQuickInventoryWidget::StaticClass(), false);
				UItemSlotWidget* QuickTarget = nullptr;
				if (QuickBars.Num() > 0) { QuickBars[0]->WidgetTree->ForEachWidget([&](UWidget* Widget) { if (QuickTarget == nullptr) { QuickTarget = Cast<UItemSlotWidget>(Widget); } }); }
				if (Test->TestNotNull(TEXT("The existing HUD quickbar remains available"), QuickTarget))
				{
					const FReply CapsuleDrag = CapsuleSlot->TakeWidget()->OnDragDetected(CapsuleSlot->GetCachedGeometry(), Pointer);
					const auto CapsuleOperation = CapsuleDrag.GetDragDropContent();
					if (Test->TestTrue(TEXT("Usable storage items start a real drag"), CapsuleOperation.IsValid()))
					{
						QuickTarget->TakeWidget()->OnDrop(QuickTarget->GetCachedGeometry(), FDragDropEvent(Pointer, CapsuleOperation.ToSharedRef()));
						Test->TestTrue(TEXT("Storage drops directly into the HUD quickbar"), Inventory->GetItemSlots(EItemContainer::QuickSlot)[0].ItemUID == TEXT("OxygenCapsule"));
						Inventory->TryMoveItemSlot(0, 1, EItemContainer::QuickSlot, EItemContainer::Storage, nullptr, Chest.Get());
					}
				}
			}
			const FGuid Id = Chest->GetChestId();
			const FVector Location = Chest->GetActorLocation();
			UMKSaveGame* Save = NewObject<UMKSaveGame>();
			Save->StorageChests = Level->GetStorageChestSaveData();
			TArray<uint8> Bytes;
			Test->TestTrue(TEXT("Chest state serializes using the actual save format"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
			UMKSaveGame* Loaded = Cast<UMKSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
			if (Test->TestNotNull(TEXT("Chest save can be loaded"), Loaded) == false) { return true; }
			Level->RestoreStorageChests(Loaded->StorageChests, {});
			Chest.Reset();
			for (TActorIterator<AStorageChestActor> It(World); It; ++It) { if (It->GetChestId() == Id) { Chest = *It; break; } }
			if (Test->TestNotNull(TEXT("Installed chest restores with its identity"), Chest.Get()) == false) { return true; }
			Test->TestEqual(TEXT("Reload preserves stored ore quantity"), Chest->GetItemSlots()[0].ItemCount, 5);
			Test->TestEqual(TEXT("Reload preserves stored consumable quantity"), Chest->GetItemSlots()[1].ItemCount, 2);
			Test->TestTrue(TEXT("Blast demolishes the installed chest"), Level->ApplyExplosion(Location, 0, Player->GetAbilitySystemComponent()) > 0);
			TMap<FName, int32> Drops;
			AItemBase* ChestDrop = nullptr;
			for (TActorIterator<AItemBase> It(World); It; ++It)
			{
				Drops.FindOrAdd(It->GetItemKey()) += It->GetItemCount();
				if (It->GetItemKey() == TEXT("StorageChest")) { ChestDrop = *It; }
			}
			Test->TestEqual(TEXT("Explosion drops the empty chest item once"), Drops.FindRef(TEXT("StorageChest")), 1);
			Test->TestEqual(TEXT("Explosion drops every stored ore"), Drops.FindRef(TEXT("Ore_Iron")), 5);
			Test->TestEqual(TEXT("Explosion drops every stored usable item separately"), Drops.FindRef(TEXT("OxygenCapsule")), 2);
			Test->TestEqual(TEXT("Repeated blast cannot duplicate chest or contents"), Level->ApplyExplosion(Location, 0, Player->GetAbilitySystemComponent()), 0);
			Test->TestTrue(TEXT("Destroyed chest is recorded to prevent loot restoration"), Level->GetDestroyedStorageChestIds().Contains(Id));
			const TArray<FGuid> Destroyed = Level->GetDestroyedStorageChestIds();
			Level->RestoreStorageChests(Loaded->StorageChests, Destroyed);
			Test->TestFalse(TEXT("Destroyed save records cannot respawn opened loot"), Level->GetStorageChestSaveData().ContainsByPredicate([&](const auto& Data) { return Data.Id == Id; }));
			Inventory->SetItemCollectionActive(true);
			Test->TestTrue(TEXT("The empty dropped chest can be collected"), ChestDrop != nullptr && Inventory->TryCollectWorldItem(ChestDrop));
			Inventory->SetItemCollectionActive(false);
			Test->TestTrue(TEXT("Recovered chest item can be installed again"), Player->GetConsumableComponent()->TryUseItem(TEXT("StorageChest")));
			for (TActorIterator<AStorageChestActor> It(World); It; ++It)
			{
				if (It->IsBuried() == false && FVector::DistSquared(It->GetActorLocation(), Player->GetActorLocation()) < FMath::Square(16.f))
				{
					Test->TestFalse(TEXT("Reinstalled chest is empty instead of carrying or duplicating contents"), It->GetItemSlots().ContainsByPredicate([](const auto& Slot) { return Slot.IsEmpty() == false; }));
					It->TryInteract(Player);
				}
			}
			Player->SetActorLocation(SavedPlayerLocation + FVector(200.f, 0.f, 0.f));
			ChangedAt = World->GetTimeSeconds();
			++Step;
			return false;
		}

	private:
		FAutomationTestBase* Test;
		double Started;
		int32 Step = 0;
		float ChangedAt = 0.f;
		FVector SavedPlayerLocation = FVector::ZeroVector;
		TWeakObjectPtr<AStorageChestActor> Chest;
		TArray<FInventorySlotData> SavedInventory, SavedQuick;
	};

	class FBuriedChestCommand : public IAutomationLatentCommand
	{
	public:
		explicit FBuriedChestCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			ULevelManagerSubsystem* Level = World != nullptr ? World->GetSubsystem<ULevelManagerSubsystem>() : nullptr;
			if (Level == nullptr || Level->GetBlocks().IsEmpty()) { return FPlatformTime::Seconds() - Started > 25.0; }
			USaveGameSubsystem::Get(World)->StartPlaytestSession();
			UGameSettingDataAsset* Settings = const_cast<UGameSettingDataAsset*>(UDataManager::Get(World)->GetGameSettingDataAsset());
			if (Step == 0)
			{
				Saved = Settings->MineableLevelGeneration;
				auto& Generation = Settings->MineableLevelGeneration;
				Generation.WidthInBlocks = 96;
				Generation.DepthInBlocks = 64;
				Generation.BuriedChestChancePerBlock = 1.f;
				Generation.bGenerateDecorations = false;
				Generation.BuriedChestMaxCount = 4;
				Generation.FallingStructureCount = 1;
				Generation.FallingStructuresPerThousandTiles = 0.f;
				for (int32 Seed : { 731, 732, 733 })
				{
					Level->RegenerateMineableBlocks(Seed);
					for (const EMineStructurePattern Pattern : Generation.GuaranteedStructurePatterns)
					{
						Test->TestTrue(TEXT("Every configured guaranteed pattern appears even below the random density quota"), Level->GetGeneratedStructures().ContainsByPredicate([&](const auto& Structure) { return Structure.bGuaranteed && Structure.Pattern == Pattern; }));
					}
				}
				Level->RegenerateMineableBlocks(731);
				const auto First = Level->GetStorageChestSaveData();
				Test->TestEqual(TEXT("The chest generation cap is respected"), First.Num(), 4);
				Level->RegenerateMineableBlocks(731);
				const auto Repeated = Level->GetStorageChestSaveData();
				Test->TestEqual(TEXT("The same seed generates the same chest count"), Repeated.Num(), First.Num());
				for (int32 Index = 0; Index < FMath::Min(First.Num(), Repeated.Num()); ++Index)
				{
					Test->TestTrue(TEXT("Buried chest identity and position are seed stable"), First[Index].Id == Repeated[Index].Id && First[Index].Location.Equals(Repeated[Index].Location));
					for (int32 Slot = 0; Slot < First[Index].Slots.Num(); ++Slot) { Test->TestTrue(TEXT("Buried loot is seed stable"), First[Index].Slots[Slot].ItemUID == Repeated[Index].Slots[Slot].ItemUID && First[Index].Slots[Slot].ItemCount == Repeated[Index].Slots[Slot].ItemCount); }
					Test->TestTrue(TEXT("Buried chests start hidden and have a nonempty loot roll"), Repeated[Index].bBuried && Repeated[Index].Slots.ContainsByPredicate([](const auto& Slot) { return Slot.IsEmpty() == false; }));
				}
				for (TActorIterator<AStorageChestActor> It(World); It; ++It) { Chest = *It; break; }
				if (Chest.IsValid())
				{
					Test->TestTrue(TEXT("Buried chest is visually hidden"), Chest->IsHidden());
					Test->TestFalse(TEXT("Buried chest cannot show an F prompt through solid ground"), Chest->CanInteract(UGameplayStatics::GetPlayerCharacter(World, 0)));
					ABlockBase* Cover = Level->GetBlockAtWorldLocation(Chest->GetActorLocation());
					if (Test->TestNotNull(TEXT("A real mineable block conceals the chest"), Cover)) { Cover->TryDestroyByExplosion(); }
				}
				ChangedAt = World->GetTimeSeconds();
				++Step;
				return false;
			}
			if (World->GetTimeSeconds() - ChangedAt < 0.35f) { return false; }
			Test->TestTrue(TEXT("Removing the covering terrain reveals the installed chest"), Chest.IsValid() && Chest->IsBuried() == false && Chest->IsHidden() == false);
			Settings->MineableLevelGeneration = Saved;
			return true;
		}

	private:
		FAutomationTestBase* Test;
		double Started;
		int32 Step = 0;
		float ChangedAt = 0.f;
		FMineableLevelGenerationSettings Saved;
		TWeakObjectPtr<AStorageChestActor> Chest;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStorageChestTest, "ProjectMK.PIE.Storage.PlacementTransferExplosionAndPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorageChestTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FStorageChestCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuriedChestTest, "ProjectMK.PIE.Storage.BuriedLootAndGuaranteedPatterns", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBuriedChestTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FBuriedChestCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
