#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"
#include "PaperSpriteActor.h"
#include "PaperSpriteComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "AbilitySystemComponent.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/MineFacilityActor.h"
#include "ProjectMK/Actor/Shop/StorageChestActor.h"
#include "ProjectMK/Actor/Transport/RailTrackActor.h"
#include "ProjectMK/Actor/Transport/MineCartActor.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Component/CraftingComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/MKSaveGame.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/MineTransportSubsystem.h"
#include "ProjectMK/Core/Subsystem/TownProgressSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/Core/Subsystem/ShopTradeSubsystem.h"
#include "ProjectMK/UI/HUDWidget.h"

namespace
{
	struct FPrototypeFixture
	{
		UWorld* World = nullptr;
		AMKCharacter* Player = nullptr;
		ULevelManagerSubsystem* Level = nullptr;
		UGameSettingDataAsset* Settings = nullptr;
		FMineableLevelGenerationSettings SavedGeneration;
		FTownFacilitySettings SavedTown;
		TStrongObjectPtr<UMKSaveGame> SavedProgress;
		bool Initialize(int32 Width)
		{
			World = GEditor->PlayWorld;
			Player = World != nullptr ? Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)) : nullptr;
			if (Player == nullptr) { return false; }
			USaveGameSubsystem::Get(World)->StartPlaytestSession();
			Level = World->GetSubsystem<ULevelManagerSubsystem>();
			Settings = const_cast<UGameSettingDataAsset*>(UDataManager::Get(World)->GetGameSettingDataAsset());
			SavedGeneration = Settings->MineableLevelGeneration; SavedTown = Settings->TownFacilities;
			SavedProgress.Reset(NewObject<UMKSaveGame>());
			World->GetSubsystem<UTownProgressSubsystem>()->WriteSaveData(SavedProgress.Get());
			TStrongObjectPtr<UMKSaveGame> Empty(NewObject<UMKSaveGame>());
			World->GetSubsystem<UTownProgressSubsystem>()->RestoreProgress(Empty.Get());
			auto& Generation = Settings->MineableLevelGeneration;
			Generation.WidthInBlocks = Width; Generation.DepthInBlocks = Width == 128 ? 96 : 24; Generation.StartColumn = Width == 128 ? 0 : 48;
			Generation.SurfaceReliefHeight = Width == 128 ? 2 : 0; Generation.FallingStructureCount = 0; Generation.EmptyBlockRatio = 0.f; Generation.RockBlockRatio = 0.f;
			Generation.bGenerateDecorations = false; Generation.BuriedChestMaxCount = 0;
			Settings->TownFacilities.QuestCooldownSeconds = 0; Settings->TownFacilities.QuestRelicChance = 0.f; Settings->TownFacilities.GuaranteedRelicQuestCount = 3;
			Level->RegenerateMineableBlocks(830 + Width);
			UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
			Inventory->ApplySavedInventorySlots({}); Inventory->ApplySavedQuickSlots({}); Inventory->ApplyDiscoveredItemKeys({}); Inventory->SetItemCollectionActive(false);
			Player->SetActorLocation(FVector(56.5f * 16.f, -20.f, -Settings->SurfaceBlockPositionY * 16.f + 9.04f));
			Player->GetCharacterMovement()->StopMovementImmediately(); Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			return true;
		}
		void Restore()
		{
			Settings->MineableLevelGeneration = SavedGeneration; Settings->TownFacilities = SavedTown;
			World->GetSubsystem<UTownProgressSubsystem>()->RestoreProgress(SavedProgress.Get());
		}
	};

	class FTownProgressCommand : public IAutomationLatentCommand
	{
	public:
		explicit FTownProgressCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (Step == 0)
			{
				if (Fixture.Initialize(128) == false) { if (FPlatformTime::Seconds() - Started > 30.0) { Test->AddError(TEXT("Town fixture timed out.")); return true; } return false; }
				Town = Fixture.World->GetSubsystem<UTownProgressSubsystem>();
				for (const auto& Actor : Town->GetFacilities())
				{
					if (Actor->GetFacilityKind() == EMineFacilityKind::Elevator && Actor->GetStationDepth() == 0) { Surface = Actor; }
					if (Actor->GetFacilityKind() == EMineFacilityKind::Elevator && Actor->GetStationDepth() == 72) { Deep = Actor; }
					if (Actor->GetFacilityKind() == EMineFacilityKind::RailFactory) { Factory = Actor; }
					if (Actor->GetFacilityKind() == EMineFacilityKind::HouseA) { House = Actor; }
				}
				if (Test->TestTrue(TEXT("Two houses, quest board, factory and all three elevator stations exist"), Town->GetFacilities().Num() == 7 && Surface.IsValid() && Deep.IsValid() && Factory.IsValid() && House.IsValid()) == false) { Fixture.Restore(); return true; }
				int32 VillageCount = 0;
				for (TActorIterator<APaperSpriteActor> It(Fixture.World); It; ++It)
				{
					if (It->ActorHasTag(TEXT("VillageScenery")) == false) { continue; }
					++VillageCount;
					TArray<int32> Columns = Fixture.Settings->TownFacilities.VillageBackdropColumns;
					Columns.Append(Fixture.Settings->TownFacilities.IndustrialBackdropColumns); Columns.Append(Fixture.Settings->TownFacilities.ForestryBackdropColumns);
					const bool bAtAuthoredPosition = Columns.ContainsByPredicate([&](int32 Column)
					{
						const float BaseX = (Fixture.Settings->MineableLevelGeneration.StartColumn + Column + 0.5f)*16.f;
						const float ShiftedX = BaseX - (Fixture.Player->GetActorLocation().X-BaseX)*Fixture.Settings->TownFacilities.BackgroundParallaxStrength;
						return FMath::IsNearlyEqual(static_cast<float>(It->GetActorLocation().X),BaseX,0.1f) || FMath::IsNearlyEqual(static_cast<float>(It->GetActorLocation().X),ShiftedX,0.1f);
					});
					Test->TestTrue(TEXT("Every district sprite matches a configured position or its continuous parallax offset"),bAtAuthoredPosition && It->GetRenderComponent()->GetSprite()!=nullptr);
					Test->TestFalse(TEXT("Background houses never collide with the player"), It->GetActorEnableCollision());
				}
				Test->TestEqual(TEXT("The configured village and industrial layers fill their districts"), VillageCount, Fixture.Settings->TownFacilities.VillageBackdropColumns.Num() + Fixture.Settings->TownFacilities.IndustrialBackdropColumns.Num() + Fixture.Settings->TownFacilities.ForestryBackdropColumns.Num());
				Test->TestFalse(TEXT("Unvisited elevator depths stay locked"), Town->CanTravelToDepth(32));
				Test->TestTrue(TEXT("The surface station explains construction before unlock"), Town->GetDialogue(Surface.Get(), Fixture.Player).ToString().Contains(TEXT("공사 중")));
				Fixture.Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UAttributeSet_Character::GetCoinAttribute(), 1000.f);
				Test->TestFalse(TEXT("Factory recipes cannot unlock before the depth milestone"), Fixture.Player->GetCraftingComponent()->CanUnlockRecipe(TEXT("RailRecipe")));
				Fixture.Player->SetActorLocation(Deep->GetActorLocation() + FVector(0.f, 0.f, -2.96f));
				ChangedAt = Fixture.World->GetTimeSeconds(); ++Step; return false;
			}
			if (Fixture.World->GetTimeSeconds() - ChangedAt < (Step == 3 ? 1.5f : 0.7f)) { return false; }
			if (Step == 2)
			{
				Test->TestTrue(TEXT("Fade travel arrives at the surface before returning"), FMath::Abs(Fixture.Player->GetActorLocation().Z - Surface->GetActorLocation().Z) < 20.f);
				if (Town->TryTravel(Fixture.Player, Surface.Get(), 72) == false)
				{
					if (Fixture.World->GetTimeSeconds() - ChangedAt < 3.f) { return false; }
					Test->AddError(TEXT("The surface elevator did not release the completed transition.")); Fixture.Restore(); return true;
				}
				Test->AddInfo(FString::Printf(TEXT("Return trip accepted after fade at %.2fs"), Fixture.World->GetTimeSeconds() - ChangedAt));
				ChangedAt = Fixture.World->GetTimeSeconds(); Step = 3; return false;
			}
			if (Step == 3)
			{
				Test->TestTrue(TEXT("The second fade travel arrives underground"), FMath::Abs(Fixture.Player->GetActorLocation().Z - Deep->GetActorLocation().Z) < 20.f);
				Fixture.Restore(); return true;
			}
			Test->TestTrue(TEXT("Reaching the real deep station records maximum depth"), Town->GetMaximumDepth() >= 72);
			Test->TestTrue(TEXT("The factory unlocks after the configured depth"), Town->IsRailFactoryUnlocked());
			Fixture.Player->SetActorLocation(Factory->GetActorLocation() + FVector(0.f, 0.f, -2.96f));
			Test->TestTrue(TEXT("Factory offers use the existing purchase transaction"), Fixture.World->GetSubsystem<UShopTradeSubsystem>()->TryPurchaseItem(Fixture.Player, TEXT("RailFactory_Rail"), 4));
			Test->TestTrue(TEXT("Factory recipe purchase is available at its counter"), Fixture.Player->GetCraftingComponent()->TryUnlockRecipe(TEXT("RailRecipe")));
			Test->TestTrue(TEXT("Factory F interaction opens the common shop screen"), Factory->TryInteract(Fixture.Player));
			TArray<UUserWidget*> HUDs; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(Fixture.World, HUDs, UHUDWidget::StaticClass(), false);
			Test->TestTrue(TEXT("The shared modal menu is visible"), CastChecked<UHUDWidget>(HUDs[0])->IsMenuVisible());
			CastChecked<UHUDWidget>(HUDs[0])->CloseFacility();
			Fixture.Player->SetActorLocation(House->GetActorLocation() + FVector(0.f, 0.f, -2.96f));
			UInventoryComponent* Inventory = Fixture.Player->FindComponentByClass<UInventoryComponent>();
			for (const FName Key : { TEXT("Ore_Coal"), TEXT("Ore_Copper"), TEXT("Ore_Iron") }) { Inventory->AddItem(Key, 30); }
			Fixture.Settings->TownFacilities.QuestOfferChance = 1.f;
			Town->BeginResidentConversation(Fixture.Player, House.Get());
			Test->TestTrue(TEXT("The first visit is small talk without a quest button"), Town->GetQuestActionLabel(House.Get()).IsEmpty());
			Town->BeginResidentConversation(Fixture.Player, House.Get());
			Test->TestTrue(TEXT("Residents keep chatting for the configured warm-up visits"), Town->GetQuestActionLabel(House.Get()).IsEmpty());
			Fixture.Settings->TownFacilities.QuestOfferChance = 0.f;
			Town->BeginResidentConversation(Fixture.Player, House.Get());
			Test->TestTrue(TEXT("An unsuccessful random offer keeps the visit as small talk"), Town->GetQuestActionLabel(House.Get()).IsEmpty());
			Fixture.Settings->TownFacilities.QuestOfferChance = 1.f;
			Town->BeginResidentConversation(Fixture.Player, House.Get());
			Test->TestTrue(TEXT("Reopening a conversation cannot reroll during its cooldown"), Town->GetQuestActionLabel(House.Get()).IsEmpty());
			for (int32 Quest = 0; Quest < 3; ++Quest)
			{
				Town->ClearQuestCooldownsForPlaytest();
				Town->BeginResidentConversation(Fixture.Player, House.Get());
				Test->TestTrue(TEXT("Resident offers can be accepted"), Town->TryQuestAction(Fixture.Player, House.Get()));
				Test->TestTrue(TEXT("Accepted quests consume the requested ore and grant rewards"), Town->TryQuestAction(Fixture.Player, House.Get()));
			}
			Test->TestTrue(TEXT("The gimmick relic has a guaranteed route after three completed quests"), Town->HasRelic(TEXT("SteadyBelt")));
			Fixture.Player->ApplyKnockback(120.f, 80.f, 0.5f);
			Test->TestFalse(TEXT("The prototype counterweight relic prevents monster knockback"), Fixture.Player->IsKnockedBack());
			TStrongObjectPtr<UMKSaveGame> Snapshot(NewObject<UMKSaveGame>()); Town->WriteSaveData(Snapshot.Get());
			TArray<uint8> Bytes; UGameplayStatics::SaveGameToMemory(Snapshot.Get(), Bytes);
			TStrongObjectPtr<UMKSaveGame> Reloaded(CastChecked<UMKSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
			Town->RestoreProgress(Reloaded.Get());
			Test->TestTrue(TEXT("Depth, quest completion and relic ownership survive save reload"), Town->GetMaximumDepth() >= 72 && Town->HasRelic(TEXT("SteadyBelt")) && Reloaded->TownQuests[0].CompletedCount == 3);
			Fixture.Player->SetActorLocation(Deep->GetActorLocation() + FVector(0.f, 0.f, -2.96f));
			Test->TestTrue(TEXT("An unlocked underground station returns the player to the surface"), Town->TryTravel(Fixture.Player, Deep.Get(), 0));
			ChangedAt = Fixture.World->GetTimeSeconds(); Step = 2; return false;
		}
	private:
		FAutomationTestBase* Test; double Started; int32 Step = 0; float ChangedAt = 0.f;
		FPrototypeFixture Fixture;
		UTownProgressSubsystem* Town = nullptr;
		TWeakObjectPtr<AMineFacilityActor> Surface, Deep, Factory, House;
	};

	class FSupportClimbCommand : public IAutomationLatentCommand
	{
	public:
		explicit FSupportClimbCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (Step == 0)
			{
				if (Fixture.Initialize(32) == false) { if (FPlatformTime::Seconds() - Started > 30.0) { Test->AddError(TEXT("Support fixture timed out.")); return true; } return false; }
				for (int32 Height = 1; Height <= 3; ++Height) { Test->TestTrue(TEXT("Supports stack as real block cells"), Fixture.Level->TryPlaceBlock(TEXT("SupportBeam"), FVector(56.5f * 16.f, -20.f, -(Fixture.Settings->SurfaceBlockPositionY - Height + 0.5f) * 16.f))); }
				StartZ = Fixture.Player->GetActorLocation().Z;
				ChangedAt = Fixture.World->GetTimeSeconds(); ++Step; return false;
			}
			APlayerController* Controller = Fixture.Player->GetController<APlayerController>();
			const float Elapsed = Fixture.World->GetTimeSeconds() - ChangedAt;
			if (Step == 1 && Elapsed > 0.3f) { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Pressed, 1.f)); ChangedAt = Fixture.World->GetTimeSeconds(); ++Step; return false; }
			if (Step == 2 && Elapsed > 2.2f)
			{
				Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Released, 0.f));
				Test->TestTrue(TEXT("W alone no longer climbs support columns"),FMath::Abs(Fixture.Player->GetActorLocation().Z-StartZ)<2.f);
				Fixture.Player->SetActorLocation(FVector(56.5f*16.f,-20.f,-(Fixture.Settings->SurfaceBlockPositionY-3)*16.f+20.f));
				Fixture.Player->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
				ChangedAt=Fixture.World->GetTimeSeconds(); Step=3; return false;
			}
			if (Step==3 && Elapsed>1.f)
			{
				Test->TestTrue(TEXT("Support tops remain standable using flight or falling"),Fixture.Player->GetCharacterMovement()->IsMovingOnGround());
				ABlockBase* Top=Fixture.Level->GetBlockAtPosition(FVector2D(56,Fixture.Settings->SurfaceBlockPositionY-3));
				Test->TestTrue(TEXT("Support is non-drillable and has no idle F hint"),Top!=nullptr && Top->IsMineable()==false && Top->ShouldShowInteractionPrompt()==false);
				Fixture.Player->FindComponentByClass<UInventoryComponent>()->AddItem(TEXT("SupportBeam"),1);
				Test->TestTrue(TEXT("Another support can be installed while standing on the stack"),Fixture.Player->GetConsumableComponent()->TryUseItem(TEXT("SupportBeam")));
				Test->TestEqual(TEXT("Support stack persists"),Fixture.Level->GetInstalledStructureSaveData().Num(),4);
				Fixture.Restore(); return true;
			}
			return false;
		}
	private:
		FAutomationTestBase* Test; double Started; int32 Step = 0; float ChangedAt = 0.f, StartZ = 0.f; FPrototypeFixture Fixture;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTownPrototypeTest, "ProjectMK.PIE.Town.DepthTravelFactoryQuestsAndRelic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTownPrototypeTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FTownProgressCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSupportClimbTest, "ProjectMK.PIE.Transport.SupportPlatformsAndStack", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSupportClimbTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FSupportClimbCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
