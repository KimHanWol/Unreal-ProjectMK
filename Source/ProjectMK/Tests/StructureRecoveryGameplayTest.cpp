#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/StorageChestActor.h"
#include "ProjectMK/Actor/Shop/WorkbenchActor.h"
#include "ProjectMK/Actor/Spawnable/BlastChargeActor.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Core/MKSaveGame.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/UI/StorageChestWidget.h"

namespace
{
	class FStructureRecoveryCommand : public IAutomationLatentCommand
	{
	public:
		explicit FStructureRecoveryCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			AMKCharacter* Player = World != nullptr ? Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)) : nullptr;
			if (Player == nullptr)
			{
				if (FPlatformTime::Seconds() - Started > 30.0) { Test->AddError(TEXT("Recovery fixture timed out.")); return true; }
				return false;
			}
			USaveGameSubsystem::Get(World)->StartPlaytestSession();
			ULevelManagerSubsystem* Level = World->GetSubsystem<ULevelManagerSubsystem>();
			UGameSettingDataAsset* Settings = const_cast<UGameSettingDataAsset*>(UDataManager::Get(World)->GetGameSettingDataAsset());
			UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
			APlayerController* Controller = Player->GetController<APlayerController>();
			auto Key = [&](bool bDown) { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F, bDown ? IE_Pressed : IE_Released, bDown ? 1.f : 0.f)); };
			auto Advance = [&]() { ChangedAt = World->GetTimeSeconds(); ++Step; };
			if (Step == 0)
			{
				Saved = Settings->MineableLevelGeneration;
				auto& Generation = Settings->MineableLevelGeneration;
				Generation.StartColumn = 48; Generation.WidthInBlocks = 32; Generation.DepthInBlocks = 24;
				Generation.SurfaceReliefHeight = 0; Generation.FallingStructureCount = 0; Generation.bGenerateDecorations = false; Generation.BuriedChestMaxCount = 0;
				Generation.EmptyBlockRatio = 0.f; Generation.RockBlockRatio = 0.f;
				Level->RegenerateMineableBlocks(821);
				Inventory->ApplySavedInventorySlots({}); Inventory->ApplySavedQuickSlots({}); Inventory->ApplyDiscoveredItemKeys({});
				Inventory->SetItemCollectionActive(false);
				Origin = FVector(56.5f * 16.f, -20.f, -Settings->SurfaceBlockPositionY * 16.f + 9.04f);
				Player->SetActorLocation(Origin); Player->GetCharacterMovement()->StopMovementImmediately(); Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				Test->TestTrue(TEXT("The empty chest installs at the player"), Level->TryPlaceStorageChest(TEXT("StorageChest"), Origin));
				for (TActorIterator<AStorageChestActor> It(World); It; ++It) { Chest = *It; FirstChestId = Chest->GetChestId(); break; }
				Advance(); return false;
			}
			const float Elapsed = World->GetTimeSeconds() - ChangedAt;
			if (Step == 1 && Elapsed > 0.4f) { Key(true); Advance(); return false; }
			if (Step == 2 && Elapsed > 0.15f) { Key(false); Advance(); return false; }
			if (Step == 3 && Elapsed > 0.2f)
			{
				TArray<UUserWidget*> Panels;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Panels, UStorageChestWidget::StaticClass(), false);
				Test->TestTrue(TEXT("A short real F press still opens the nonmodal chest"), Panels.ContainsByPredicate([](UUserWidget* Panel) { return Panel->IsVisible(); }));
				Key(true); Advance(); return false;
			}
			if (Step == 4 && Elapsed > 1.f)
			{
				Key(false);
				Test->TestFalse(TEXT("Holding F removes the actual empty chest"), Chest.IsValid());
				Test->TestTrue(TEXT("Recovered generated chest identities remain removed after regeneration"), Level->GetDestroyedStorageChestIds().Contains(FirstChestId));
				Test->TestEqual(TEXT("Recovery grants exactly one chest without requiring a relic"), Inventory->GetItemCount(TEXT("StorageChest")), 1);
				Test->TestTrue(TEXT("A second chest installs"), Level->TryPlaceStorageChest(TEXT("StorageChest"), Origin));
				for (TActorIterator<AStorageChestActor> It(World); It; ++It) { Chest = *It; break; }
				FInventorySlotData Ore; Ore.ItemUID = TEXT("Ore_Iron"); Ore.ItemCount = 5;
				if (Chest.IsValid()) { Chest->ApplySavedContents({ Ore }); }
				Advance(); return false;
			}
			if (Step == 5 && Elapsed > 0.2f) { Key(true); Advance(); return false; }
			if (Step == 6 && Elapsed > 1.f)
			{
				Key(false);
				Test->TestTrue(TEXT("A loaded chest cannot be recovered"), Chest.IsValid() && Chest->GetItemSlots()[0].ItemCount == 5);
				Test->TestEqual(TEXT("Rejected loaded recovery cannot duplicate the chest item"), Inventory->GetItemCount(TEXT("StorageChest")), 1);
				Test->TestTrue(TEXT("Stored ore can be emptied normally"), Inventory->TryMoveItemSlot(0, 1, EItemContainer::Storage, EItemContainer::Inventory, Chest.Get()));
				Advance(); return false;
			}
			if (Step == 7 && Elapsed > 0.2f) { Key(true); Advance(); return false; }
			if (Step == 8 && Elapsed > 1.f)
			{
				Key(false);
				Test->TestFalse(TEXT("The emptied chest is recoverable again"), Chest.IsValid());
				Test->TestEqual(TEXT("All stored ore survives recovery"), Inventory->GetItemCount(TEXT("Ore_Iron")), 5);
				Test->TestTrue(TEXT("The workbench installs"), Level->TryPlaceWorkbench(TEXT("Workbench"), Origin));
				for (TActorIterator<AWorkbenchActor> It(World); It; ++It) { Bench = *It; break; }
				Advance(); return false;
			}
			if (Step == 9 && Elapsed > 0.2f) { Key(true); Advance(); return false; }
			if (Step == 10 && Elapsed > 1.f)
			{
				Key(false);
				Test->TestFalse(TEXT("Holding F recovers the workbench"), Bench.IsValid());
				Test->TestEqual(TEXT("Workbench item is returned once"), Inventory->GetItemCount(TEXT("Workbench")), 1);
				Test->TestTrue(TEXT("A support beam installs"), Level->TryPlaceBlock(TEXT("SupportBeam"), Origin));
				Support = Level->GetBlockAtWorldLocation(Origin);
				Advance(); return false;
			}
			if (Step == 11 && Elapsed > 0.2f) { Key(true); Advance(); return false; }
			if (Step == 12 && Elapsed > 1.f)
			{
				Key(false);
				Test->TestFalse(TEXT("Holding F removes the support from the block registry"), Support.IsValid());
				Test->TestEqual(TEXT("Support recovery avoids duplicate mining drops"), Inventory->GetItemCount(TEXT("SupportBeam")), 1);
				Inventory->AddItem(TEXT("BlastCharge"), 1);
				Test->TestTrue(TEXT("A timed explosive installs"), Player->GetConsumableComponent()->TryUseItem(TEXT("BlastCharge")));
				for (TActorIterator<ABlastChargeActor> It(World); It; ++It) { Charge = *It; break; }
				Advance(); return false;
			}
			if (Step == 13 && Elapsed > 0.15f) { Key(true); Advance(); return false; }
			if (Step == 14 && Elapsed > 1.f)
			{
				Key(false);
				Test->TestFalse(TEXT("An explosive can be recovered before its fuse ends"), Charge.IsValid());
				Test->TestEqual(TEXT("Recovered explosive returns its item"), Inventory->GetItemCount(TEXT("BlastCharge")), 1);
				UMKSaveGame* Snapshot = NewObject<UMKSaveGame>();
				Snapshot->DiscoveredItemKeys = Inventory->GetDiscoveredItemKeys();
				TArray<uint8> Bytes; UGameplayStatics::SaveGameToMemory(Snapshot, Bytes);
				UMKSaveGame* Reloaded = CastChecked<UMKSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
				Inventory->ApplyDiscoveredItemKeys(Reloaded->DiscoveredItemKeys);
				Inventory->ApplySavedInventorySlots({}); Inventory->ApplySavedQuickSlots({});
				Inventory->AddItem(TEXT("Ore_Iron"), 2);
				Test->TestFalse(TEXT("Discovery history persists after all copies are gone and the save reloads"), Inventory->GetItemSlots(EItemContainer::Inventory)[0].bIsNew);
				Settings->MineableLevelGeneration = Saved;
				return true;
			}
			if (FPlatformTime::Seconds() - Started > 75.0) { Test->AddError(TEXT("Recovery input sequence stalled.")); Key(false); Settings->MineableLevelGeneration = Saved; return true; }
			return false;
		}
	private:
		FAutomationTestBase* Test;
		double Started;
		int32 Step = 0;
		float ChangedAt = 0.f;
		FVector Origin;
		FGuid FirstChestId;
		FMineableLevelGenerationSettings Saved;
		TWeakObjectPtr<AStorageChestActor> Chest;
		TWeakObjectPtr<AWorkbenchActor> Bench;
		TWeakObjectPtr<ABlockBase> Support;
		TWeakObjectPtr<ABlastChargeActor> Charge;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStructureRecoveryTest, "ProjectMK.PIE.Interaction.HoldRecoveryAndDiscoveryHistory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStructureRecoveryTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FStructureRecoveryCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
