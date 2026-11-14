#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/WorkbenchActor.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/UI/MKMenuBase.h"

namespace
{
	class FWorkbenchCommand : public IAutomationLatentCommand
	{
	public:
		explicit FWorkbenchCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			AMKCharacter* Player = World != nullptr ? Cast<AMKCharacter>(World->GetFirstPlayerController()->GetPawn()) : nullptr;
			if (Player == nullptr || Player->IsOnGround() == false || World->GetTimeSeconds() < 0.5f)
			{
				if (FPlatformTime::Seconds() - Started > 20.0) { Test->AddError(TEXT("Workbench fixture did not reach a settled floor.")); return true; }
				return false;
			}
			USaveGameSubsystem::Get(World)->StartPlaytestSession();
			UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
			const TArray<FInventorySlotData> Saved = Inventory->GetInventorySlotDataList();
			Inventory->ApplySavedInventorySlots({});
			UConsumableComponent* Consumables = Player->FindComponentByClass<UConsumableComponent>();
			const FName ItemKey(TEXT("Workbench"));
			const FItemDataTableRow* Data = UDataManager::Get(World)->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemKey);
			if (Test->TestTrue(TEXT("Workbench item has a placeable consumable effect"), Data != nullptr && Data->ConsumableEffect == EConsumableEffectType::PlaceWorkbench) == false) { return true; }
			Inventory->AddItem(ItemKey, 2);
			Test->TestTrue(TEXT("Using a workbench places it on the current floor"), Consumables->TryUseItem(ItemKey));
			Test->TestEqual(TEXT("Successful placement consumes exactly one workbench"), Inventory->GetItemCount(ItemKey), 1);
			Test->TestFalse(TEXT("Placing a second bench over the first is rejected"), Consumables->TryUseItem(ItemKey));
			Test->TestEqual(TEXT("Rejected placement preserves the item"), Inventory->GetItemCount(ItemKey), 1);
			AWorkbenchActor* Bench = nullptr;
			for (TActorIterator<AWorkbenchActor> It(World); It; ++It) { Bench = *It; break; }
			if (Test->TestNotNull(TEXT("The placed bench exists in the world"), Bench))
			{
				Test->TestTrue(TEXT("The nearby bench uses the common interactable prompt query"), Bench->CanInteract(Player));
				Test->TestTrue(TEXT("Common interaction opens crafting"), Bench->TryInteract(Player));
				TArray<UUserWidget*> Menus;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Menus, UMKMenuBase::StaticClass(), false);
				UMKMenuBase* Menu = Menus.Num() > 0 ? Cast<UMKMenuBase>(Menus[0]) : nullptr;
				Test->TestTrue(TEXT("The reusable crafting popup is open"), Menu != nullptr && Menu->IsMenuOpen(EMenuContentsType::Crafting));
				if (Menu != nullptr) { Menu->CloseMenu(); }
				const FVector Origin = Player->GetActorLocation();
				Player->SetActorLocation(Origin + FVector(100.f, 0.f, 0.f));
				Test->TestFalse(TEXT("A distant bench cannot be interacted with"), Bench->TryInteract(Player));
				Player->SetActorLocation(Origin);
				Bench->Destroy();
			}
			Inventory->ApplySavedInventorySlots(Saved);
			return true;
		}

	private:
		FAutomationTestBase* Test;
		double Started;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorkbenchTest, "ProjectMK.PIE.Crafting.PlaceableWorkbench", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorkbenchTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWorkbenchCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
