#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ProjectMK/UI/StorageChestWidget.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/StorageChestActor.h"
#include "ProjectMK/Actor/Transport/RailTrackActor.h"
#include "ProjectMK/Actor/Transport/MineCartActor.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/MineTransportSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"

namespace
{
	class FRailTransportCommand : public IAutomationLatentCommand
	{
	public:
		explicit FRailTransportCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			AMKCharacter* Player = World != nullptr ? Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)) : nullptr;
			if (Player == nullptr) { if (FPlatformTime::Seconds() - Started > 30.0) { Test->AddError(TEXT("Rail fixture timed out.")); return true; } return false; }
			USaveGameSubsystem::Get(World)->StartPlaytestSession();
			ULevelManagerSubsystem* Level = World->GetSubsystem<ULevelManagerSubsystem>();
			UMineTransportSubsystem* Network = World->GetSubsystem<UMineTransportSubsystem>();
			UGameSettingDataAsset* Settings = const_cast<UGameSettingDataAsset*>(UDataManager::Get(World)->GetGameSettingDataAsset());
			UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
			APlayerController* Controller = Player->GetController<APlayerController>();
			const int32 Surface = Settings->SurfaceBlockPositionY;
			if (Step == 0)
			{
				Saved = Settings->MineableLevelGeneration;
				auto& Generation = Settings->MineableLevelGeneration;
				Generation.StartColumn = 48; Generation.WidthInBlocks = 32; Generation.DepthInBlocks = 24;
				Generation.SurfaceReliefHeight = 0; Generation.FallingStructureCount = 0; Generation.bGenerateDecorations = false; Generation.BuriedChestMaxCount = 0;
				Generation.EmptyBlockRatio = 0.f; Generation.RockBlockRatio = 0.f;
				Level->RegenerateMineableBlocks(872);
				Inventory->ApplySavedInventorySlots({}); Inventory->ApplySavedQuickSlots({}); Inventory->SetItemCollectionActive(false);
				TArray<FIntPoint> Nodes = { {54,Surface}, {55,Surface}, {56,Surface}, {57,Surface+1}, {58,Surface+2}, {59,Surface+2} };
				for (const auto& Node : Nodes) { for (int32 H = 1; H <= 2; ++H) { if (ABlockBase* Block = Level->GetBlockAtPosition(FVector2D(Node.X,Node.Y-H))) { Block->Destroy(); } } }
				for (int32 Index=1;Index<Nodes.Num();++Index)
				{
					if(Nodes[Index].Y!=Nodes[Index-1].Y){if(auto* Crossing=Level->GetBlockAtPosition(FVector2D(Nodes[Index-1].X,FMath::Min(Nodes[Index-1].Y,Nodes[Index].Y)))){Crossing->Destroy();}}
					Test->TestTrue(TEXT("Rails attach along flat and diagonal terrain"),Network->TryPlaceRailSegment(Nodes[Index-1],Nodes[Index]));
				}
				Test->TestFalse(TEXT("Duplicate rail segments cannot consume another item"), Network->TryPlaceRailSegment(Nodes[0],Nodes[1]));
				Test->TestFalse(TEXT("Three-way branch connections are rejected"), Network->TryPlaceRailSegment(Nodes[1],FIntPoint(56,Surface+1)));
				const TArray<FIntPoint> Ring = { {64,Surface-1}, {65,Surface}, {64,Surface+1}, {63,Surface} };
				for (const auto& Node : Ring)
				{
					if (ABlockBase* Block = Level->GetBlockAtPosition(FVector2D(Node))) { Block->Destroy(); }
					Test->TestTrue(TEXT("A scaffold can support a rail node"), Level->TryPlaceBlock(TEXT("SupportBeam"),FVector((Node.X+0.5f)*16.f,-20.f,-(Node.Y+0.5f)*16.f)));
				}
				for (const auto& Node : Ring)
				{
					ABlockBase* Above = Level->GetBlockAtPosition(FVector2D(Node.X, Node.Y - 1));
					if (Above != nullptr && Above->IsClimbableSupport() == false) { Above->Destroy(); }
				}
				for (int32 Index = 1; Index < Ring.Num(); ++Index) { Test->TestTrue(TEXT("An open bent line can be placed on supports"),Network->TryPlaceRailSegment(Ring[Index-1],Ring[Index])); }
				Test->TestTrue(TEXT("A closing placement becomes a capped disconnected end instead of a loop"),Network->TryPlaceRailSegment(Ring.Last(),Ring[0]));
				ARailTrackActor* Capped = Network->GetRails().Last().Get();
				Test->TestTrue(TEXT("The visibly separated end cannot transfer a train across the gap"),Capped->GetSaveData().bEndDisconnected && Network->FindNextRail(Capped,Capped->GetSaveData().End)==nullptr);
				Player->SetActorLocation(FVector(53.5f*16.f,-20.f,-Surface*16.f+9.04f)); Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				Test->TestTrue(TEXT("A manual mine cart installs near a real rail"),Network->TryPlaceCart(Player,false));
				if (Network->GetCarts().IsEmpty()) { Settings->MineableLevelGeneration=Saved; return true; }
				Cart=Network->GetCarts()[0];
				Player->SetActorLocation(Cart->GetActorLocation()+FVector(-14.f,0.f,3.f));
				ChangedAt=World->GetTimeSeconds(); ++Step; return false;
			}
			const float Elapsed=World->GetTimeSeconds()-ChangedAt;
			if (Step==1 && Elapsed>0.3f) { Start=Cart->GetActorLocation(); Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D,IE_Pressed,1.f)); ChangedAt=World->GetTimeSeconds(); ++Step; return false; }
			if (Step==2 && Elapsed>0.65f)
			{
				Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D,IE_Released,0.f));
				const float Travel=Cart->GetActorLocation().X-Start.X;
				Test->AddInfo(FString::Printf(TEXT("Heavy manual push distance %.2f"),Travel));
				Test->TestTrue(TEXT("Real body pushing moves a heavy cart slowly"),Travel>0.5f && Travel<16.f);
				ARailTrackActor* Slope=Network->GetRails()[2].Get();
				FMineCartSaveData Data=Cart->GetSaveData(); Data.RailId=Slope->GetSaveData().Id; Data.Position=0.5f; Data.Speed=0.f;
				Cart->InitializeCart(Data,Slope); Start=Cart->GetActorLocation();
				const FVector Tangent = (Slope->GetEndWorld() - Slope->GetStartWorld()).GetSafeNormal();
				Test->TestTrue(TEXT("Cart body follows the slope tangent"), FVector::DotProduct(Cart->GetActorForwardVector(), Tangent) > 0.99f);
				Test->TestFalse(TEXT("Cart movement rejects solid terrain"), Network->CanCartOccupy(Cart.Get(), FVector(60.5f * 16.f, -20.f, -(Surface + 5.5f) * 16.f)));
				Player->GetCharacterMovement()->DisableMovement(); Player->SetActorLocation(FVector(53.f*16.f,-20.f,-Surface*16.f+100.f));
				ChangedAt=World->GetTimeSeconds(); ++Step; return false;
			}
			if (Step==3 && Elapsed>0.9f)
			{
				Test->TestTrue(TEXT("A manual cart rolls downhill under gravity"),Cart->GetActorLocation().Z<Start.Z-5.f);
				FMineCartSaveData Data=Cart->GetSaveData(); Data.Speed=0.f; Data.Position=0.6f;
				ARailTrackActor* Bottom=Network->GetRails()[4].Get(); Data.RailId=Bottom->GetSaveData().Id; Cart->InitializeCart(Data,Bottom);
				Player->SetActorLocation(Cart->GetActorLocation()+FVector(-18.f,0.f,3.f)); Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				Inventory->AddItem(TEXT("StorageChest"),1);
				ChangedAt=World->GetTimeSeconds(); ++Step; return false;
			}
			if (Step==4 && Elapsed>0.3f)
			{
				Test->TestTrue(TEXT("Using the chest item in front of a cart mounts it"),Player->GetConsumableComponent()->TryUseItem(TEXT("StorageChest")));
				AStorageChestActor* Cargo=Cart->GetCargo();
				if (Test->TestNotNull(TEXT("The cart owns an actual storage chest"),Cargo)==false) { Settings->MineableLevelGeneration=Saved; return true; }
				Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F,IE_Pressed,1.f));
				Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F,IE_Released,0.f));
				ChangedAt=World->GetTimeSeconds(); Step=40; return false;
			}
			if (Step==40 && Elapsed>0.3f)
			{
				TArray<UUserWidget*> Panels; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Panels,UStorageChestWidget::StaticClass(),false);
				Test->TestTrue(TEXT("Real F input chooses the mounted chest ahead of the cart body"),Panels.ContainsByPredicate([](UUserWidget* Panel){return Panel->IsVisible();}));
				FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("AgentLogs/rail-cargo-prototype.png"), true, false);
				ChangedAt=World->GetTimeSeconds(); Step=41; return false;
			}
			if (Step==41 && Elapsed>0.2f)
			{
				AStorageChestActor* Cargo=Cart->GetCargo();
				Inventory->AddItem(TEXT("Ore_Iron"),5);
				Test->TestTrue(TEXT("The existing slot transfer stores ore in the cart chest"),Inventory->TryMoveItemSlot(0,0,EItemContainer::Inventory,EItemContainer::Storage,nullptr,Cargo));
				Test->TestFalse(TEXT("A loaded cart chest cannot be removed"),Cargo->TryRecover(Player));
				Test->TestFalse(TEXT("The cart cannot be recovered before its chest"),Cart->TryRecover(Player));
				Test->TestTrue(TEXT("Rails cannot be removed while a cart is on their connected line"),Network->GetRails()[0]->GetRecoveryBlockedReason(Player).ToString().Contains(TEXT("열차")));
				const auto SavedRails=Network->GetRailSaveData(); const auto SavedCarts=Network->GetCartSaveData(); const auto SavedChests=Level->GetStorageChestSaveData();
				Network->RestoreNetwork(SavedRails,SavedCarts); Level->RestoreStorageChests(SavedChests,{});
				Cart=Network->FindCart(SavedCarts[0].Id); Cargo=Cart.IsValid()?Cart->GetCargo():nullptr;
				Test->TestTrue(TEXT("Rail position, cart identity and mounted chest contents restore together"),Cargo!=nullptr && Cargo->GetItemSlots()[0].ItemCount==5 && Cargo->GetSaveData().MountedCartId==SavedCarts[0].Id);
				if (Cargo!=nullptr)
				{
					Test->TestTrue(TEXT("The mounted chest can be emptied"),Inventory->TryMoveItemSlot(0,0,EItemContainer::Storage,EItemContainer::Inventory,Cargo));
					Test->TestTrue(TEXT("Recovery removes the empty chest first"),Cargo->TryRecover(Player));
				}
				Test->TestTrue(TEXT("The cart is recoverable after its chest"),Cart->TryRecover(Player));
				ARailTrackActor* Slope=Network->GetRails()[2].Get();
				Test->TestTrue(TEXT("An automatic cart can be installed on the same network"),Network->TryPlaceCart(Player,true));
				if (Network->GetCarts().IsEmpty()) { Settings->MineableLevelGeneration=Saved; return true; }
				Cart=Network->GetCarts()[0]; FMineCartSaveData Auto=Cart->GetSaveData(); Auto.RailId=Slope->GetSaveData().Id; Auto.Position=0.8f; Auto.Speed=0.f; Auto.DriveDirection=-1;
				Cart->InitializeCart(Auto,Slope); Start=Cart->GetActorLocation(); Player->GetCharacterMovement()->DisableMovement(); Player->SetActorLocation(FVector(53.f*16.f,-20.f,-Surface*16.f+100.f));
				ChangedAt=World->GetTimeSeconds(); Step=5; return false;
			}
			if (Step==5 && Elapsed>1.f)
			{
				Test->TestTrue(TEXT("The automatic drive climbs uphill after activation"),Cart->GetActorLocation().Z>Start.Z+5.f);
				Settings->MineableLevelGeneration=Saved; return true;
			}
			return false;
		}
	private:
		FAutomationTestBase* Test; double Started; int32 Step=0; float ChangedAt=0.f; FVector Start; FMineableLevelGenerationSettings Saved; TWeakObjectPtr<AMineCartActor> Cart;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailTransportTest,"ProjectMK.PIE.Transport.RailCartCargoAndRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailTransportTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FRailTransportCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
