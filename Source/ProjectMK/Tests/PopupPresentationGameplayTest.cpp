#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"
#include "ProjectMK/UI/CraftingMenuContents.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "ProjectMK/UI/MKMenuBase.h"
#include "ProjectMK/UI/MKTooltipBase.h"
#include "ProjectMK/UI/SkillSlotWidget.h"
#include "Widgets/SWidget.h"

namespace
{
	class FPopupPresentationCommand : public IAutomationLatentCommand
	{
	public:
		explicit FPopupPresentationCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			UWorld* World = GEditor->PlayWorld;
			APlayerController* Controller = ::IsValid(World) ? World->GetFirstPlayerController() : nullptr;
			AMKCharacter* Player = ::IsValid(Controller) ? Cast<AMKCharacter>(Controller->GetPawn()) : nullptr;
			if (::IsValid(Player) == false) { return FPlatformTime::Seconds() - Started > 15.0; }
			if (Step == 0)
			{
				USaveGameSubsystem::Get(World)->StartPlaytestSession();
				Test->TestTrue(TEXT("Collection sphere stays hidden during play"), Player->FindComponentByClass<UInventoryComponent>()->bHiddenInGame);
				UDataManager* Data = UDataManager::Get(World);
				int32 PassiveCount = 0;
				for (FName Key : Data->GetSkillRowNames())
				{
					const FSkillDataTableRow* Row = Data->GetSkillDataTableRow(Key);
					if (Row != nullptr && Row->AcquisitionType == ESkillAcquisitionType::CharacterPassive)
					{
						++PassiveCount;
						Test->TestFalse(TEXT("Character passive cannot be purchased"), Player->GetSkillComponent()->CanPurchaseSkill(Key));
					}
				}
				Test->TestEqual(TEXT("Four character passives are classified in the table"), PassiveCount, 4);
				TArray<UUserWidget*> HUDs;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, HUDs, UHUDWidget::StaticClass(), false);
				if (Test->TestFalse(TEXT("HUD exists"), HUDs.IsEmpty()) == false) { return true; }
				UHUDWidget* HUD = CastChecked<UHUDWidget>(HUDs[0]);
				Test->TestTrue(TEXT("Tooltip has its own viewport layer above the quickbar"), HUD->GetTooltipWidget()->IsInViewport());
				Menu = Cast<UMKMenuBase>(HUD->WidgetTree->FindWidget(TEXT("Menu")));
				if (Test->TestNotNull(TEXT("Common popup shell exists"), Menu.Get()) == false) { return true; }
				Menu->OpenMenu(EMenuContentsType::Crafting);
				TArray<UUserWidget*> Contents;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Contents, UCraftingMenuContents::StaticClass(), false);
				for (UUserWidget* Content : Contents)
				{
					Content->WidgetTree->ForEachWidget([this](UWidget* Widget)
					{
						if (UScrollBox* Candidate = Cast<UScrollBox>(Widget))
						{
							if (Candidate->GetChildrenCount() > 0 && Candidate->GetChildAt(0)->GetName() == TEXT("RecipeSlotListPanel")) { Scroll = Candidate; }
						}
					});
				}
				Started = FPlatformTime::Seconds();
				Step = 1;
				return false;
			}
			if (Step == 1 && FPlatformTime::Seconds() - Started > 0.6)
			{
				if (Test->TestNotNull(TEXT("Recipe scroll area exists"), Scroll.Get()) == false) { return true; }
				const FScrollBarStyle& Bar = Scroll->GetWidgetBarStyle();
				Test->TestTrue(TEXT("Wheel scrolling uses interpolation"), Scroll->IsAnimateWheelScrolling());
				Test->TestNotNull(TEXT("Scrollbar thumb uses the authored sprite"), Bar.NormalThumbImage.GetResourceObject());
				Test->TestTrue(TEXT("Scrollbar rail has a separate brush"), Bar.VerticalBackgroundImage.DrawAs != Bar.NormalThumbImage.DrawAs);
				InitialY = Scroll->GetChildAt(0)->GetCachedGeometry().GetAbsolutePosition().Y;
				LastY = InitialY;
				const FGeometry Geometry = Scroll->GetCachedGeometry();
				const FVector2D Position = Geometry.GetAbsolutePosition();
				const FPointerEvent Pointer(0, Position, Position, TSet<FKey>(), EKeys::Invalid, -3.f, FModifierKeysState());
				Test->TestTrue(TEXT("Recipe list accepts the wheel event"), Scroll->TakeWidget()->OnMouseWheel(Geometry, Pointer).IsEventHandled());
				Started = FPlatformTime::Seconds();
				Step = 2;
				return false;
			}
			if (Step == 2)
			{
				const float Y = Scroll->GetChildAt(0)->GetCachedGeometry().GetAbsolutePosition().Y;
				if (FMath::Abs(Y - LastY) > 0.05f) { ++MovingFrames; LastY = Y; }
				if (FPlatformTime::Seconds() - Started < 0.7) { return false; }
				Test->TestTrue(TEXT("Wheel content travels across multiple rendered positions"), MovingFrames >= 3);
				Test->TestTrue(TEXT("Wheel content actually moves upward"), LastY < InitialY - 10.f);
				Menu->OpenMenu(EMenuContentsType::SkillTree);
				TArray<UUserWidget*> Nodes;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Nodes, USkillSlotWidget::StaticClass(), false);
				const FNameProperty* KeyProperty = FindFProperty<FNameProperty>(USkillSlotWidget::StaticClass(), TEXT("SkillKeyName"));
				for (UUserWidget* Node : Nodes)
				{
					const FName Key = KeyProperty->GetPropertyValue_InContainer(Node);
					const FSkillDataTableRow* Row = UDataManager::Get(World)->GetSkillDataTableRow(Key);
					Test->TestTrue(TEXT("Only purchasable skills have tree nodes"), Row != nullptr && Row->AcquisitionType == ESkillAcquisitionType::SkillTree);
				}
				Menu->CloseMenu();
				return true;
			}
			return false;
		}

	private:
		FAutomationTestBase* Test;
		TWeakObjectPtr<UMKMenuBase> Menu;
		TWeakObjectPtr<UScrollBox> Scroll;
		double Started;
		int32 Step = 0;
		int32 MovingFrames = 0;
		float InitialY = 0.f;
		float LastY = 0.f;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPopupPresentationTest, "ProjectMK.PIE.UI.PopupScrollAndPassiveClassification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPopupPresentationTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FPopupPresentationCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
