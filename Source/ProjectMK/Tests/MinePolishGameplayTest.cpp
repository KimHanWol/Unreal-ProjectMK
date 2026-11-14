#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/GridPanel.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/StorageChestActor.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "ProjectMK/UI/InventoryWidget.h"
#include "ProjectMK/UI/ItemSlotWidget.h"
#include "ProjectMK/UI/PlaytestWidget.h"

namespace
{
	class FChestFallCommand : public IAutomationLatentCommand
	{
	public:
		explicit FChestFallCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			AMKCharacter* Player = World != nullptr ? Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)) : nullptr;
			if (Player == nullptr) { if (FPlatformTime::Seconds() - Started > 25.0) { Test->AddError(TEXT("Chest physics fixture timed out.")); return true; } return false; }
			USaveGameSubsystem::Get(World)->StartPlaytestSession();
			ULevelManagerSubsystem* Level = World->GetSubsystem<ULevelManagerSubsystem>();
			UGameSettingDataAsset* Settings = const_cast<UGameSettingDataAsset*>(UDataManager::Get(World)->GetGameSettingDataAsset());
			const int32 Surface = Settings->SurfaceBlockPositionY;
			if (Step == 0)
			{
				Saved = Settings->MineableLevelGeneration;
				auto& Generation = Settings->MineableLevelGeneration;
				Generation.StartColumn = 48;
				Generation.WidthInBlocks = 32;
				Generation.DepthInBlocks = 24;
				Generation.FallingStructureCount = 0;
				Generation.EmptyBlockRatio = 0.f;
				Generation.RockBlockRatio = 0.f;
				Generation.bGenerateDecorations = false;
				Generation.BuriedChestMaxCount = 0;
				Level->RegenerateMineableBlocks(812);
				for (int32 X = 55; X <= 57; ++X)
				{
					for (int32 Y = Surface + 2; Y < Surface + 10; ++Y)
					{
						if (X == 56 && Y >= Surface + 5) { continue; }
						if (ABlockBase* Block = Level->GetBlockAtPosition(FVector2D(X, Y))) { Block->TryDestroyByExplosion(); }
					}
				}
				FStorageChestSaveData Record;
				Record.Id = FGuid::NewGuid();
				Record.Location = FVector(56.5f * 16.f, -20.f, -(Surface + 4.5f) * 16.f);
				FInventorySlotData Ore;
				Ore.ItemUID = TEXT("Ore_Iron"); Ore.ItemCount = 7;
				Record.Slots.Add(Ore);
				Level->RestoreStorageChests({ Record }, {});
				for (TActorIterator<AStorageChestActor> It(World); It; ++It) { if (It->GetChestId() == Record.Id) { Chest = *It; break; } }
				Test->TestNotNull(TEXT("Chest installs in the excavated chamber"), Chest.Get());
				Player->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
				Player->SetActorLocation(Record.Location + FVector(16.f, 0.f, 4.f));
				ChangedAt = World->GetTimeSeconds(); ++Step; return false;
			}
			if (World->GetTimeSeconds() - ChangedAt < 0.3f) { return false; }
			if (Step == 1)
			{
				if (Chest.IsValid() == false) { Settings->MineableLevelGeneration = Saved; return true; }
				ABlockBase* Body = Level->GetBlockAtWorldLocation(Chest->GetActorLocation());
				Test->TestTrue(TEXT("Chest occupies a real falling-block cell"), Body != nullptr && Body->CanFall() && Body->IsMineable() == false);
				Test->TestTrue(TEXT("A nearby player can open the installed chest"), Chest->CanInteract(Player) && Chest->TryInteract(Player));
				InitialZ = Chest->GetActorLocation().Z;
				for (int32 Y = Surface + 5; Y <= Surface + 7; ++Y) { if (ABlockBase* Block = Level->GetBlockAtPosition(FVector2D(56, Y))) { Block->TryDestroyByExplosion(); } }
				ChangedAt = World->GetTimeSeconds(); ++Step; return false;
			}
			const float FallElapsed = World->GetTimeSeconds() - ChangedAt;
			ABlockBase* SettledBody = Chest.IsValid() ? Level->GetBlockAtWorldLocation(Chest->GetActorLocation()) : nullptr;
			if (FallElapsed < 1.5f || ((SettledBody == nullptr || SettledBody->IsFalling()) && FallElapsed < 6.f)) { return false; }
			Test->AddInfo(FString::Printf(TEXT("Chest fall: initial Z %.2f, final Z %.2f, elapsed %.2f, registered %d"), InitialZ, Chest.IsValid() ? Chest->GetActorLocation().Z : 0.f, FallElapsed, SettledBody != nullptr));
			Test->TestTrue(TEXT("An unsupported chest falls several block cells"), Chest.IsValid() && Chest->GetActorLocation().Z < InitialZ - 32.f);
			if (Chest.IsValid())
			{
				ABlockBase* Body = Level->GetBlockAtWorldLocation(Chest->GetActorLocation());
				Test->TestTrue(TEXT("Chest settles back into the terrain block registry"), Body != nullptr && Body->IsFalling() == false);
				Test->TestEqual(TEXT("Falling preserves every stored item"), Chest->GetItemSlots()[0].ItemCount, 7);
				Player->SetActorLocation(Chest->GetActorLocation() + FVector(16.f, 0.f, 4.f));
				Test->TestTrue(TEXT("Chest can be interacted with after landing"), Chest->CanInteract(Player) && Chest->TryInteract(Player));
			}
			UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
			Inventory->ApplySavedInventorySlots({}); Inventory->ApplySavedQuickSlots({}); Inventory->ApplyDiscoveredItemKeys({});
			Inventory->AddItem(TEXT("Ore_Coal"), 3);
			Test->TestTrue(TEXT("A newly collected item is unread"), Inventory->GetItemSlots(EItemContainer::Inventory)[0].bIsNew);
			TArray<UUserWidget*> HUDs;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, HUDs, UHUDWidget::StaticClass(), false);
			UInventoryWidget* Display = Cast<UInventoryWidget>(HUDs[0]->WidgetTree->FindWidget(TEXT("Inventory")));
			UGridPanel* Grid = CastChecked<UGridPanel>(Display->WidgetTree->FindWidget(TEXT("ItemSlotPanel")));
			UItemSlotWidget* Slot = CastChecked<UItemSlotWidget>(Grid->GetChildAt(0));
			UBorder* Dot = CastChecked<UBorder>(Slot->WidgetTree->FindWidget(TEXT("NewItemDot")));
			Test->TestTrue(TEXT("Unread items show the reusable red indicator"), Dot->IsVisible());
			const FPointerEvent Pointer(0, FVector2D::ZeroVector, FVector2D::ZeroVector, TSet<FKey>(), EKeys::Invalid, 0.f, FModifierKeysState());
			Slot->TakeWidget()->OnMouseEnter(Slot->GetCachedGeometry(), Pointer);
			Test->TestFalse(TEXT("Hover marks the actual inventory slot as read"), Inventory->GetItemSlots(EItemContainer::Inventory)[0].bIsNew);
			Test->TestFalse(TEXT("Hover removes the red indicator"), Dot->IsVisible());
			Inventory->AddItem(TEXT("Ore_Coal"), 1);
			Test->TestFalse(TEXT("Collecting an already discovered item never creates another red indicator"), Inventory->GetItemSlots(EItemContainer::Inventory)[0].bIsNew);
			Inventory->AddItem(TEXT("Ore_Copper"), 1);
			Test->TestTrue(TEXT("A different item type receives its own first discovery"), Inventory->GetItemSlots(EItemContainer::Inventory)[1].bIsNew);
			Inventory->TryMoveItemSlot(1, 7);
			Test->TestTrue(TEXT("Rearranging a stack preserves its unread state"), Inventory->GetItemSlots(EItemContainer::Inventory)[7].bIsNew);
			TArray<FInventorySlotData> MergeSlots;
			MergeSlots.SetNum(8);
			MergeSlots[0].ItemUID = TEXT("Ore_Coal"); MergeSlots[0].ItemCount = 2;
			MergeSlots[1].ItemUID = TEXT("Ore_Coal"); MergeSlots[1].ItemCount = 3; MergeSlots[1].bIsNew = true;
			Inventory->ApplySavedInventorySlots(MergeSlots);
			Test->TestTrue(TEXT("An unread stack merges into an existing seen stack"), Inventory->TryMoveItemSlot(1, 0));
			Test->TestTrue(TEXT("The merged stack keeps its unread indicator"), Inventory->GetItemSlots(EItemContainer::Inventory)[0].bIsNew);
			Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			Player->SetActorLocation(FVector(56.5f * 16.f, -20.f, -Surface * 16.f + 10.f));
			TArray<UUserWidget*> Tools;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Tools, UPlaytestWidget::StaticClass(), false);
			const int32 Before = Level->GetStorageChestSaveData().Num();
			Tools[0]->ProcessEvent(Tools[0]->FindFunctionChecked(TEXT("OnSpawnChestClicked")), nullptr);
			Test->TestTrue(TEXT("The cheat UI places an actual chest near the player"), Level->GetStorageChestSaveData().Num() > Before);
			Settings->MineableLevelGeneration = Saved;
			return true;
		}
	private:
		FAutomationTestBase* Test;
		double Started;
		int32 Step = 0;
		float ChangedAt = 0.f, InitialZ = 0.f;
		FMineableLevelGenerationSettings Saved;
		TWeakObjectPtr<AStorageChestActor> Chest;
	};

	class FIceTractionCommand : public IAutomationLatentCommand
	{
	public:
		explicit FIceTractionCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			AMKCharacter* Player = World != nullptr ? Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)) : nullptr;
			if (Player == nullptr) { if (FPlatformTime::Seconds() - Started > 25.0) { Test->AddError(TEXT("Ice traction fixture timed out.")); return true; } return false; }
			USaveGameSubsystem::Get(World)->StartPlaytestSession();
			ULevelManagerSubsystem* Level = World->GetSubsystem<ULevelManagerSubsystem>();
			UGameSettingDataAsset* Settings = const_cast<UGameSettingDataAsset*>(UDataManager::Get(World)->GetGameSettingDataAsset());
			const int32 Surface = Settings->SurfaceBlockPositionY;
			UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
			auto MakeFloor = [&](FName Key)
			{
				Movement->SetMovementMode(MOVE_Flying);
				Player->SetActorLocation(FVector(56.5f * 16.f, -20.f, -Surface * 16.f + 80.f));
				for (int32 X = 50; X <= 76; ++X)
				{
					ABlockBase* Block = Level->GetBlockAtPosition(FVector2D(X, Surface));
					if (Block != nullptr) { Block->Destroy(); }
					Test->TestTrue(TEXT("Traction test platform can be placed"), Level->TryPlaceBlock(Key, FVector((X + 0.5f) * 16.f, -20.f, -(Surface + 0.5f) * 16.f)));
				}
				Movement->Velocity = FVector::ZeroVector;
				Player->SetActorLocation(FVector(56.5f * 16.f, -20.f, -Surface * 16.f + 9.1f));
				Movement->SetMovementMode(MOVE_Walking);
			};
			if (Step == 0)
			{
				Saved = Settings->MineableLevelGeneration;
				auto& Generation = Settings->MineableLevelGeneration;
				Generation.StartColumn = 48; Generation.WidthInBlocks = 32; Generation.DepthInBlocks = 24;
				Generation.SurfaceReliefHeight = 0; Generation.FallingStructureCount = 0; Generation.bGenerateDecorations = false; Generation.BuriedChestMaxCount = 0;
				Generation.EmptyBlockRatio = 0.f; Generation.RockBlockRatio = 0.f;
				Level->RegenerateMineableBlocks(813);
				OriginalFriction = Movement->GroundFriction;
				MakeFloor(TEXT("Ice")); ChangedAt = World->GetTimeSeconds(); ++Step; return false;
			}
			if (World->GetTimeSeconds() - ChangedAt < (Step % 2 == 1 ? 0.2f : 0.35f)) { return false; }
			if (Step == 1)
			{
				Test->TestTrue(TEXT("The character detects real ice under its feet"), Movement->GroundFriction < OriginalFriction);
				Movement->Velocity = FVector(100.f, 0.f, 0.f); StartX = Player->GetActorLocation().X;
				ChangedAt = World->GetTimeSeconds(); ++Step; return false;
			}
			if (Step == 2)
			{
				IceDistance = Player->GetActorLocation().X - StartX;
				Test->TestTrue(TEXT("The character coasts on ice after movement input stops"), IceDistance > 12.f && Movement->Velocity.X > 10.f);
				MakeFloor(TEXT("Dirt")); ChangedAt = World->GetTimeSeconds(); ++Step; return false;
			}
			if (Step == 3)
			{
				Test->TestTrue(TEXT("Leaving ice restores the original ground friction"), FMath::IsNearlyEqual(Movement->GroundFriction, OriginalFriction));
				Movement->Velocity = FVector(100.f, 0.f, 0.f); StartX = Player->GetActorLocation().X;
				ChangedAt = World->GetTimeSeconds(); ++Step; return false;
			}
			const float DirtDistance = Player->GetActorLocation().X - StartX;
			Test->TestTrue(TEXT("Ice produces substantially longer coasting than ordinary dirt"), IceDistance > DirtDistance + 8.f);
			Settings->MineableLevelGeneration = Saved;
			return true;
		}
	private:
		FAutomationTestBase* Test;
		double Started;
		int32 Step = 0;
		float ChangedAt = 0.f, StartX = 0.f, IceDistance = 0.f, OriginalFriction = 0.f;
		FMineableLevelGenerationSettings Saved;
	};

	class FVeinFoundationCommand : public IAutomationLatentCommand
	{
	public:
		explicit FVeinFoundationCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			ULevelManagerSubsystem* Level = World != nullptr ? World->GetSubsystem<ULevelManagerSubsystem>() : nullptr;
			if (Level == nullptr || Level->GetBlocks().IsEmpty()) { if (FPlatformTime::Seconds() - Started > 25.0) { Test->AddError(TEXT("Vein generation fixture timed out.")); return true; } return false; }
			USaveGameSubsystem::Get(World)->StartPlaytestSession();
			UDataManager* Data = UDataManager::Get(World);
			UGameSettingDataAsset* Settings = const_cast<UGameSettingDataAsset*>(Data->GetGameSettingDataAsset());
			const auto Saved = Settings->MineableLevelGeneration;
			auto& Generation = Settings->MineableLevelGeneration;
			Generation.WidthInBlocks = 96; Generation.DepthInBlocks = 64; Generation.StartColumn = 0;
			Generation.FallingStructureCount = 0; Generation.EmptyBlockRatio = 0.f; Generation.RockBlockRatio = 0.f;
			Generation.OreVeinsPerThousandTiles = 4.f;
			Level->RegenerateMineableBlocks(814);
			const int32 Foundation = Data->GetBlockDataTableRow(TEXT("SurfaceFoundation"))->TileIndex;
			int32 ProtectedCount = 0, OrdinaryCount = 0;
			for (int32 X = 0; X < 96; ++X)
			{
				ABlockBase* Block = Level->GetBlockAtPosition(FVector2D(X, Level->GetSurfaceRow(X)));
				if (Block == nullptr) { continue; }
				if (Block->GetTileIndex() == Foundation)
				{
					++ProtectedCount;
					Test->TestFalse(TEXT("Landmark foundations cannot be drilled"), Block->IsMineable());
					Test->TestFalse(TEXT("Landmark foundations cannot be blasted"), Block->TryDestroyByExplosion());
					if (Level->GetSurfaceRow(X) == Settings->SurfaceBlockPositionY)
					{
						ABlockBase* Below = Level->GetBlockAtPosition(FVector2D(X, Settings->SurfaceBlockPositionY + 1));
						Test->TestTrue(TEXT("Flat landmark foundations occupy only one protected stone layer"), Below != nullptr && Below->GetTileIndex() != Foundation);
					}
				}
				else { ++OrdinaryCount; }
			}
			Test->TestTrue(TEXT("Only landmark and raised-ground areas receive protected stone"), ProtectedCount > 0 && OrdinaryCount > 0);
			TSet<EOreVeinShape> Shapes;
			uint32 Hash = 0;
			auto VeinHash = [](const TArray<FGeneratedOreVein>& Veins)
			{
				uint32 Value = 0;
				for (const auto& Vein : Veins) { Value = HashCombine(Value, GetTypeHash(Vein.ItemKey)); for (const auto& Cell : Vein.Cells) { Value = HashCombine(Value, GetTypeHash(Cell)); } }
				return Value;
			};
			for (const auto& Vein : Level->GetGeneratedOreVeins())
			{
				Shapes.Add(Vein.Shape);
				Test->TestTrue(TEXT("Veins consist of several connected ore cells"), Vein.Cells.Num() >= 3);
				for (const FIntPoint& Cell : Vein.Cells)
				{
					Test->TestTrue(TEXT("Every vein cell touches another ore cell in its pattern"), Vein.Cells.ContainsByPredicate([&](const FIntPoint& Other) { return FMath::Abs(Other.X - Cell.X) + FMath::Abs(Other.Y - Cell.Y) == 1; }));
					Test->TestNotNull(TEXT("Veins occupy actual terrain rather than empty cave space"), Level->GetBlockAtPosition(FVector2D(Cell)));
				}
			}
			Test->TestEqual(TEXT("Branch, cluster and curved vein shapes coexist"), Shapes.Num(), 3);
			Hash = VeinHash(Level->GetGeneratedOreVeins());
			Level->RegenerateMineableBlocks(814);
			Test->TestEqual(TEXT("Ore pattern placement and mineral selection repeat with the seed"), VeinHash(Level->GetGeneratedOreVeins()), Hash);
			Settings->MineableLevelGeneration = Saved;
			return true;
		}
	private:
		FAutomationTestBase* Test;
		double Started;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChestFallUnreadTest, "ProjectMK.PIE.MinePolish.ChestFallAndUnreadHover", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChestFallUnreadTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FChestFallCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIceTractionTest, "ProjectMK.PIE.MinePolish.IceTraction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FIceTractionTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FIceTractionCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVeinFoundationTest, "ProjectMK.PIE.MinePolish.VeinsAndProtectedFoundations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVeinFoundationTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FVeinFoundationCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
