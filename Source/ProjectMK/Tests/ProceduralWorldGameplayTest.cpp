#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PaperTileMapActor.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Monster/MKMonster.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"

namespace
{
	class FProceduralWorldCommand : public IAutomationLatentCommand
	{
	public:
		explicit FProceduralWorldCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			ULevelManagerSubsystem* Level = World ? World->GetSubsystem<ULevelManagerSubsystem>() : nullptr;
			if (Level == nullptr || Level->GetBlocks().IsEmpty())
			{
				if (FPlatformTime::Seconds() - Started < 30.0) { return false; }
				Test->AddError(TEXT("Procedural world did not initialize."));
				return true;
			}
			if (USaveGameSubsystem::Get(World)->StartPlaytestSession() == false) { Test->AddError(TEXT("Could not protect saved progress.")); return true; }
			UDataManager* Data = UDataManager::Get(World);
			UGameSettingDataAsset* Settings = const_cast<UGameSettingDataAsset*>(Data->GetGameSettingDataAsset());
			const FMineableLevelGenerationSettings Saved = Settings->MineableLevelGeneration;
			FMineableLevelGenerationSettings& Generation = Settings->MineableLevelGeneration;
			for (TActorIterator<APaperTileMapActor> It(World); It; ++It) { if (It->ActorHasTag(TEXT("ArtBackground")) == false) { It->Destroy(); } }
			Generation.WidthInBlocks = 32;
			Generation.DepthInBlocks = 96;
			Generation.StartColumn = 48;
			Generation.RockBlockRatio = 0.f;
			Generation.EmptyBlockRatio = 0.f;
			Generation.FallingStructureCount = 0;
			const int32 Surface = Settings->SurfaceBlockPositionY;
			Level->RegenerateMineableBlocks(451);
			auto TileAt = [Level](int32 X, int32 Y)
			{
				const ABlockBase* Block = Level->GetBlockAtPosition(FVector2D(X, Y));
				return Block ? Block->GetTileIndex() : INDEX_NONE;
			};
			const int32 Dirt = Data->GetBlockDataTableRow(TEXT("Dirt"))->TileIndex;
			const int32 Sand = Data->GetBlockDataTableRow(TEXT("Sand"))->TileIndex;
			const int32 Ice = Data->GetBlockDataTableRow(TEXT("Ice"))->TileIndex;
			const int32 Rock = Data->GetBlockDataTableRow(TEXT("Rock"))->TileIndex;
			Test->TestTrue(TEXT("World generation works after deleting the foreground tile map"), Level->GetBlocks().Num() > 3000);
			for (int32 X = 48; X < 80; ++X)
			{
				Test->TestEqual(TEXT("Surface remains a complete grass row"), TileAt(X, Surface), Data->GetBlockDataTableRow(TEXT("Dirt_Grass"))->TileIndex);
				Test->TestEqual(TEXT("Shallow band uses dirt"), TileAt(X, Surface + 10), Dirt);
				Test->TestEqual(TEXT("Middle band uses sand"), TileAt(X, Surface + 48), Sand);
				Test->TestEqual(TEXT("Deep band uses ice"), TileAt(X, Surface + 90), Ice);
			}
			Test->TestEqual(TEXT("Left edge is rock"), TileAt(47, Surface + 20), Rock);
			Test->TestEqual(TEXT("Right edge is rock"), TileAt(80, Surface + 20), Rock);
			Test->TestEqual(TEXT("Bottom edge is rock"), TileAt(60, Surface + 96), Rock);
			TSet<int32> TransitionMaterials;
			for (int32 Y = Surface + 28; Y <= Surface + 36; ++Y) { for (int32 X = 48; X < 80; ++X) { TransitionMaterials.Add(TileAt(X, Y)); } }
			Test->TestTrue(TEXT("Transition region mixes neighboring materials"), TransitionMaterials.Contains(Dirt) && TransitionMaterials.Contains(Sand));
			auto LayoutHash = [&TileAt, Surface]()
			{
				uint32 Hash = 0;
				for (int32 Y = Surface; Y < Surface + 96; ++Y) { for (int32 X = 48; X < 80; ++X) { Hash = HashCombine(Hash, GetTypeHash(TileAt(X, Y))); } }
				return Hash;
			};
			const uint32 FirstHash = LayoutHash();
			Level->RegenerateMineableBlocks(451);
			Test->TestEqual(TEXT("Same seed produces the same terrain"), LayoutHash(), FirstHash);
			Level->RegenerateMineableBlocks(452);
			Test->TestNotEqual(TEXT("Different seed changes transition placement"), LayoutHash(), FirstHash);
			Generation.WidthInBlocks = 64;
			Generation.StartColumn = 24;
			Generation.FallingStructureCount = 6;
			Generation.bGenerateOrePuzzles = true;
			Generation.bGenerateShortcuts = true;
			Generation.bGenerateCombatTraps = true;
			Level->RegenerateMineableBlocks(451);
			TSet<EMineStructurePurpose> Purposes;
			for (const FGeneratedMineStructure& Encounter : Level->GetGeneratedStructures())
			{
				Purposes.Add(Encounter.Purpose);
				Test->TestFalse(TEXT("Every encounter has a reward"), Encounter.RewardCells.IsEmpty());
				for (const FIntPoint& Cell : Encounter.RewardCells) { Test->TestTrue(TEXT("Reward block exists"), TileAt(Cell.X, Cell.Y) != INDEX_NONE); }
				if (Encounter.Purpose == EMineStructurePurpose::CombatTrap)
				{
					Test->TestTrue(TEXT("Combat trap has a monster position"), Encounter.bHasMonster);
					Test->TestEqual(TEXT("Monster stands on trap floor"), TileAt(Encounter.MonsterCell.X, Encounter.MonsterCell.Y + 1), Rock);
				}
			}
			Test->TestEqual(TEXT("Generation includes ore, shortcut, and combat purposes"), Purposes.Num(), 3);
			int32 MonsterCount = 0;
			for (TActorIterator<AMKMonster> It(World); It; ++It) { ++MonsterCount; }
			Test->TestTrue(TEXT("Combat encounter spawns a monster"), MonsterCount > 0);
			Settings->MineableLevelGeneration = Saved;
			return true;
		}

