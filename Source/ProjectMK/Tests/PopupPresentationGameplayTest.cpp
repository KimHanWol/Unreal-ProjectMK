#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CanvasPanel.h"
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
#include "ProjectMK/UI/ItemQuantityDialog.h"
#include "ProjectMK/UI/SkillSlotWidget.h"
#include "ProjectMK/UI/SkillTreeMenuContents.h"
#include "ProjectMK/UI/SkillConnectionWidget.h"
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
				TArray<UUserWidget*> Menus;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Menus, UMKMenuBase::StaticClass(), false);
				Menu = Menus.IsEmpty() ? nullptr : Cast<UMKMenuBase>(Menus[0]);
				if (Test->TestNotNull(TEXT("Common popup shell exists"), Menu.Get()) == false) { return true; }
				Test->TestTrue(TEXT("Popup dimming uses its own viewport layer above HUD elements"), Menu->IsInViewport());
				Menu->OpenMenu(EMenuContentsType::Shop);
				TArray<UUserWidget*> QuantityDialogs;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, QuantityDialogs, UItemQuantityDialog::StaticClass(), false);
				Test->TestTrue(TEXT("Shop and world quantity dialogs exist"), QuantityDialogs.Num() >= 2);
				for (UUserWidget* Dialog : QuantityDialogs) { Test->TestTrue(TEXT("Quantity dialog remains attached when popup moves to viewport"), Dialog->IsInViewport()); }
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
				UWidget* Header = Menu->WidgetTree->FindWidget(TEXT("WindowTitle"));
				UWidget* Close = Menu->WidgetTree->FindWidget(TEXT("ExitButton"));
				const FGeometry& HeaderGeometry = Header->GetCachedGeometry();
				const FGeometry& CloseGeometry = Close->GetCachedGeometry();
				const float HeaderCenter = HeaderGeometry.LocalToAbsolute(HeaderGeometry.GetLocalSize() * 0.5f).Y;
				const float CloseCenter = CloseGeometry.LocalToAbsolute(CloseGeometry.GetLocalSize() * 0.5f).Y;
				Test->TestTrue(TEXT("Title and close button share the same vertical center"), FMath::IsNearlyEqual(HeaderCenter, CloseCenter, 1.f));
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
				TArray<UUserWidget*> Trees;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Trees, USkillTreeMenuContents::StaticClass(), false);
				Trees.RemoveAll([](UUserWidget* Tree) { return Tree->GetVisibility() == ESlateVisibility::Collapsed || Tree->GetVisibility() == ESlateVisibility::Hidden; });
				if (Test->TestFalse(TEXT("Skill tree exists"), Trees.IsEmpty()))
				{
					USkillTreeMenuContents* Tree = CastChecked<USkillTreeMenuContents>(Trees[0]);
					const FArrayProperty* ConnectionsProperty = FindFProperty<FArrayProperty>(USkillTreeMenuContents::StaticClass(), TEXT("Connections"));
					const TArray<FSkillConnectionVisual>& Connections = *ConnectionsProperty->ContainerPtrToValuePtr<TArray<FSkillConnectionVisual>>(Tree);
					FName PurchaseKey;
					int32 RootConnections = 0;
					int32 HighestUnpurchasedLineLayer = 0;
					for (const FSkillConnectionVisual& Connection : Connections)
					{
						if (Player->GetSkillComponent()->HasPurchasedSkill(Connection.TargetSkillKey) == false)
						{
							HighestUnpurchasedLineLayer = FMath::Max(HighestUnpurchasedLineLayer, CastChecked<UCanvasPanelSlot>(Connection.Elements[1]->Slot)->GetZOrder());
						}
					}
					for (const FSkillConnectionVisual& Connection : Connections)
					{
						const USkillConnectionWidget* Rail = Cast<USkillConnectionWidget>(Connection.Elements[1]);
						const UImage* DirectionArrow = Cast<UImage>(Connection.Elements[0]);
						Test->TestTrue(TEXT("Each connection shares the reusable beveled rail widget"), Rail != nullptr);
						Test->TestTrue(TEXT("Rail and arrow always use the same state color"), Rail != nullptr && DirectionArrow != nullptr && Rail->GetLineColor().Equals(DirectionArrow->GetColorAndOpacity()));
						Test->TestTrue(TEXT("Arrows stay above learned and unlearned rails"), CastChecked<UCanvasPanelSlot>(Connection.Elements[0]->Slot)->GetZOrder() > 4);
						const FSkillDataTableRow* TargetRow = UDataManager::Get(World)->GetSkillDataTableRow(Connection.TargetSkillKey);
						Test->TestTrue(TEXT("Connections represent only the target's actual prerequisites"), TargetRow != nullptr && (TargetRow->RequiredSkill.IsEmpty() ? Connection.SourceSkillKey.IsNone() : TargetRow->RequiredSkill.Contains(Connection.SourceSkillKey)));
						for (UUserWidget* Node : Nodes)
						{
							if (Node->GetTypedOuter<USkillTreeMenuContents>() != Tree) { continue; }
							const FName Key = KeyProperty->GetPropertyValue_InContainer(Node);
							if (Key == Connection.SourceSkillKey || Key == Connection.TargetSkillKey) { continue; }
							const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Node->Slot);
							if (Slot == nullptr) { continue; }
							const FBox2D Obstacle(Slot->GetPosition() - FVector2D(2.f), Slot->GetPosition() + Slot->GetSize() + FVector2D(2.f));
							for (const FVector2D& Point : Connection.RoutePoints) { Test->TestFalse(TEXT("A route never passes through an unrelated skill"), Obstacle.IsInside(Point)); }
						}
						if (Player->GetSkillComponent()->HasPurchasedSkill(Connection.TargetSkillKey))
						{
							const UImage* Arrow = Cast<UImage>(Connection.Elements[0]);
							Test->TestTrue(TEXT("Opening the visible tree reflects previously loaded purchases"), Arrow != nullptr && Arrow->GetColorAndOpacity().G > Arrow->GetColorAndOpacity().R * 2.f);
							Test->TestTrue(TEXT("Purchased trunks render above overlapping unpurchased paths"), CastChecked<UCanvasPanelSlot>(Connection.Elements[1]->Slot)->GetZOrder() > HighestUnpurchasedLineLayer);
						}
						const FSkillDataTableRow* Row = UDataManager::Get(World)->GetSkillDataTableRow(Connection.TargetSkillKey);
						if (Row != nullptr && Row->RequiredSkill.IsEmpty())
						{
							++RootConnections;
							Test->TestTrue(TEXT("Each root skill has visible segments from the central character"), Connection.Elements.Num() >= 2);
							if (Player->GetSkillComponent()->HasPurchasedSkill(Connection.TargetSkillKey) == false) { PurchaseKey = Connection.TargetSkillKey; }
						}
					}
					Test->TestTrue(TEXT("The central character connects to first skills"), RootConnections > 0);
					if (PurchaseKey.IsNone() == false)
					{
						Player->GetSkillComponent()->TryAddCoin(UDataManager::Get(World)->GetSkillDataTableRow(PurchaseKey)->Price, false);
						Test->TestTrue(TEXT("Root fixture purchase succeeds"), Player->GetSkillComponent()->TryPurchaseSkill(PurchaseKey));
						for (const FSkillConnectionVisual& Connection : Connections)
						{
							if (Connection.TargetSkillKey != PurchaseKey) { continue; }
							UImage* Arrow = Cast<UImage>(Connection.Elements[0]);
							Test->TestTrue(TEXT("Purchase immediately recolors the existing connection teal"), Arrow != nullptr && Arrow->GetColorAndOpacity().G > Arrow->GetColorAndOpacity().R * 2.f);
							const USkillConnectionWidget* Rail = CastChecked<USkillConnectionWidget>(Connection.Elements[1]);
							Test->TestTrue(TEXT("Purchase recolors and promotes the whole rail above shared trunks"), Rail->GetLineColor().Equals(Arrow->GetColorAndOpacity()) && CastChecked<UCanvasPanelSlot>(Rail->Slot)->GetZOrder() > HighestUnpurchasedLineLayer);
						}
					}
				}
				Step = 3;
				Started = FPlatformTime::Seconds();
				return false;
			}
			if (Step == 3 && FPlatformTime::Seconds() - Started > 0.4)
			{
				TArray<UUserWidget*> Trees;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Trees, USkillTreeMenuContents::StaticClass(), false);
				USkillTreeMenuContents* Tree = nullptr;
				for (UUserWidget* Candidate : Trees) { if (Candidate->GetVisibility() == ESlateVisibility::Visible) { Tree = Cast<USkillTreeMenuContents>(Candidate); } }
				if (Test->TestNotNull(TEXT("Visible tree exists for input testing"), Tree))
				{
					UCanvasPanel* View = CastChecked<UCanvasPanel>(Tree->WidgetTree->FindWidget(TEXT("RuntimeSkillTreeViewport")));
					UCanvasPanel* Canvas = CastChecked<UCanvasPanel>(View->GetChildAt(0));
					UCanvasPanelSlot* CanvasLayout = CastChecked<UCanvasPanelSlot>(Canvas->Slot);
					Player->GetSkillComponent()->TryAddCoin(5000, false);
					USkillSlotWidget* DragSlot = nullptr;
					const FNameProperty* Key = FindFProperty<FNameProperty>(USkillSlotWidget::StaticClass(), TEXT("SkillKeyName"));
					for (UWidget* Child : Canvas->GetAllChildren())
					{
						USkillSlotWidget* Slot = Cast<USkillSlotWidget>(Child);
						if (Slot != nullptr && Player->GetSkillComponent()->CanPurchaseSkill(Key->GetPropertyValue_InContainer(Slot))) { DragSlot = Slot; break; }
					}
					if (Test->TestNotNull(TEXT("An available slot can start a drag"), DragSlot))
					{
						const FGeometry Geometry = DragSlot->GetCachedGeometry();
						const FVector2D Start = Geometry.LocalToAbsolute(Geometry.GetLocalSize() * 0.5f);
						const FVector2D End = Start + FVector2D(45.f, 30.f);
						const FVector2D Before = CanvasLayout->GetPosition();
						FPointerEvent Down(0, Start, Start, TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0.f, FModifierKeysState());
						DragSlot->TakeWidget()->OnMouseButtonDown(Geometry, Down);
						FPointerEvent Move(0, End, Start, TSet<FKey>{EKeys::LeftMouseButton}, EKeys::Invalid, 0.f, FModifierKeysState());
						Test->TestTrue(TEXT("Moving an available slot cancels hold and begins tree drag"), DragSlot->TakeWidget()->OnMouseMove(Geometry, Move).IsEventHandled() && Tree->HasActiveSkillTreeDrag());
						Test->TestFalse(TEXT("Dragging actually moves the canvas"), Before.Equals(CanvasLayout->GetPosition()));
						FPointerEvent Up(0, End, End, TSet<FKey>(), EKeys::LeftMouseButton, 0.f, FModifierKeysState());
						Test->TestTrue(TEXT("Ending drag releases pointer capture"), Tree->TakeWidget()->OnMouseButtonUp(Tree->GetCachedGeometry(), Up).ShouldReleaseMouse());
						Test->TestFalse(TEXT("Drag state ends on release"), Tree->HasActiveSkillTreeDrag());
					}
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
