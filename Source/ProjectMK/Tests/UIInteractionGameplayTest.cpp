#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/GridPanel.h"
#include "Components/Image.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/World.h"
#include "Slate/UMGDragDropOp.h"
#include "Tests/AutomationEditorCommon.h"
#include "Widgets/SWidget.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/UI/InventoryWidget.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "ProjectMK/UI/ItemQuantityDialog.h"
#include "ProjectMK/UI/ItemSlotWidget.h"
#include "ProjectMK/UI/ShopMenuContents.h"
#include "ProjectMK/UI/ShopRecipeSlotWidget.h"
#include "ProjectMK/UI/MKTooltipBase.h"
#include "ProjectMK/UI/MKMenuBase.h"
#include "ProjectMK/UI/SkillSlotWidget.h"

namespace
{
	class FUIInteractionCommand : public IAutomationLatentCommand
	{
	public:
		explicit FUIInteractionCommand(FAutomationTestBase* InTest) : Test(InTest), StartedAt(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			if (Step == 0)
			{
				UWorld* World = GEditor->PlayWorld;
				APlayerController* Controller = ::IsValid(World) ? World->GetFirstPlayerController() : nullptr;
				Player = ::IsValid(Controller) ? Cast<AMKCharacter>(Controller->GetPawn()) : nullptr;
				if (Player.IsValid() == false)
				{
					if (Elapsed() < 15.0) { return false; }
					Test->AddError(TEXT("UI fixture could not find the PIE player."));
					return true;
				}
				if (USaveGameSubsystem::Get(World)->StartPlaytestSession() == false) { return true; }
				UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
				SavedSlots = Inventory->GetInventorySlotDataList();
				Inventory->ApplySavedInventorySlots({});
				UDataManager* Data = UDataManager::Get(World);
				for (const FName Key : Data->GetDataTable(EDataTableType::Item)->GetRowNames())
				{
					const FItemDataTableRow* Row = Data->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, Key);
					if (Row->bIsOre && Row->SellPrice > 0 && Row->bIsInventoryItem) { ItemKey = Key; break; }
				}
				if (Test->TestFalse(TEXT("A sellable ore exists"), ItemKey.IsNone()) == false) { return Finish(); }
				if (Test->TestTrue(TEXT("Ore fixture fits the inventory"), Inventory->AddItem(ItemKey, 5)) == false) { return Finish(); }
				Shop = CreateWidget<UShopMenuContents>(Controller);
				Shop->SetDesiredSizeInViewport(FVector2D(900.f, 700.f));
				Shop->AddToViewport(50);
				Advance();
				return false;
			}
			if (Step == 1 && Elapsed() > 0.5)
			{
				TArray<UWidget*> ShopWidgets;
				Shop->WidgetTree->GetAllWidgets(ShopWidgets);
				UInventoryWidget* InventoryWidget = nullptr;
				UShopRecipeSlotWidget* RecipeEntry = nullptr;
				for (UWidget* Widget : ShopWidgets)
				{
					if (UInventoryWidget* Candidate = Cast<UInventoryWidget>(Widget)) { InventoryWidget = Candidate; }
					if (UShopRecipeSlotWidget* Candidate = Cast<UShopRecipeSlotWidget>(Widget)) { RecipeEntry = Candidate; }
				}
				UItemSlotWidget* Source = nullptr;
				if (::IsValid(InventoryWidget))
				{
					UGridPanel* Grid = Cast<UGridPanel>(InventoryWidget->WidgetTree->FindWidget(TEXT("ItemSlotPanel")));
					Source = ::IsValid(Grid) ? Cast<UItemSlotWidget>(Grid->GetChildAt(0)) : nullptr;
				}
				if (Test->TestNotNull(TEXT("Shop reuses the inventory slot grid"), Source) == false) { return Finish(); }
				CheckHover(Source);
				const FGeometry Geometry = Source->GetCachedGeometry();
				const FVector2D Position = Geometry.GetAbsolutePosition();
				const FPointerEvent Pointer(0, Position, Position, TSet<FKey>{ EKeys::LeftMouseButton }, EKeys::LeftMouseButton, 0.f, FModifierKeysState());
				if (::IsValid(RecipeEntry))
				{
					RecipeEntry->TakeWidget()->OnMouseEnter(RecipeEntry->GetCachedGeometry(), Pointer);
					TArray<UUserWidget*> FoundHUDs;
					UWidgetBlueprintLibrary::GetAllWidgetsOfClass(Player.Get(), FoundHUDs, UHUDWidget::StaticClass(), false);
					UMKTooltipBase* RecipeTooltip = CastChecked<UHUDWidget>(FoundHUDs[0])->GetTooltipWidget();
					UVerticalBox* Materials = Cast<UVerticalBox>(RecipeTooltip->WidgetTree->FindWidget(TEXT("TooltipMaterials")));
					Test->TestTrue(TEXT("Recipe hover shows the common materials tooltip"), RecipeTooltip->IsVisible() && ::IsValid(Materials) && Materials->GetChildrenCount() > 0);
					UTextBlock* RecipeTitle = Cast<UTextBlock>(RecipeTooltip->WidgetTree->FindWidget(TEXT("RuntimeTooltipTitle")));
					UTextBlock* RecipeDescription = Cast<UTextBlock>(RecipeTooltip->WidgetTree->FindWidget(TEXT("RuntimeTooltipDescription")));
					Test->TestTrue(TEXT("Shop tooltip names the blueprint instead of its crafted result"), RecipeTitle->GetText().ToString().EndsWith(TEXT(" 설계도")));
					Test->TestTrue(TEXT("Shop tooltip explains recipe unlocking"), RecipeDescription->GetText().ToString().Contains(TEXT("레시피를 해금")));
					RecipeEntry->TakeWidget()->OnMouseLeave(Pointer);
				}
				FReply DragReply = Source->TakeWidget()->OnDragDetected(Geometry, Pointer);
				const TSharedPtr<FDragDropOperation> Operation = DragReply.GetDragDropContent();
				if (Test->TestTrue(TEXT("Inventory begins a real UMG drag"), Operation.IsValid()) == false) { return Finish(); }
				TSharedPtr<FUMGDragDropOp> UMGOperation = StaticCastSharedPtr<FUMGDragDropOp>(Operation);
				UDragDropOperation* ItemOperation = UMGOperation->GetOperation();
				ItemOperation->DefaultDragVisual->TakeWidget()->SlatePrepass();
				Test->TestTrue(TEXT("Drag ghost uses the same 40px icon size"), ItemOperation->DefaultDragVisual->GetDesiredSize().Equals(FVector2D(40.f), 0.1f));
				Test->TestEqual(TEXT("Drag ghost is centered on the mouse"), ItemOperation->Pivot, EDragPivot::CenterCenter);
				const FVector2D DropPosition = Shop->GetCachedGeometry().LocalToAbsolute(FVector2D(100.f, 280.f));
				const FPointerEvent DropPointer(0, DropPosition, Position, TSet<FKey>(), EKeys::LeftMouseButton, 0.f, FModifierKeysState());
				const FReply DropReply = Shop->TakeWidget()->OnDrop(Shop->GetCachedGeometry(), FDragDropEvent(DropPointer, Operation.ToSharedRef()));
				Test->TestTrue(TEXT("Merchant accepts the inventory drop"), DropReply.IsEventHandled());
				TArray<UUserWidget*> SaleDialogs;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(Player.Get(), SaleDialogs, UItemQuantityDialog::StaticClass(), false);
				for (UUserWidget* Candidate : SaleDialogs) { if (Candidate->IsVisible()) { Dialog = Cast<UItemQuantityDialog>(Candidate); break; } }
				Test->TestTrue(TEXT("Drop opens the quantity dialog"), Dialog.IsValid() && Dialog->IsVisible());
				if (Dialog.IsValid() == false || Dialog->IsVisible() == false) { return Finish(); }
				Test->TestTrue(TEXT("Sale quantity modal covers the full viewport"), Dialog->IsInViewport());
				UButton* CancelStyleButton = Cast<UButton>(Dialog->WidgetTree->FindWidget(TEXT("QuantityCancel")));
				UButton* PrimaryStyleButton = Cast<UButton>(Dialog->WidgetTree->FindWidget(TEXT("QuantityConfirm")));
				UButton* StepperStyleButton = Cast<UButton>(Dialog->WidgetTree->FindWidget(TEXT("QuantityDecrease")));
				Test->TestTrue(TEXT("Primary, secondary and stepper use distinct artwork"), CancelStyleButton->GetStyle().Normal.GetResourceObject() != PrimaryStyleButton->GetStyle().Normal.GetResourceObject()
					&& StepperStyleButton->GetStyle().Normal.GetResourceObject() != PrimaryStyleButton->GetStyle().Normal.GetResourceObject());
				USpinBox* Quantity = nullptr;
				Dialog->WidgetTree->ForEachWidget([&Quantity](UWidget* Widget) { if (USpinBox* Input = Cast<USpinBox>(Widget)) { Quantity = Input; } });
				CastChecked<UButton>(Dialog->WidgetTree->FindWidget(TEXT("QuantityHalf")))->OnClicked.Broadcast();
				Test->TestEqual(TEXT("Half preset selects two from a stack of five"), Quantity->GetValue(), 2.f);
				CastChecked<UButton>(Dialog->WidgetTree->FindWidget(TEXT("QuantityAll")))->OnClicked.Broadcast();
				Test->TestEqual(TEXT("All preset restores the selected stack quantity"), Quantity->GetValue(), 5.f);
				InitialCoin = Player->GetAbilitySystemComponent()->GetNumericAttribute(UAttributeSet_Character::GetCoinAttribute());
				SalePrice = Player->FindComponentByClass<UInventoryComponent>()->CalculateSellPrice(ItemKey, 2);
				UButton* Confirm = Cast<UButton>(Dialog->WidgetTree->FindWidget(TEXT("QuantityConfirm")));
				Dialog->WidgetTree->ForEachWidget([](UWidget* Widget)
				{
					if (USpinBox* Input = Cast<USpinBox>(Widget)) { Input->SetValue(2.f); }
				});
				if (Test->TestNotNull(TEXT("Sale confirmation button exists"), Confirm) == false) { return Finish(); }
				Confirm->OnClicked.Broadcast();
				Test->TestEqual(TEXT("Only the selected quantity is sold"), Player->FindComponentByClass<UInventoryComponent>()->GetItemCount(ItemKey), 3);
				Test->TestEqual(TEXT("Sale adds the correct currency"), Player->GetAbilitySystemComponent()->GetNumericAttribute(UAttributeSet_Character::GetCoinAttribute()), InitialCoin + SalePrice);
				Test->TestFalse(TEXT("Confirmation closes the modal"), Dialog->IsVisible());
				const FPointerEvent SecondaryPointer(0, Position, Position, TSet<FKey>{ EKeys::RightMouseButton }, EKeys::RightMouseButton, 0.f, FModifierKeysState());
				Test->TestTrue(TEXT("Right click opens the same sale flow"), Source->TakeWidget()->OnPreviewMouseButtonDown(Geometry, SecondaryPointer).IsEventHandled());
				Test->TestTrue(TEXT("Right click shows the sale quantity modal"), Dialog->IsVisible());
				CastChecked<UButton>(Dialog->WidgetTree->FindWidget(TEXT("QuantityCancel")))->OnClicked.Broadcast();
				Test->TestEqual(TEXT("Cancelling secondary sale preserves the items"), Player->FindComponentByClass<UInventoryComponent>()->GetItemCount(ItemKey), 3);
				Shop->RemoveFromParent();
				TArray<UUserWidget*> HUDs;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(Player.Get(), HUDs, UHUDWidget::StaticClass(), false);
				UHUDWidget* HUD = HUDs.IsEmpty() ? nullptr : Cast<UHUDWidget>(HUDs[0]);
				if (Test->TestNotNull(TEXT("HUD provides the world drop target"), HUD) == false) { return Finish(); }
				TArray<UUserWidget*> FoundMenus;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(Player.Get(), FoundMenus, UMKMenuBase::StaticClass(), false);
				UMKMenuBase* Popup = CastChecked<UMKMenuBase>(FoundMenus[0]);
				Popup->OpenMenu(EMenuContentsType::Crafting);
				const FVector2D WorldPoint = HUD->GetCachedGeometry().LocalToAbsolute(HUD->GetCachedGeometry().GetLocalSize() * FVector2D(0.97f, 0.3f));
				const FPointerEvent WorldPointer(0, WorldPoint, Position, TSet<FKey>(), EKeys::LeftMouseButton, 0.f, FModifierKeysState());
				Test->TestTrue(TEXT("Dropping outside the viewport popup opens the world quantity flow"), Popup->TakeWidget()->OnDrop(Popup->GetCachedGeometry(), FDragDropEvent(WorldPointer, Operation.ToSharedRef())).IsEventHandled());
				TArray<UUserWidget*> Dialogs;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(Player.Get(), Dialogs, UItemQuantityDialog::StaticClass(), false);
				UItemQuantityDialog* DropDialog = nullptr;
				for (UUserWidget* Candidate : Dialogs) { if (Candidate->IsVisible()) { DropDialog = Cast<UItemQuantityDialog>(Candidate); break; } }
				if (Test->TestNotNull(TEXT("World drop quantity dialog is visible"), DropDialog) == false) { return Finish(); }
				UButton* DropConfirm = Cast<UButton>(DropDialog->WidgetTree->FindWidget(TEXT("QuantityConfirm")));
				Test->TestFalse(TEXT("Drop quantity does not show a money amount"), DropDialog->WidgetTree->FindWidget(TEXT("QuantityPriceRow"))->IsVisible());
				DropDialog->WidgetTree->ForEachWidget([](UWidget* Widget)
				{
					if (USpinBox* Input = Cast<USpinBox>(Widget)) { Input->SetValue(1.f); }
				});
				if (DropConfirm != nullptr) { DropConfirm->OnClicked.Broadcast(); }
				Test->TestEqual(TEXT("World quantity confirmation removes only one item"), Player->FindComponentByClass<UInventoryComponent>()->GetItemCount(ItemKey), 2);
				Popup->CloseMenu();
				for (const FName Key : UDataManager::Get(Player.Get())->GetSkillRowNames())
				{
					if (Player->GetSkillComponent()->CanPurchaseSkill(Key)) { SkillKey = Key; break; }
				}
				if (Test->TestFalse(TEXT("An unpurchased skill is available"), SkillKey.IsNone()) == false) { return Finish(); }
				Skill = CreateWidget<USkillSlotWidget>(Player->GetController<APlayerController>());
				Skill->AddToViewport(50);
				Skill->SetDesiredSizeInViewport(FVector2D(96.f));
				Skill->InitializeSkill(SkillKey);
				Advance();
				return false;
			}
			if (Step == 2 && Elapsed() > 0.4)
			{
				CheckHover(Skill.Get());
				TArray<UUserWidget*> FoundHUDs;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(Player.Get(), FoundHUDs, UHUDWidget::StaticClass(), false);
				UMKTooltipBase* SkillTooltip = CastChecked<UHUDWidget>(FoundHUDs[0])->GetTooltipWidget();
				FSkillDataTableRow* SkillData = const_cast<FSkillDataTableRow*>(UDataManager::Get(Player.Get())->GetSkillDataTableRow(SkillKey));
				const int32 SavedPrice = SkillData->Price;
				SkillData->Price = MAX_int32;
				SkillTooltip->ShowSkillTooltip(SkillKey);
				UTextBlock* State = Cast<UTextBlock>(SkillTooltip->WidgetTree->FindWidget(TEXT("TooltipStatus")));
				Test->TestEqual(TEXT("Unaffordable skill replaces the hold prompt"), State->GetText().ToString(), FString(TEXT("코인 부족")));
				SkillData->Price = SavedPrice;
				SkillTooltip->HideTooltip();
				SendSkillPointer(true);
				Advance();
				return false;
			}
			if (Step == 3 && Elapsed() > 0.15)
			{
				SendSkillPointer(false);
				Test->TestFalse(TEXT("A short click does not buy the skill"), Player->GetSkillComponent()->HasPurchasedSkill(SkillKey));
				SendSkillPointer(true);
				Advance();
				return false;
			}
			if (Step == 4 && Elapsed() > 1.1)
			{
				Test->TestTrue(TEXT("Holding buys the skill"), Player->GetSkillComponent()->HasPurchasedSkill(SkillKey));
				SendSkillPointer(false);
				return Finish();
			}
			return false;
		}

