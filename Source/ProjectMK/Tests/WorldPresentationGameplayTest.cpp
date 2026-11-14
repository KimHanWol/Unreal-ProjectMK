#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PaperTileMapActor.h"
#include "SceneView.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/UI/MinimapWidget.h"

namespace
{
	class FWorldPresentationCommand : public IAutomationLatentCommand
	{
	public:
		explicit FWorldPresentationCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			APlayerController* Controller = World != nullptr ? World->GetFirstPlayerController() : nullptr;
			AMKCharacter* Player = Controller != nullptr ? Cast<AMKCharacter>(Controller->GetPawn()) : nullptr;
			if (Player == nullptr) { return FPlatformTime::Seconds() - Started > 20.0; }
			UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>();
			if (Camera == nullptr) { Test->AddError(TEXT("Camera fixture is missing.")); return true; }
			if (FPlatformTime::Seconds() - Started > 30.0)
			{
				Test->AddError(TEXT("Camera presentation fixture timed out waiting for game frames."));
				if (Step > 0) { Restore(Camera); }
				return true;
			}
			if (Step == 0)
			{
				USaveGameSubsystem::Get(World)->StartPlaytestSession();
				if (Player->IsOnGround() == false || World->GetTimeSeconds() < 0.5f) { return false; }
				SavedAspect = Camera->AspectRatio;
				bSavedConstrain = Camera->bConstrainAspectRatio;
				Camera->bConstrainAspectRatio = true;
				Camera->SetAspectRatio(4.f / 3.f);
				ChangedAt = World->GetTimeSeconds();
				Advance();
				return false;
			}
			if (World->GetTimeSeconds() - ChangedAt < 0.5f) { return false; }
			TArray<UUserWidget*> Found;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, UMinimapWidget::StaticClass(), false);
			if (Test->TestTrue(TEXT("The minimap exists"), Found.Num() > 0) == false) { Restore(Camera); return true; }
			UMinimapWidget* Map = CastChecked<UMinimapWidget>(Found[0]);
			const FStructProperty* CameraProperty = FindFProperty<FStructProperty>(UMinimapWidget::StaticClass(), TEXT("CameraBounds"));
			const FStructProperty* MapProperty = FindFProperty<FStructProperty>(UMinimapWidget::StaticClass(), TEXT("MapBounds"));
			const FBox2D& CameraBounds = *CameraProperty->ContainerPtrToValuePtr<FBox2D>(Map);
			const FBox2D& MapBounds = *MapProperty->ContainerPtrToValuePtr<FBox2D>(Map);
			Test->TestTrue(TEXT("Minimap camera bounds are valid at each aspect ratio"), CameraBounds.bIsValid && MapBounds.bIsValid);
			const ULocalPlayer* Local = Controller->GetLocalPlayer();
			FSceneViewProjectionData Projection;
			if (Local != nullptr && Local->GetProjectionData(Local->ViewportClient->Viewport, Projection))
			{
				const FIntRect Rect = Projection.GetConstrainedViewRect();
				FVector2D TopLeft, BottomRight;
				const double Plane = Player->GetActorLocation().Y;
				Controller->ProjectWorldLocationToScreen(FVector(CameraBounds.Min.X, Plane, -CameraBounds.Min.Y), TopLeft);
				Controller->ProjectWorldLocationToScreen(FVector(CameraBounds.Max.X, Plane, -CameraBounds.Max.Y), BottomRight);
				Test->AddInfo(FString::Printf(TEXT("Camera aspect %.3f: projected %s / %s, viewport %s / %s"), Camera->AspectRatio,
					*TopLeft.ToString(), *BottomRight.ToString(), *FVector2D(Rect.Min).ToString(), *FVector2D(Rect.Max).ToString()));
				Test->TestTrue(TEXT("Minimap top-left projects to the actual visible camera corner"), TopLeft.Equals(FVector2D(Rect.Min), 2.f));
				Test->TestTrue(TEXT("Minimap bottom-right projects to the actual visible camera corner"), BottomRight.Equals(FVector2D(Rect.Max), 2.f));
			}
			Test->TestTrue(TEXT("Expanded minimap contains the entire gameplay view"), MapBounds.IsInside(CameraBounds.Min) && MapBounds.IsInside(CameraBounds.Max));
			const FVector2D Area = Map->WidgetTree->FindWidget(TEXT("MapArea"))->GetCachedGeometry().GetLocalSize();
			Test->TestTrue(TEXT("Terrain bounds have the same aspect as the minimap without cropping"), FMath::IsNearlyEqual(MapBounds.GetSize().X / MapBounds.GetSize().Y, Area.X / Area.Y, 0.01));
			if (Step < 3)
			{
				Camera->SetAspectRatio(Step == 1 ? 16.f / 9.f : 21.f / 9.f);
				ChangedAt = World->GetTimeSeconds();
				Advance();
				return false;
			}
			const UGameSettingDataAsset* Settings = UDataManager::Get(World)->GetGameSettingDataAsset();
			const auto& Generation = Settings->MineableLevelGeneration;
			const float GroundZ = -Settings->SurfaceBlockPositionY * 16.f;
			int32 BackgroundCount = 0;
			for (TActorIterator<APaperTileMapActor> It(World); It; ++It)
			{
				if (It->ActorHasTag(TEXT("ArtBackground")) == false) { continue; }
				++BackgroundCount;
				FVector Center, Half;
				It->GetActorBounds(false, Center, Half);
				Test->TestTrue(TEXT("Background covers the generated world's left and right edges"), Center.X - Half.X <= Generation.StartColumn * 16.f
					&& Center.X + Half.X >= (Generation.StartColumn + Generation.WidthInBlocks) * 16.f);
				const bool bUnderground = Center.Z < GroundZ;
				Test->TestTrue(TEXT("Surface and underground meet exactly at ground height"), FMath::IsNearlyEqual(bUnderground ? Center.Z + Half.Z : Center.Z - Half.Z, GroundZ, 0.1));
				if (bUnderground) { Test->TestTrue(TEXT("Underground background reaches the world floor"), Center.Z - Half.Z <= GroundZ - Generation.DepthInBlocks * 16.f); }
			}
			Test->TestEqual(TEXT("Both art backgrounds were checked"), BackgroundCount, 2);
			Restore(Camera);
			return true;
		}

	private:
		void Advance() { ++Step; Started = FPlatformTime::Seconds(); }
		void Restore(UCameraComponent* Camera) { Camera->SetAspectRatio(SavedAspect); Camera->bConstrainAspectRatio = bSavedConstrain; }
		FAutomationTestBase* Test;
		double Started;
		int32 Step = 0;
		float SavedAspect = 0.f;
		float ChangedAt = 0.f;
		bool bSavedConstrain = false;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldPresentationTest, "ProjectMK.PIE.World.CameraMinimapAndBackgroundCoverage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldPresentationTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWorldPresentationCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
