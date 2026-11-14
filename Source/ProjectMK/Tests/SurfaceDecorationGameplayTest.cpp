#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PaperSpriteActor.h"
#include "PaperSpriteComponent.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"

namespace
{
	class FSurfaceDecorationCommand : public IAutomationLatentCommand
	{
	public:
		explicit FSurfaceDecorationCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			ULevelManagerSubsystem* Level = World != nullptr ? World->GetSubsystem<ULevelManagerSubsystem>() : nullptr;
			if (Level == nullptr || Level->GetBlocks().IsEmpty()) { return FPlatformTime::Seconds() - Started > 25.0; }
			USaveGameSubsystem::Get(World)->StartPlaytestSession();
			UDataManager* Data = UDataManager::Get(World);
			UGameSettingDataAsset* Settings = const_cast<UGameSettingDataAsset*>(Data->GetGameSettingDataAsset());
			const FMineableLevelGenerationSettings Saved = Settings->MineableLevelGeneration;
			auto& Generation = Settings->MineableLevelGeneration;
			Generation.WidthInBlocks = 96;
			Generation.DepthInBlocks = 32;
			Generation.StartColumn = 16;
			Generation.SurfaceReliefHeight = 2;
			Generation.EmptyBlockRatio = 0.f;
			Generation.RockBlockRatio = 0.f;
			Generation.FallingStructureCount = 0;
			Generation.bGenerateDecorations = true;
			Level->RegenerateMineableBlocks(731);
			const int32 Surface = Settings->SurfaceBlockPositionY;
			const int32 Grass = Data->GetBlockDataTableRow(TEXT("Dirt_Grass"))->TileIndex;
			TSet<int32> Heights;
			uint32 HeightHash = 0;
			for (int32 X = 16; X < 112; ++X)
			{
				const int32 Top = Level->GetSurfaceRow(X);
				Heights.Add(Top);
				HeightHash = HashCombine(HeightHash, GetTypeHash(Top));
				if (Top < Surface) { Test->TestTrue(TEXT("Every raised tier joins neighboring terrain instead of a single pillar"), (X > 16 && Level->GetSurfaceRow(X - 1) <= Top) || (X < 111 && Level->GetSurfaceRow(X + 1) <= Top)); }
				if (X > 16) { Test->TestTrue(TEXT("Adjacent surface columns change by at most one block"), FMath::Abs(Top - Level->GetSurfaceRow(X - 1)) <= 1); }
				ABlockBase* TopBlock = Level->GetBlockAtPosition(FVector2D(X, Top));
				Test->TestTrue(TEXT("Each surface column has a complete grass top"), TopBlock != nullptr && TopBlock->GetTileIndex() == Grass);
				for (int32 Y = Top; Y <= Surface + 1; ++Y) { Test->TestNotNull(TEXT("Surface relief never leaves holes in its supporting ground"), Level->GetBlockAtPosition(FVector2D(X, Y))); }
			}
			Test->TestEqual(TEXT("The authored surface includes broad one-block and two-block rises"), Heights.Num(), 3);
			for (int32 X = Generation.ShopColumn - 6; X <= Generation.ShopColumn + 6; ++X) { Test->TestEqual(TEXT("The shop frontage stays flat"), Level->GetSurfaceRow(X), Surface); }
			int32 DecorationCount = 0;
			for (TActorIterator<APaperSpriteActor> It(World); It; ++It)
			{
				if (It->ActorHasTag(TEXT("MineDecoration")) == false) { continue; }
				++DecorationCount;
				Test->TestFalse(TEXT("Mine scenery never collides with gameplay"), It->GetActorEnableCollision());
				Test->TestTrue(TEXT("Scenery is supported by an actual terrain block"), Cast<ABlockBase>(It->GetAttachParentActor()) != nullptr);
				const UPaperSprite* Sprite = It->GetRenderComponent()->GetSprite();
				Test->TestTrue(TEXT("Props use the terrain's native pixel density"), Sprite != nullptr && FMath::IsNearlyEqual(Sprite->GetPixelsPerUnrealUnit(), 2.f));
			}
			Test->TestTrue(TEXT("The world displays multiple mining props"), DecorationCount >= 6);
			Level->RegenerateMineableBlocks(732);
			uint32 RepeatedHash = 0;
			for (int32 X = 16; X < 112; ++X) { RepeatedHash = HashCombine(RepeatedHash, GetTypeHash(Level->GetSurfaceRow(X))); }
			Test->TestEqual(TEXT("Authored surface heights stay fixed even when the underground seed changes"), RepeatedHash, HeightHash);
			Settings->MineableLevelGeneration = Saved;
			return true;
		}

	private:
		FAutomationTestBase* Test;
		double Started;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurfaceDecorationTest, "ProjectMK.PIE.World.SurfaceReliefAndMineDecoration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurfaceDecorationTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FSurfaceDecorationCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