	private:
		double Elapsed() const { return FPlatformTime::Seconds() - StartedAt; }
		void Advance() { ++Step; StartedAt = FPlatformTime::Seconds(); }
		void CheckHover(UMKSlotBase* Slot)
		{
			const FGeometry Geometry = Slot->GetCachedGeometry();
			const FVector2D Position = Geometry.GetAbsolutePosition();
			const FPointerEvent Pointer(0, Position, Position, TSet<FKey>(), EKeys::Invalid, 0.f, FModifierKeysState());
			Slot->TakeWidget()->OnMouseEnter(Geometry, Pointer);
			TArray<UUserWidget*> Found;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(Player.Get(), Found, UHUDWidget::StaticClass(), false);
			UHUDWidget* HUD = Found.IsEmpty() ? nullptr : Cast<UHUDWidget>(Found[0]);
			UMKTooltipBase* Tooltip = ::IsValid(HUD) ? HUD->GetTooltipWidget() : nullptr;
			Test->TestTrue(TEXT("Slot hover opens the common tooltip"), ::IsValid(Tooltip) && Tooltip->IsVisible());
			UTextBlock* Title = ::IsValid(Tooltip) ? Cast<UTextBlock>(Tooltip->WidgetTree->FindWidget(TEXT("RuntimeTooltipTitle"))) : nullptr;
			Test->TestTrue(TEXT("Hovered tooltip contains readable title text"), ::IsValid(Title) && Title->GetText().IsEmpty() == false);
			UImage* Icon = Cast<UImage>(Slot->WidgetTree->FindWidget(TEXT("IconImage")));
			Test->TestTrue(TEXT("Hover preserves the item or skill icon"), ::IsValid(Icon) && ::IsValid(Icon->GetBrush().GetResourceObject()));
			Slot->TakeWidget()->OnMouseLeave(Pointer);
			Test->TestFalse(TEXT("Leaving the slot closes its tooltip"), ::IsValid(Tooltip) && Tooltip->IsVisible());
		}
		void SendSkillPointer(bool bDown)
		{
			const FGeometry Geometry = Skill->GetCachedGeometry();
			const FVector2D Position = Geometry.GetAbsolutePosition();
			const FPointerEvent Pointer(0, Position, Position, bDown ? TSet<FKey>{ EKeys::LeftMouseButton } : TSet<FKey>(), EKeys::LeftMouseButton, 0.f, FModifierKeysState());
			if (bDown) { Skill->TakeWidget()->OnMouseButtonDown(Geometry, Pointer); }
			else { Skill->TakeWidget()->OnMouseButtonUp(Geometry, Pointer); }
		}
		bool Finish()
		{
			if (Shop.IsValid()) { Shop->RemoveFromParent(); }
			if (Skill.IsValid()) { Skill->RemoveFromParent(); }
			if (Player.IsValid()) { Player->FindComponentByClass<UInventoryComponent>()->ApplySavedInventorySlots(SavedSlots); }
			return true;
		}
		FAutomationTestBase* Test;
		TWeakObjectPtr<AMKCharacter> Player;
		TWeakObjectPtr<UShopMenuContents> Shop;
		TWeakObjectPtr<UItemQuantityDialog> Dialog;
		TWeakObjectPtr<USkillSlotWidget> Skill;
		TArray<FInventorySlotData> SavedSlots;
		FName ItemKey;
		FName SkillKey;
		float InitialCoin = 0.f;
		int32 SalePrice = 0;
		int32 Step = 0;
		double StartedAt;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIInteractionTest, "ProjectMK.PIE.UI.DragSaleAndSkillHold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIInteractionTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FUIInteractionCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
