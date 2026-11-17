#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "ProjectMK/UI/BountyNoticeWidget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSprite.h"
#include "PaperSpriteActor.h"
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
				Settings->Forestry.bEnabled = Mode == 1 || Mode == 4; Settings->Forestry.RegrowSeconds = 2;
				if (Mode == 4) { Settings->Forestry.GrowthStageSeconds = 1; }
				Level->RegenerateMineableBlocks(1541);
				Player->GetCharacterMovement()->DisableMovement(); Player->SetActorLocation(FVector(40.5f * 16.f, -20.f, -Settings->SurfaceBlockPositionY * 16.f + 10.f));
				Player->FindComponentByClass<UInventoryComponent>()->SetItemCollectionActive(false);
				Surface = Settings->SurfaceBlockPositionY;
				if (Mode == 0)
				{
					for (const auto& Facility : Town->GetFacilities()) { if (Facility.IsValid() && Facility->GetFacilityKind() == EMineFacilityKind::QuestBoard) { Board = Facility; } }
					if (Test->TestNotNull(TEXT("The noticeboard is an actual interactable facility"), Board.Get()) == false) { return Finish(); }
					Player->SetActorLocation(Board->GetActorLocation() + FVector(0.f, 0.f, -2.96f));
					const int64 BoardNow = FDateTime::UtcNow().ToUnixTimestamp();
					Settings->TownFacilities.BoardOfferChance = 0.f;
					Town->UpdateBoardOffers(BoardNow);
					for (int32 Slot=0;Slot<4;++Slot) { Test->TestNull(TEXT("An empty board has no fallback notice"),Town->GetBoardQuestDefinition(Slot)); }
					for (auto& Quest : Settings->TownFacilities.HuntQuests) { Quest.RequiredCount=2; Quest.MinimumDepth=0; }
					Settings->TownFacilities.HuntQuests[3].MinimumDepth=9999;
					Settings->TownFacilities.BoardOfferChance=1.f; Town->ClearQuestCooldownsForPlaytest(); Town->UpdateBoardOffers(BoardNow);
					int32 Posted=0;
					for (int32 Slot=0;Slot<4;++Slot) { const auto* Quest=Town->GetBoardQuestDefinition(Slot); if(Quest){++Posted;Test->TestEqual(TEXT("Every posted contract already meets its depth condition"),Quest->MinimumDepth,0);} }
					Test->TestEqual(TEXT("Ineligible contracts leave genuinely empty spaces"),Posted,3);
					Settings->TownFacilities.HuntQuests[3].MinimumDepth=0; Town->ClearQuestCooldownsForPlaytest(); Town->UpdateBoardOffers(BoardNow);
					Town->BeginResidentConversation(Player, Board.Get());
					Test->TestTrue(TEXT("The noticeboard offers a contract immediately"), Town->TryQuestAction(Player, Board.Get()));
					Settings->TownFacilities.HuntQuests[1].MinimumDepth=0;
					Test->TestTrue(TEXT("A second paper can accept an independent contract"),Town->TryQuestAction(Player,Board.Get(),1));
					Test->TestTrue(TEXT("Another notice can be accepted before abandoning it"),Town->TryQuestAction(Player,Board.Get(),3));
					Test->TestTrue(TEXT("An accepted bounty has an explicit abandon action"),Town->TryAbandonBoardQuest(Player,Board.Get(),3));
					Test->TestNull(TEXT("Abandoning removes that paper without a default replacement"),Town->GetBoardQuestDefinition(3));
					Town->UpdateBoardOffers(BoardNow+1000);
					Test->TestNotNull(TEXT("Accepted first notice never expires"),Town->GetBoardQuestDefinition(0));
					Test->TestNotNull(TEXT("Accepted second notice never expires"),Town->GetBoardQuestDefinition(1));
					Test->TestNull(TEXT("An ignored notice expires without user interaction"),Town->GetBoardQuestDefinition(2));
					TArray<UUserWidget*> HUDs; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,HUDs,UHUDWidget::StaticClass(),false);
					Test->TestTrue(TEXT("The dedicated board popup opens"),CastChecked<UHUDWidget>(HUDs[0])->OpenFacility(Board.Get()));
					TArray<UUserWidget*> Papers; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Papers,UBountyNoticeWidget::StaticClass(),false);
					Test->TestEqual(TEXT("Four reusable paper widgets are present"),Papers.Num(),4);
					FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("AgentLogs/bounty-board-revision.png"),true,false);
					Test->TestFalse(TEXT("A contract cannot reward before kills"), Town->CanUseQuestAction(Board.Get(), Player));
					Town->NotifyMonsterDefeated(TEXT("UnknownMonster"));
					AMKMonster* Removed = World->SpawnActor<AMKMonster>(AMKMonster::StaticClass(), Player->GetActorLocation() + FVector(60.f, 0.f, 0.f), FRotator::ZeroRotator); Removed->Destroy();
					TStrongObjectPtr<UMKSaveGame> Before(NewObject<UMKSaveGame>()); Town->WriteSaveData(Before.Get());
					Test->TestEqual(TEXT("Unrelated keys and actor cleanup never count as a kill"), Before->TownQuests[2].MonsterKillProgress, 0);
					AMKMonster* Killed = World->SpawnActor<AMKMonster>(AMKMonster::StaticClass(), Player->GetActorLocation() + FVector(60.f, 0.f, 0.f), FRotator::ZeroRotator);
					FGameplayAbilityUtils::ApplyDamage(Killed->GetAbilitySystemComponent(), nullptr, 1000.f);
					TStrongObjectPtr<UMKSaveGame> Snapshot(NewObject<UMKSaveGame>()); Town->WriteSaveData(Snapshot.Get());
					Test->TestEqual(TEXT("Actual death records exactly one kill"), Snapshot->TownQuests[2].MonsterKillProgress, 1);
					Test->TestEqual(TEXT("The second paper tracks that same real kill independently"),Snapshot->TownQuests[3].MonsterKillProgress,1);
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
					int32 Beds = 0, Lamps = 0, Sheds = 0;
					for (TActorIterator<APaperSpriteActor> It(World); It; ++It)
					{
						if (It->ActorHasTag(TEXT("LumberyardScenery")) == false) { continue; }
						Beds += It->ActorHasTag(TEXT("LumberyardEnclosure")); Lamps += It->ActorHasTag(TEXT("LanternPost")); Sheds += It->ActorHasTag(TEXT("Lumberyard"));
						Test->TestFalse(TEXT("Yard scenery never obstructs the player"), It->GetActorEnableCollision());
					}
					Test->TestTrue(TEXT("The left lumberyard has a single continuous enclosure, a right-side shed and two entrance lamps"), Beds == 1 && Lamps == 2 && Sheds == 1);
					if (Forest->GetTrees().IsEmpty() || Forest->GetTrees()[0].Pieces.IsEmpty()) { Test->AddError(TEXT("Tree blocks did not grow")); return Finish(); }
					Test->TestEqual(TEXT("Trees begin with one trunk and one leaf"),Forest->GetTrees()[0].Pieces.Num(),2);
					Test->TestEqual(TEXT("New trees start at their first growth stage"),Forest->GetTrees()[0].GrowthStage,1);
					for (int32 Stage=2;Stage<=4;++Stage) { Forest->UpdateTrees(FDateTime::UtcNow().ToUnixTimestamp()+Stage*Settings->Forestry.GrowthStageSeconds); Test->TestEqual(TEXT("Growth advances by one stage at a time"),Forest->GetTrees()[0].GrowthStage,Stage); }
					int32 MatureTrunks=0;
					const auto MatureSave = Forest->GetSaveData();
					for (const auto& Piece:MatureSave[0].Pieces) { MatureTrunks+=Piece.BlockKey==TEXT("TreeTrunk"); }
					Test->TestEqual(TEXT("A mature tree has four trunk cells"),MatureTrunks,4);
					Test->TestTrue(TEXT("Trunks withstand substantially more drilling"),UDataManager::Get(World)->GetBlockDataTableRow(TEXT("TreeTrunk"))->BlockDurability>=120.f);
					OriginalPieces = Forest->GetTrees()[0].Pieces.Num();
					UPaperSprite* FirstLeaf = nullptr;
					const auto CanopySnapshot = Forest->GetSaveData();
					for (const auto& Piece : CanopySnapshot[0].Pieces)
					{
						if (Piece.BlockKey.ToString().StartsWith(TEXT("TreeLeaves_")) == false) { continue; }
						UPaperSprite* Sprite = UDataManager::Get(World)->GetBlockDataTableRow(Piece.BlockKey)->TileSprite.LoadSynchronous();
						if (FirstLeaf == nullptr) { FirstLeaf = Sprite; }
						Test->TestTrue(TEXT("Canopy cells share one continuous source image at native block size"), Sprite != nullptr && Sprite->GetSourceTexture() == FirstLeaf->GetSourceTexture() && Sprite->GetSourceSize().Equals(FVector2D(32.f,32.f)));
					}
					Test->TestNotNull(TEXT("The connected canopy uses atlas pieces"), FirstLeaf);
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
				if (Mode == 4)
				{
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
				TArray<UUserWidget*> Papers; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Papers,UBountyNoticeWidget::StaticClass(),false);
				if (Test->TestEqual(TEXT("Four independent notice widgets remain available"),Papers.Num(),4))
				{
					TArray<UUserWidget*> Visible;
					for (auto* Paper : Papers)
					{
						if (Paper->GetVisibility()==ESlateVisibility::Collapsed) { continue; }
						Visible.Add(Paper);
						Test->TestTrue(TEXT("Every visible paper receives a real cell area"),Paper->GetCachedGeometry().GetLocalSize().X>250.f && Paper->GetCachedGeometry().GetLocalSize().Y>150.f);
					}
					for (int32 I=0;I<Visible.Num();++I) { for (int32 J=I+1;J<Visible.Num();++J) { Test->TestTrue(TEXT("Independent papers never overlap at the same origin"),(Visible[I]->GetCachedGeometry().GetAbsolutePosition()-Visible[J]->GetCachedGeometry().GetAbsolutePosition()).Size()>100.f); } }
				}
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
				// Simulate a stored tree whose planting platform moved up by one block.
				++Snapshot->Trees[0].GroundRow;
				for (auto& Piece : Snapshot->Trees[0].Pieces) { Piece.Location.Z -= 16.f; }
				TArray<uint8> Bytes; UGameplayStatics::SaveGameToMemory(Snapshot.Get(), Bytes);
				TStrongObjectPtr<UMKSaveGame> Reloaded(CastChecked<UMKSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes))); Forest->RestoreTrees(Reloaded->Trees);
				Test->TestEqual(TEXT("Partial tree state survives serialization and reconstruction"), Forest->GetSaveData()[0].Pieces.Num(), OriginalPieces - 1);
				for (int32 I = 0; I < Reloaded->Trees[0].Pieces.Num(); ++I) { Test->TestEqual(TEXT("Saved canopy region identity is retained"), Forest->GetSaveData()[0].Pieces[I].BlockKey, Reloaded->Trees[0].Pieces[I].BlockKey); }
				for (TActorIterator<APaperSpriteActor> It(World); It; ++It) { if (It->ActorHasTag(TEXT("VillageScenery"))) { ParallaxSample = *It; BeforeParallax = It->GetActorLocation(); break; } }
				Player->SetActorLocation(Player->GetActorLocation() + FVector(32.f, 0.f, 0.f));
				const auto Pieces = Forest->GetTrees()[0].Pieces;
				FGameplayAbilityUtils::ApplyDamageToDurability(Pieces[0]->GetAbilitySystemComponent(), nullptr, 1000.f);
				ChangedAt = World->GetTimeSeconds(); Step = 2; return false;
			}
			if (Mode == 1 && Step == 2 && Elapsed > 2.5f)
			{
				int32 WoodCount = 0; for (TActorIterator<AItemBase> It(World); It; ++It) { if (It->GetItemKey() == TEXT("Wood")) { WoodCount += It->GetItemCount(); } }
				Test->TestTrue(TEXT("Manual trunk mining produces actual Wood drops"), WoodCount >= 2);
				int32 Leaves = 0; const int32 TrunkTile = UDataManager::Get(World)->GetBlockDataTableRow(TEXT("TreeTrunk"))->TileIndex;
				for (const auto& Block : Forest->GetTrees()[0].Pieces) { if (Block.IsValid() && Block->GetTileIndex() != TrunkTile && Block->IsBreaking() == false) { ++Leaves; } }
				Test->TestEqual(TEXT("Unsupported weak leaves fall and shatter"), Leaves, 0);
				int32 Trunks = 0;
				for (const auto& Block : Forest->GetTrees()[0].Pieces) { Trunks += Block.IsValid() && Block->IsBreaking()==false && Block->GetTileIndex()==TrunkTile; }
				Test->TestEqual(TEXT("The falling upper tree clears together when it hits the ground"),Trunks,0);
				Test->TestTrue(TEXT("Town backdrops remain fixed when the player moves"), ParallaxSample.IsValid() && ParallaxSample->GetActorLocation().Equals(BeforeParallax,0.01f));
				Test->TestNotNull(TEXT("Tree collapse preserves the root soil"), Level->GetBlockAtPosition(FVector2D(Forest->GetTrees()[0].Column, Surface)));
				for (const auto& Block : Forest->GetTrees()[0].Pieces) { if (Block.IsValid()) { Block->Destroy(); } }
				ChangedAt = World->GetTimeSeconds(); Step = 3; return false;
			}
			if (Mode == 1 && Step == 3 && Elapsed > 3.5f)
			{
				Test->TestEqual(TEXT("A fully cleared tree restarts as a sapling"), Forest->GetSaveData()[0].GrowthStage,1);
				Test->TestEqual(TEXT("Regrowth begins small rather than restoring a complete tree"),Forest->GetSaveData()[0].Pieces.Num(),2);
				FStorageChestSaveData LegacyChest; LegacyChest.Id = FGuid(1541,14,Surface,0x44454348u); LegacyChest.Location = FVector(14.5f * 16.f, -20.f, -(Surface + 0.5f) * 16.f);
				FInventorySlotData StoredOre; StoredOre.ItemUID = TEXT("Ore_Iron"); StoredOre.ItemCount = 5; LegacyChest.Slots = { StoredOre };
				ABlockBase* OriginalFloor = Level->GetBlockAtPosition(FVector2D(14, Surface));
				Level->RestoreStorageChests({ LegacyChest }, {});
				AStorageChestActor* MovedChest = nullptr;
				for (TActorIterator<AStorageChestActor> It(World); It; ++It) { if (It->GetChestId() == LegacyChest.Id) { MovedChest = *It; } }
				Test->TestTrue(TEXT("An old stocked road prop moves outside the planting yard without losing its cargo"), MovedChest != nullptr && MovedChest->GetActorLocation().X > (Settings->Forestry.YardEndColumn + 1) * 16.f && MovedChest->GetItemSlots()[0].ItemCount == 5);
				Test->TestTrue(TEXT("Restored props never cut a hole in a planting foundation"), OriginalFloor != nullptr && Level->GetBlockAtPosition(FVector2D(14, Surface)) == OriginalFloor);
				LegacyChest.Id = FGuid(1541,22,Surface,0x44454348u); LegacyChest.Slots.Reset();
				Level->RestoreStorageChests({ LegacyChest }, {});
				Test->TestTrue(TEXT("Empty old road decoration chests stay removed from the facility"), Level->GetDestroyedStorageChestIds().Contains(LegacyChest.Id));
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
			if (Mode == 4 && Step == 1 && Elapsed > 5.2f)
			{
				for (const auto& Tree : Forest->GetTrees())
				{
					Test->TestEqual(TEXT("A tree reaches maturity while physics runs between growth stages"),Tree.GrowthStage,4);
					Test->TestFalse(TEXT("An untouched growing tree is never marked as harvested"),Tree.bHarvested);
					Test->TestEqual(TEXT("Growth retains every expected tree cell across real frames"),Tree.Pieces.Num(),Tree.ExpectedPieceCount);
				}
				auto Saved = Forest->GetSaveData();
				// The former migration marked a complete two-trunk canopy as a harvested mature tree.
				auto& Legacy = Saved[0];
				Legacy.Pieces.RemoveAll([&](const auto& Piece) { return Piece.BlockKey==TEXT("TreeTrunk") && Piece.Location.Z>-(Legacy.GroundRow-2+0.5f)*16.f; });
				for (auto& Piece:Legacy.Pieces) { if(Piece.BlockKey!=TEXT("TreeTrunk")){Piece.Location.Z-=32.f;} }
				Legacy.ExpectedPieceCount=Legacy.Pieces.Num(); Legacy.bHarvested=true;
				Saved[1].Pieces.Pop(); Saved[1].bHarvested=true;
				Forest->RestoreTrees(Saved);
				Test->TestFalse(TEXT("The incomplete legacy maturity metadata is repaired rather than treated as harvest"),Forest->GetTrees()[0].bHarvested);
				Test->TestTrue(TEXT("A real partial harvest remains blocked"),Forest->GetTrees()[1].bHarvested);
				ChangedAt=World->GetTimeSeconds(); Step=2; return false;
			}
			if (Mode == 4 && Step == 2 && Elapsed > 2.5f)
			{
				Test->TestEqual(TEXT("The repaired legacy tree restores all four trunk cells and canopy"),Forest->GetTrees()[0].Pieces.Num(),17);
				Test->TestFalse(TEXT("A repaired untouched legacy tree stays healthy across physics ticks"),Forest->GetTrees()[0].bHarvested);
				Test->TestTrue(TEXT("A genuinely cut tree is still a red blocked plot"),Forest->GetTrees()[1].bHarvested);
				auto Saved=Forest->GetSaveData();
				Player->SetActorLocation(Saved[2].Pieces[0].Location);
				Forest->RestoreTrees(Saved);
				Test->TestEqual(TEXT("A pawn blocking reconstruction never loses a saved tree piece"),Forest->GetSaveData()[2].Pieces.Num(),17);
				Test->TestFalse(TEXT("An occupied reconstruction is not a harvest"),Forest->GetSaveData()[2].bHarvested);
				ChangedAt=World->GetTimeSeconds(); Step=3; return false;
			}
			if (Mode == 4 && Step == 3 && Elapsed > 0.6f)
			{
				Player->SetActorLocation(FVector(40.5f*16.f,-20.f,-Surface*16.f+10.f));
				ChangedAt=World->GetTimeSeconds(); Step=4; return false;
			}
			if (Mode == 4 && Step == 4 && Elapsed > 1.5f)
			{
				Test->TestEqual(TEXT("Reconstruction finishes after the pawn leaves"),Forest->GetTrees()[2].Pieces.Num(),17);
				Test->TestFalse(TEXT("The reconstructed tree remains healthy"),Forest->GetTrees()[2].bHarvested);
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
		TStrongObjectPtr<UMKSaveGame> Progress; TStrongObjectPtr<UMKTooltipBase> Tooltip; TWeakObjectPtr<AMineFacilityActor> Board; TWeakObjectPtr<AMKMonster> FallingMonster; TWeakObjectPtr<AActor> ParallaxSample; FVector BeforeParallax;
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestryTemporalTest, "ProjectMK.PIE.Expansion.UntouchedTreeGrowthAcrossFrames", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FForestryTemporalTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false)); ADD_LATENT_AUTOMATION_COMMAND(FMineExpansionCommand(this, 4)); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
