#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/MineFacilityActor.h"
#include "ProjectMK/Actor/Shop/StorageChestActor.h"
#include "ProjectMK/Actor/Transport/RailTrackActor.h"
#include "ProjectMK/Actor/Transport/MineCartActor.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/MineTransportSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/UI/HUDWidget.h"

namespace
{
	class FMineRevisionCommand : public IAutomationLatentCommand
	{
	public:
		FMineRevisionCommand(FAutomationTestBase* InTest,int32 InMode) : Test(InTest),Mode(InMode),Started(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (Step==0)
			{
				World=GEditor->PlayWorld;
				Player=World!=nullptr?Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(World,0)):nullptr;
				if (Player==nullptr) { if(FPlatformTime::Seconds()-Started>30.0){Test->AddError(TEXT("Revision fixture timed out"));return true;} return false; }
				USaveGameSubsystem::Get(World)->StartPlaytestSession();
				Settings=const_cast<UGameSettingDataAsset*>(UDataManager::Get(World)->GetGameSettingDataAsset()); Saved=Settings->MineableLevelGeneration;
				auto& G=Settings->MineableLevelGeneration; G.StartColumn=48;G.WidthInBlocks=32;G.DepthInBlocks=24;G.SurfaceReliefHeight=0;G.FallingStructureCount=0;G.EmptyBlockRatio=0.f;G.RockBlockRatio=0.f;G.bGenerateDecorations=false;G.BuriedChestMaxCount=0;
				Level=World->GetSubsystem<ULevelManagerSubsystem>(); Network=World->GetSubsystem<UMineTransportSubsystem>(); Level->RegenerateMineableBlocks(1827);
				Inventory=Player->FindComponentByClass<UInventoryComponent>();Inventory->ApplySavedInventorySlots({});Inventory->ApplySavedQuickSlots({});Inventory->SetItemCollectionActive(false);
				Surface=Settings->SurfaceBlockPositionY;Controller=Player->GetController<APlayerController>();
				Player->SetActorLocation(FVector(52.5f*16.f,-20.f,-Surface*16.f+9.04f));Player->GetCharacterMovement()->DisableMovement();
				TArray<UUserWidget*> Widgets;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Widgets,UHUDWidget::StaticClass(),false);HUD=CastChecked<UHUDWidget>(Widgets[0]);
				if(Mode==0)
				{
					for(int32 X=53;X<=57;++X){for(int32 Y=Surface;Y<=Surface+3;++Y){if(auto* B=Level->GetBlockAtPosition(FVector2D(X,Y))){B->Destroy();}}}
					for(FIntPoint Cell:{FIntPoint(54,Surface),FIntPoint(55,Surface+1),FIntPoint(54,Surface+2)}){Test->TestTrue(TEXT("Turn support installs"),Level->TryPlaceBlock(TEXT("SupportBeam"),FVector((Cell.X+0.5f)*16.f,-20.f,-(Cell.Y+0.5f)*16.f)));}
					Test->TestTrue(TEXT("Down-right track installs"),Network->TryPlaceRailSegment(FIntPoint(54,Surface),FIntPoint(55,Surface+1)));
					Player->SetActorLocation(FVector(55.5f*16.f,-20.f,-(Surface+1.5f)*16.f));
					Test->TestTrue(TEXT("Using rail inside the endpoint support creates a down-left return"),Network->TryPlaceRail(Player));
					if(Network->GetRails().Num()!=2){return Finish();}
					ARailTrackActor* Second=Network->GetRails()[1].Get();Test->TestTrue(TEXT("Both sides of the V joint share the exact same cart centre"),AMineCartActor::CalculateRailPosition(Network->GetRails()[0].Get(),1.f).Equals(AMineCartActor::CalculateRailPosition(Second,1.f),0.01f));Test->TestTrue(TEXT("The cart tangent is also continuous through the turn"),AMineCartActor::CalculateRailDirection(Network->GetRails()[0].Get(),1.f).Equals(AMineCartActor::CalculateRailDirection(Second,1.f),0.001f));Test->TestTrue(TEXT("The return track shares the exact descending endpoint"),Second->GetSaveData().Start==FIntPoint(54,Surface+2)&&Second->GetSaveData().End==FIntPoint(55,Surface+1));
					Player->SetActorLocation(FVector(52.5f*16.f,-20.f,-Surface*16.f+12.f));
					Test->TestTrue(TEXT("A cart can be installed near a narrow slope"),Network->TryPlaceCart(Player,false));if(Network->GetCarts().IsEmpty()){return Finish();}
					Cart=Network->GetCarts()[0];auto D=Cart->GetSaveData();D.Position=0.9f;D.Speed=35.f;Cart->InitializeCart(D,Network->GetRails()[0].Get());Cart->UpdateCart(0.15f);
					Test->TestTrue(TEXT("Cart reverses horizontal direction through the V turn without leaving the line"),Cart->GetRail()==Second&&Cart->GetActorLocation().Y==-20.f);
					Test->TestTrue(TEXT("Cart body rotates with the returning slope"),FVector::DotProduct(Cart->GetActorForwardVector(),AMineCartActor::CalculateRailDirection(Second,Cart->GetSaveData().Position))>0.99f);
					D=Cart->GetSaveData();D.bAutomatic=true;D.Position=0.6f;D.Speed=0.f;D.DriveDirection=-1;Cart->InitializeCart(D,Second);
					Player->SetActorLocation(Cart->GetActorLocation()+FVector(-8.f,0.f,-3.f));const FVector Before=Cart->GetActorLocation();Cart->UpdateCart(0.1f);
					Test->TestTrue(TEXT("Automatic cart stops immediately for a player in its path"),Cart->GetActorLocation().Equals(Before,0.01f)&&Cart->GetSpeed()==0.f);
					// Stand on the cart while an NPC interaction volume occupies the same space.
					Player->SetActorLocation(Cart->GetActorLocation()+FVector(0.f,0.f,15.f));
					House=World->SpawnActor<AMineFacilityActor>();House->InitializeFacility(EMineFacilityKind::HouseA,0,Cart->GetActorLocation()-FVector(0.f,0.f,6.f));
					ChangedAt=World->GetTimeSeconds();Step=1;return false;
				}
				if(Mode==1)
				{
					for(int32 X=62;X<=67;++X){for(int32 Y=Surface;Y<Surface+7;++Y){if(auto* B=Level->GetBlockAtPosition(FVector2D(X,Y))){B->Destroy();}}}
					auto Place=[&](FName Name,int32 X){return Level->TryPlaceBlock(Name,FVector((X+0.5f)*16.f,-20.f,-(Surface+1.5f)*16.f));};
					Test->TestTrue(TEXT("Initial anchor installs"),Place(TEXT("SupportBeam"),62));Place(TEXT("Dirt"),63);Place(TEXT("Dirt"),64);
					Suspended=Level->GetBlockAtPosition(FVector2D(64,Surface+1));Player->SetActorLocation(FVector(61.5f*16.f,-20.f,-(Surface+1.5f)*16.f));
					Test->TestTrue(TEXT("Recovering the anchor emits the actual mined-support event"),Level->GetBlockAtPosition(FVector2D(62,Surface+1))->TryRecover(Player));
					ChangedAt=World->GetTimeSeconds();Step=1;return false;
				}
				if(Mode==2)
				{
					Player->SetActorLocation(FVector(58.5f*16.f,-20.f,-Surface*16.f+9.04f));
					Test->TestTrue(TEXT("Storage fixture installs"),Level->TryPlaceStorageChest(TEXT("StorageChest"),Player->GetActorLocation()));
					for (TActorIterator<AStorageChestActor> It(World); It; ++It) { Chest=*It;break; }Test->TestTrue(TEXT("Nonmodal chest opens"),HUD->OpenStorageChest(Chest.Get()));
					Inventory->AddItem(TEXT("Ore_Iron"),5);
					Test->TestTrue(TEXT("Inventory secondary action routes to the open chest"),HUD->TryStoreInOpenChest(EItemContainer::Inventory,0));
					Test->TestTrue(TEXT("A full ore stack reaches storage without swapping unrelated slots"),Inventory->GetItemCount(TEXT("Ore_Iron"))==0&&Chest->GetItemSlots()[0].ItemCount==5);
					Test->TestTrue(TEXT("Storage transfer back to inventory uses the same stack mover"),Inventory->TryTransferItemStack(0,EItemContainer::Storage,EItemContainer::Inventory,Chest.Get()));
					HUD->CloseStorageChest();Test->TestFalse(TEXT("A closed chest never intercepts inventory actions"),HUD->TryStoreInOpenChest(EItemContainer::Inventory,0));
					ASC=Player->GetAbilitySystemComponent();PreviousDistance=ASC->GetNumericAttribute(UAttributeSet_Character::GetDrillingDistanceAttribute());ASC->SetNumericAttributeBase(UAttributeSet_Character::GetDrillingDistanceAttribute(),48.f);
					Test->TestEqual(TEXT("Three-block skill exposes its maximum reach"),Player->GetMaximumDrillCells(),3);
					Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseWheelAxis,IE_Axis,-1.f));ChangedAt=World->GetTimeSeconds();Step=1;return false;
				}
			}
			const float Elapsed=World->GetTimeSeconds()-ChangedAt;
			if(Mode==0&&Step==1&&Elapsed>0.4f) { Key(true);ChangedAt=World->GetTimeSeconds();Step=2;return false; }
			if(Mode==0&&Step==2&&Elapsed>0.1f) { Key(false);ChangedAt=World->GetTimeSeconds();Step=3;return false; }
			if(Mode==0&&Step==3&&Elapsed>0.2f)
			{
				Test->TestTrue(TEXT("A short F release opens the overlapping house without recovering its cart"),HUD->IsMenuVisible()&&Cart.IsValid());HUD->CloseFacility();ChangedAt=World->GetTimeSeconds();Step=4;return false;
			}
			if(Mode==0&&Step==4&&Elapsed>0.2f) { Key(true);ChangedAt=World->GetTimeSeconds();Step=5;return false; }
			if(Mode==0&&Step==5&&Elapsed>Settings->StructureRecoveryHoldDuration+0.4f) { Key(false);ChangedAt=World->GetTimeSeconds();Step=6;return false; }
			if(Mode==0&&Step==6&&Elapsed>0.2f)
			{
				Test->TestFalse(TEXT("Holding F on top of a cart recovers it despite an overlapping house"),Cart.IsValid());Test->TestEqual(TEXT("Automatic cart is returned once"),Inventory->GetItemCount(TEXT("AutoMineCart")),1);return Finish();
			}
			if(Mode==1&&Step==1&&Elapsed>0.2f)
			{
				Test->TestTrue(TEXT("Disconnected ground enters its warning shake"),Suspended.IsValid()&&Level->IsBlockShaking(Suspended.Get()));
				Test->TestTrue(TEXT("A new anchor can be inserted during the warning"),Level->TryPlaceBlock(TEXT("SupportBeam"),FVector(65.5f*16.f,-20.f,-(Surface+1.5f)*16.f)));ChangedAt=World->GetTimeSeconds();Step=2;return false;
			}
			if(Mode==1&&Step==2&&Elapsed>1.4f)
			{
				Test->TestTrue(TEXT("Restored support cancels shaking and preserves the original cell"),Suspended.IsValid()&&Suspended->IsFalling()==false&&Level->GetBlockAtPosition(FVector2D(64,Surface+1))==Suspended.Get());
				Test->TestTrue(TEXT("A rail attaches to the rescued scaffold"),Network->TryPlaceRailSegment(FIntPoint(65,Surface+1),FIntPoint(66,Surface+1)));
				Level->TryPlaceBlock(TEXT("SupportBeam"),FVector(66.5f*16.f,-20.f,-(Surface-0.5f)*16.f));
				Level->TryPlaceBlock(TEXT("TreeTrunk"),FVector(65.5f*16.f,-20.f,-(Surface-0.5f)*16.f));
				Player->SetActorLocation(FVector(66.5f*16.f,-20.f,-(Surface-0.5f)*16.f));
				Test->TestTrue(TEXT("Upper temporary support is recoverable"),Level->GetBlockAtPosition(FVector2D(66,Surface-1))->TryRecover(Player));
				ChangedAt=World->GetTimeSeconds();Step=3;return false;
			}
			if(Mode==1&&Step==3&&Elapsed>2.5f)
			{
				Test->TestTrue(TEXT("A falling block which stacks over track returns the buried rail as an item"),Network->GetRails().IsEmpty());return Finish();
			}
			if(Mode==2&&Step==1&&Elapsed>0.15f)
			{
				Test->TestEqual(TEXT("Real wheel-down input shortens the drill"),Player->GetSelectedDrillCells(),2);Player->AdjustDrillReach(-99);Test->TestEqual(TEXT("Reach cannot be shorter than one block"),Player->GetSelectedDrillCells(),1);Player->AdjustDrillReach(99);Test->TestEqual(TEXT("Reach cannot exceed the purchased skill"),Player->GetSelectedDrillCells(),3);ASC->SetNumericAttributeBase(UAttributeSet_Character::GetDrillingDistanceAttribute(),PreviousDistance);return Finish();
			}
			if(FPlatformTime::Seconds()-Started>40.0){Test->AddError(TEXT("Revision scenario stalled"));Key(false);return Finish();}
			return false;
		}
	private:
		void Key(bool bDown){Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F,bDown?IE_Pressed:IE_Released,bDown?1.f:0.f));}
		bool Finish(){if(House.IsValid()){House->Destroy();}Settings->MineableLevelGeneration=Saved;return true;}
		FAutomationTestBase* Test;int32 Mode;double Started;int32 Step=0,Surface=0;float ChangedAt=0.f,PreviousDistance=0.f;
		UWorld* World=nullptr;AMKCharacter* Player=nullptr;APlayerController* Controller=nullptr;UGameSettingDataAsset* Settings=nullptr;ULevelManagerSubsystem* Level=nullptr;UMineTransportSubsystem* Network=nullptr;UInventoryComponent* Inventory=nullptr;UHUDWidget* HUD=nullptr;UAbilitySystemComponent* ASC=nullptr;
		FMineableLevelGenerationSettings Saved;TWeakObjectPtr<AMineCartActor> Cart;TWeakObjectPtr<AMineFacilityActor> House;TWeakObjectPtr<ABlockBase> Suspended;TWeakObjectPtr<AStorageChestActor> Chest;
	};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRevisionCartTest,"ProjectMK.PIE.Revision.CartTurnObstacleAndOverlappedRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRevisionCartTest::RunTest(const FString&){ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));ADD_LATENT_AUTOMATION_COMMAND(FMineRevisionCommand(this,0));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRevisionBraceTest,"ProjectMK.PIE.Revision.RebraceShakingGround",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRevisionBraceTest::RunTest(const FString&){ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));ADD_LATENT_AUTOMATION_COMMAND(FMineRevisionCommand(this,1));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRevisionInputTest,"ProjectMK.PIE.Revision.StorageSecondaryActionAndWheelReach",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRevisionInputTest::RunTest(const FString&){ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));ADD_LATENT_AUTOMATION_COMMAND(FMineRevisionCommand(this,2));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;}

#endif
