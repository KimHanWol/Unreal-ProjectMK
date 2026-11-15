#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Monster/MKMonster.h"
#include "ProjectMK/Actor/Shop/MineFacilityActor.h"
#include "ProjectMK/Actor/Shop/StorageChestActor.h"
#include "ProjectMK/Actor/Transport/MineCartActor.h"
#include "ProjectMK/Actor/Transport/RailTrackActor.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/MKSaveGame.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/ForestrySubsystem.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/MineTransportSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/Core/Subsystem/TownProgressSubsystem.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"
#include "ProjectMK/UI/MKTooltipBase.h"

namespace
{
	class FMineExpansionCommand : public IAutomationLatentCommand
	{
	public:
		FMineExpansionCommand(FAutomationTestBase* InTest, int32 InMode) : Test(InTest), Mode(InMode), Started(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (Step == 0)
			{
				World = GEditor->PlayWorld;
				Player = World != nullptr ? Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)) : nullptr;
				if (Player == nullptr) { if (FPlatformTime::Seconds() - Started > 30.0) { Test->AddError(TEXT("Expansion fixture timed out")); return true; } return false; }
				USaveGameSubsystem::Get(World)->StartPlaytestSession();
				Settings = const_cast<UGameSettingDataAsset*>(UDataManager::Get(World)->GetGameSettingDataAsset());
				SavedGeneration = Settings->MineableLevelGeneration; SavedTown = Settings->TownFacilities; SavedForestry = Settings->Forestry;
				Level = World->GetSubsystem<ULevelManagerSubsystem>(); Town = World->GetSubsystem<UTownProgressSubsystem>(); Forest = World->GetSubsystem<UForestrySubsystem>(); Network = World->GetSubsystem<UMineTransportSubsystem>();
				Progress.Reset(NewObject<UMKSaveGame>()); Town->WriteSaveData(Progress.Get());
				TStrongObjectPtr<UMKSaveGame> Empty(NewObject<UMKSaveGame>()); Town->RestoreProgress(Empty.Get());
				auto& Generation = Settings->MineableLevelGeneration;
				Generation.StartColumn = 0; Generation.WidthInBlocks = 128; Generation.DepthInBlocks = 24; Generation.SurfaceReliefHeight = 0; Generation.FallingStructureCount = 0;
				Generation.EmptyBlockRatio = 0.f; Generation.RockBlockRatio = 0.f; Generation.bGenerateDecorations = false; Generation.BuriedChestMaxCount = 0;
				Settings->Forestry.bEnabled = Mode == 1; Settings->Forestry.RegrowSeconds = 2;
				Level->RegenerateMineableBlocks(1541);
				Player->GetCharacterMovement()->DisableMovement(); Player->SetActorLocation(FVector(40.5f * 16.f, -20.f, -Settings->SurfaceBlockPositionY * 16.f + 10.f));
				Player->FindComponentByClass<UInventoryComponent>()->SetItemCollectionActive(false);
				Surface = Settings->SurfaceBlockPositionY;
				if (Mode == 0)
				{
					for (const auto& Facility : Town->GetFacilities()) { if (Facility.IsValid() && Facility->GetFacilityKind() == EMineFacilityKind::QuestBoard) { Board = Facility; } }
					if (Test->TestNotNull(TEXT("The noticeboard is an actual interactable facility"), Board.Get()) == false) { return Finish(); }
					Player->SetActorLocation(Board->GetActorLocation() + FVector(0.f, 0.f, -2.96f));
					Settings->TownFacilities.HuntQuests[0].RequiredCount = 2;
					Town->BeginResidentConversation(Player, Board.Get());
					Test->TestTrue(TEXT("The noticeboard offers a contract immediately"), Town->TryQuestAction(Player, Board.Get()));
					Test->TestFalse(TEXT("A contract cannot reward before kills"), Town->CanUseQuestAction(Board.Get(), Player));
					Town->NotifyMonsterDefeated(TEXT("UnknownMonster"));
					AMKMonster* Removed = World->SpawnActor<AMKMonster>(AMKMonster::StaticClass(), Player->GetActorLocation() + FVector(60.f, 0.f, 0.f), FRotator::ZeroRotator); Removed->Destroy();
					TStrongObjectPtr<UMKSaveGame> Before(NewObject<UMKSaveGame>()); Town->WriteSaveData(Before.Get());
					Test->TestEqual(TEXT("Unrelated keys and actor cleanup never count as a kill"), Before->TownQuests[2].MonsterKillProgress, 0);
					AMKMonster* Killed = World->SpawnActor<AMKMonster>(AMKMonster::StaticClass(), Player->GetActorLocation() + FVector(60.f, 0.f, 0.f), FRotator::ZeroRotator);
					FGameplayAbilityUtils::ApplyDamage(Killed->GetAbilitySystemComponent(), nullptr, 1000.f);
					TStrongObjectPtr<UMKSaveGame> Snapshot(NewObject<UMKSaveGame>()); Town->WriteSaveData(Snapshot.Get());
					Test->TestEqual(TEXT("Actual death records exactly one kill"), Snapshot->TownQuests[2].MonsterKillProgress, 1);
					TArray<uint8> Bytes; UGameplayStatics::SaveGameToMemory(Snapshot.Get(), Bytes);
					TStrongObjectPtr<UMKSaveGame> Reloaded(CastChecked<UMKSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes))); Town->RestoreProgress(Reloaded.Get());
					Test->TestEqual(TEXT("The active hunt survives a real save serialization"), Reloaded->TownQuests[2].MonsterKillProgress, 1);
					FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					FallingMonster = World->SpawnActor<AMKMonster>(AMKMonster::StaticClass(), FVector(40.5f * 16.f, -20.f, -Surface * 16.f + 120.f), FRotator::ZeroRotator, Params);
					ChangedAt = World->GetTimeSeconds(); Step = 1; return false;
				}
				if (Mode == 1)
				{
					Test->TestEqual(TEXT("Three configured lumber stands grow"), Forest->GetTrees().Num(), 3);
					if (Forest->GetTrees().IsEmpty() || Forest->GetTrees()[0].Pieces.IsEmpty()) { Test->AddError(TEXT("Tree blocks did not grow")); return Finish(); }
					OriginalPieces = Forest->GetTrees()[0].Pieces.Num();
					Forest->GetTrees()[0].Pieces.Last()->Destroy();
					ChangedAt = World->GetTimeSeconds(); Step = 1; return false;
				}
				if (Mode == 2)
				{
					for (int32 X = 43; X <= 45; ++X) { for (int32 Y = Surface; Y < Surface + X - 42; ++Y) { if (ABlockBase* Block = Level->GetBlockAtPosition(FVector2D(X, Y))) { Block->Destroy(); } } }
					for (int32 X = 42; X <= 44; ++X)
					{
						Player->SetActorLocation(FVector((X + 0.5f) * 16.f, -20.f, -(Surface + X - 42) * 16.f + 9.04f));
						Test->TestTrue(TEXT("Normal placement joins the previous upper-left endpoint"), Network->TryPlaceRail(Player));
					}
					if (Network->GetRails().Num() != 3) { Test->AddError(TEXT("The stepped track did not place all three pieces")); return Finish(); }
					const ARailTrackActor* First = Network->GetRails()[0].Get(); const ARailTrackActor* Slope = Network->GetRails()[1].Get();
					Test->TestTrue(TEXT("Flat rail spans one complete block from boundary to boundary"), First->GetSaveData().Start == FIntPoint(42, Surface) && First->GetSaveData().End == FIntPoint(43, Surface) && FMath::IsNearlyEqual((First->GetEndWorld() - First->GetStartWorld()).Size(), 16.f));
					Test->TestTrue(TEXT("Lower-right placement creates the descending diagonal"), Slope->GetSaveData().Start == FIntPoint(43, Surface) && Slope->GetSaveData().End == FIntPoint(44, Surface + 1));
					TArray<UPaperSpriteComponent*> Sprites; Slope->GetComponents(Sprites);
					UPaperSpriteComponent* RailSprite = *Sprites.FindByPredicate([](const auto* Sprite) { return Sprite->GetName() == TEXT("Rail"); });
					const FVector VisualAxis = RailSprite->GetComponentTransform().TransformVector(FVector::ForwardVector).GetSafeNormal();
					Test->TestTrue(TEXT("Rendered sprite axis follows the real downhill travel direction"), VisualAxis.Equals((Slope->GetEndWorld() - Slope->GetStartWorld()).GetSafeNormal(), 0.001f));
					// Reverse construction: put the right flat first, then approach from one lower cell on the left.
					Test->TestTrue(TEXT("The right-hand starter rail places"), Network->TryPlaceRailSegment(FIntPoint(85, Surface), FIntPoint(86, Surface)));
					if (auto* Block = Level->GetBlockAtPosition(FVector2D(84, Surface))) { Block->Destroy(); }
					Player->SetActorLocation(FVector(84.5f * 16.f, -20.f, -(Surface + 1) * 16.f + 9.04f));
					Test->TestTrue(TEXT("Normal placement also joins from the opposite direction"), Network->TryPlaceRail(Player));
					Test->TestTrue(TEXT("The reverse slope meets the upper-right neighbor without protrusion"), Network->GetRails().Last()->GetSaveData().Start == FIntPoint(84, Surface + 1) && Network->GetRails().Last()->GetSaveData().End == FIntPoint(85, Surface));
					Player->SetActorLocation(FVector(40.5f * 16.f, -20.f, -Surface * 16.f + 20.f));
					Test->TestTrue(TEXT("A cart installs on the test line before support removal"), Network->TryPlaceCart(Player, false));
					if (Network->GetCarts().IsEmpty() == false)
					{
						AStorageChestActor* Cargo = Level->SpawnMountedStorageChest(TEXT("StorageChest"), Network->GetCarts()[0].Get());
						if (Test->TestNotNull(TEXT("A storage chest mounts on the doomed cart"), Cargo))
						{
							FInventorySlotData Ore; Ore.ItemUID = TEXT("Ore_Iron"); Ore.ItemCount = 5; Cargo->ApplySavedContents({ Ore });
						}
					}
					for (int32 X = 41; X <= 46; ++X) { for (int32 Y = Surface; Y <= Surface + 4; ++Y) { if (auto* Block = Level->GetBlockAtPosition(FVector2D(X, Y))) { Block->Destroy(); } } }
					ChangedAt = World->GetTimeSeconds(); Step = 1; return false;
				}
				if (Mode == 3)
				{
					Tooltip.Reset(CreateWidget<UMKTooltipBase>(World->GetFirstPlayerController(), UMKTooltipBase::StaticClass()));
					Tooltip->AddToViewport(2000); Tooltip->SetDesiredSizeInViewport(FVector2D(360.f, 380.f));
					Cycles = 0; Step = 1; return false;
				}
			}
			const float Elapsed = World->GetTimeSeconds() - ChangedAt;
			if (Mode == 0 && Elapsed > 2.f)
			{
				Test->TestFalse(TEXT("A natural six-block fall kills the monster"), FallingMonster.IsValid());
				Test->TestTrue(TEXT("The fall kill completes the same board contract"), Town->CanUseQuestAction(Board.Get(), Player));
				Test->TestTrue(TEXT("A completed hunt grants its reward"), Town->TryQuestAction(Player, Board.Get()));
				Test->TestFalse(TEXT("The same contract cannot be claimed twice"), Town->TryQuestAction(Player, Board.Get()));
				return Finish();
			}
			if (Mode == 1 && Step == 1 && Elapsed > 3.2f)
			{
				Test->TestEqual(TEXT("A partially cut tree does not regrow after the delay"), Forest->GetSaveData()[0].Pieces.Num(), OriginalPieces - 1);
				TStrongObjectPtr<UMKSaveGame> Snapshot(NewObject<UMKSaveGame>()); Snapshot->Trees = Forest->GetSaveData();
				TArray<uint8> Bytes; UGameplayStatics::SaveGameToMemory(Snapshot.Get(), Bytes);
				TStrongObjectPtr<UMKSaveGame> Reloaded(CastChecked<UMKSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes))); Forest->RestoreTrees(Reloaded->Trees);
				Test->TestEqual(TEXT("Partial tree state survives serialization and reconstruction"), Forest->GetSaveData()[0].Pieces.Num(), OriginalPieces - 1);
				const auto Pieces = Forest->GetTrees()[0].Pieces;
				FGameplayAbilityUtils::ApplyDamageToDurability(Pieces[0]->GetAbilitySystemComponent(), nullptr, 1000.f);
				ChangedAt = World->GetTimeSeconds(); Step = 2; return false;
			}
			if (Mode == 1 && Step == 2 && Elapsed > 2.5f)
			{
				int32 WoodCount = 0; for (TActorIterator<AItemBase> It(World); It; ++It) { if (It->GetItemKey() == TEXT("Wood")) { WoodCount += It->GetItemCount(); } }
				Test->TestTrue(TEXT("Manual trunk mining produces actual Wood drops"), WoodCount >= 2);
				int32 Leaves = 0; const int32 LeafTile = UDataManager::Get(World)->GetBlockDataTableRow(TEXT("TreeLeaves"))->TileIndex;
				for (const auto& Block : Forest->GetTrees()[0].Pieces) { if (Block.IsValid() && Block->GetTileIndex() == LeafTile && Block->IsBreaking() == false) { ++Leaves; } }
				Test->TestEqual(TEXT("Unsupported weak leaves fall and shatter"), Leaves, 0);
				Test->TestNotNull(TEXT("Tree collapse preserves the root soil"), Level->GetBlockAtPosition(FVector2D(Forest->GetTrees()[0].Column, Surface)));
				for (const auto& Block : Forest->GetTrees()[0].Pieces) { if (Block.IsValid()) { Block->Destroy(); } }
				ChangedAt = World->GetTimeSeconds(); Step = 3; return false;
			}
			if (Mode == 1 && Step == 3 && Elapsed > 3.5f)
			{
				Test->TestEqual(TEXT("A fully cleared tree regrows after the configured delay"), Forest->GetSaveData()[0].Pieces.Num(), OriginalPieces);
				const auto* Recipe = UDataManager::Get(World)->GetShopRecipeDataTableRow(TEXT("RailRecipe"));
				Test->TestTrue(TEXT("Transport crafting uses harvested wood"), Recipe != nullptr && Recipe->RequiredItems.ContainsByPredicate([](const auto& Ingredient) { return Ingredient.ItemKey == TEXT("Wood"); }));
				return Finish();
			}
			if (Mode == 2 && Elapsed > 0.5f)
			{
				Test->TestEqual(TEXT("Every unsupported rail collapses while the independent line survives"), Network->GetRails().Num(), 2);
				int32 Drops = 0; for (TActorIterator<AItemBase> It(World); It; ++It) { if (It->GetItemKey() == TEXT("Rail")) { Drops += It->GetItemCount(); } }
				Test->TestEqual(TEXT("Collapsed pieces become reusable rail drops"), Drops, 3);
				Test->TestTrue(TEXT("A cart cannot remain floating above missing track"), Network->GetCarts().IsEmpty());
				TMap<FName, int32> CargoDrops;
				for (TActorIterator<AItemBase> It(World); It; ++It) { CargoDrops.FindOrAdd(It->GetItemKey()) += It->GetItemCount(); }
				Test->TestEqual(TEXT("The unsupported cart is returned as an item"), CargoDrops.FindRef(TEXT("MineCart")), 1);
				Test->TestEqual(TEXT("The mounted chest becomes a placeable drop"), CargoDrops.FindRef(TEXT("StorageChest")), 1);
				Test->TestEqual(TEXT("All stored cargo drops without being lost"), CargoDrops.FindRef(TEXT("Ore_Iron")), 5);
				return Finish();
			}
			if (Mode == 3)
			{
				if (Step == 2)
				{
					// Keep the removed widget alive until Slate has painted its removal before the next PIE teardown.
					if (++Cycles < 123) { return false; }
					Tooltip.Reset(); return Finish();
				}
				if (Cycles % 3 == 0) { Tooltip->ShowRecipeTooltip(TEXT("RailRecipe")); }
				else if (Cycles % 3 == 1) { Tooltip->ShowRecipeTooltip(TEXT("AutoMineCartRecipe"), true); }
				else { Tooltip->ShowItemTooltip(TEXT("Wood")); }
				TSet<UWidget*> Visited;
				bool bRepeatedWidget = false;
				Tooltip->WidgetTree->ForEachWidget([&](UWidget* Widget) { if (Visited.Contains(Widget)) { Test->AddInfo(FString::Printf(TEXT("Aliased tooltip widget %s %p"), *Widget->GetPathName(), Widget)); bRepeatedWidget = true; } Visited.Add(Widget); });
				if (Test->TestFalse(TEXT("Every tooltip branch owns distinct widgets"), bRepeatedWidget) == false) { Tooltip->RemoveFromParent(); Tooltip.Reset(); return Finish(); }
				CollectGarbage(RF_NoFlags);
				bool bInvalidResource = false;
				Tooltip->WidgetTree->ForEachWidget([&](UWidget* Widget) { if (const auto* Image = Cast<UImage>(Widget)) { UObject* Resource = Image->GetBrush().GetResourceObject(); bInvalidResource |= Resource != nullptr && ::IsValid(Resource) == false; } });
				Test->TestFalse(TEXT("Visible tooltip image resources survive forced garbage collection"), bInvalidResource);
				if (++Cycles >= 120) { Tooltip->HideTooltip(); Tooltip->RemoveFromParent(); Step = 2; return false; }
			}
			if (FPlatformTime::Seconds() - Started > 90.0) { Test->AddError(TEXT("Expansion scenario timed out")); return Finish(); }
			return false;
		}
	private:
		bool Finish()
		{
			Settings->MineableLevelGeneration = SavedGeneration; Settings->TownFacilities = SavedTown; Settings->Forestry = SavedForestry; Town->RestoreProgress(Progress.Get()); return true;
		}
		FAutomationTestBase* Test; int32 Mode; double Started; int32 Step = 0; float ChangedAt = 0.f; int32 Surface = 0; int32 OriginalPieces = 0; int32 Cycles = 0;
		UWorld* World = nullptr; AMKCharacter* Player = nullptr; UGameSettingDataAsset* Settings = nullptr; ULevelManagerSubsystem* Level = nullptr; UTownProgressSubsystem* Town = nullptr; UForestrySubsystem* Forest = nullptr; UMineTransportSubsystem* Network = nullptr;
		FMineableLevelGenerationSettings SavedGeneration; FTownFacilitySettings SavedTown; FForestrySettings SavedForestry;
		TStrongObjectPtr<UMKSaveGame> Progress; TStrongObjectPtr<UMKTooltipBase> Tooltip; TWeakObjectPtr<AMineFacilityActor> Board; TWeakObjectPtr<AMKMonster> FallingMonster;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoardHuntTest, "ProjectMK.PIE.Expansion.BoardHuntAndMonsterFall", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBoardHuntTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false)); ADD_LATENT_AUTOMATION_COMMAND(FMineExpansionCommand(this, 0)); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestryTest, "ProjectMK.PIE.Expansion.HarvestCollapseRegrowthAndSave", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FForestryTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false)); ADD_LATENT_AUTOMATION_COMMAND(FMineExpansionCommand(this, 1)); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailBoundaryTest, "ProjectMK.PIE.Expansion.NeighborRailGeometryAndCollapse", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRailBoundaryTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false)); ADD_LATENT_AUTOMATION_COMMAND(FMineExpansionCommand(this, 2)); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTooltipResourceTest, "ProjectMK.PIE.Expansion.TooltipGarbageCollectionStress", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTooltipResourceTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false)); ADD_LATENT_AUTOMATION_COMMAND(FMineExpansionCommand(this, 3)); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