	private:
		FAutomationTestBase* Test;
		double Started;
	};

	class FOxygenFlightCommand : public IAutomationLatentCommand
	{
	public:
		explicit FOxygenFlightCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			if (Player.IsValid() == false)
			{
				UWorld* World = GEditor->PlayWorld;
				Player = World ? Cast<AMKCharacter>(World->GetFirstPlayerController()->GetPawn()) : nullptr;
				if (Player.IsValid() == false)
				{
					if (FPlatformTime::Seconds() - Started < 30.0) { return false; }
					Test->AddError(TEXT("Oxygen fixture player did not initialize.")); return true;
				}
				if (USaveGameSubsystem::Get(World)->StartPlaytestSession() == false) { return true; }
				Player->GetCharacterMovement()->DisableMovement();
				Origin = Player->GetActorLocation();
				Oxygen = World->SpawnActor<AItemBase>(AItemBase::StaticClass(), Origin + FVector(96.f, 0.f, 0.f), FRotator::ZeroRotator);
				Oxygen->InitializeItemBase(TEXT("Oxygen"));
				Oxygen->TryLoot(Player);
				TArray<UPaperSpriteComponent*> Sprites;
				Oxygen->GetComponents(Sprites);
				Test->TestEqual(TEXT("Oxygen uses a core, three trails, and a hidden item sprite"), Sprites.Num(), 5);
				UPaperFlipbookComponent* Glow = Oxygen->FindComponentByClass<UPaperFlipbookComponent>();
				Test->TestNotNull(TEXT("Oxygen uses an animated effect"), Glow);
				const UVisualStyleDataAsset* Style = UDataManager::Get(World)->GetVisualStyleDataAsset();
				Test->TestTrue(TEXT("Oxygen uses the dedicated cyan material"), Glow != nullptr && Glow->GetMaterial(0) == Style->OxygenPickupMaterial);
				Started = FPlatformTime::Seconds();
				FlightStarted = World->GetTimeSeconds();
				return false;
			}
			if (Oxygen.IsValid())
			{
				MaxHeight = FMath::Max(MaxHeight, Oxygen->GetActorLocation().Z - Origin.Z);
				TArray<UPaperSpriteComponent*> Sprites;
				Oxygen->GetComponents(Sprites);
				int32 VisibleSprites = 0;
				for (const UPaperSpriteComponent* Sprite : Sprites) { VisibleSprites += Sprite->IsVisible() ? 1 : 0; }
				bSawTrail |= VisibleSprites >= 3;
			}
			if (Player->GetWorld()->GetTimeSeconds() - FlightStarted < 1.5f && FPlatformTime::Seconds() - Started < 30.0) { return false; }
			Test->TestTrue(TEXT("Oxygen follows a high arc distinct from ordinary loot"), MaxHeight > 40.f);
			Test->TestTrue(TEXT("Visible afterimages follow the oxygen core during flight"), bSawTrail);
			Test->TestFalse(TEXT("Oxygen is collected at the end of its flight"), Oxygen.IsValid());
			if (Oxygen.IsValid()) { Oxygen->Destroy(); }
			Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			return true;
		}

	private:
		FAutomationTestBase* Test;
		double Started;
		TWeakObjectPtr<AMKCharacter> Player;
		TWeakObjectPtr<AItemBase> Oxygen;
		FVector Origin;
		float MaxHeight = 0.f;
		float FlightStarted = 0.f;
		bool bSawTrail = false;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProceduralWorldTest, "ProjectMK.PIE.World.ProceduralDepthAndRewardEncounters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProceduralWorldTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FProceduralWorldCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOxygenFlightTest, "ProjectMK.PIE.World.OxygenWispFlight", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOxygenFlightTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FOxygenFlightCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
