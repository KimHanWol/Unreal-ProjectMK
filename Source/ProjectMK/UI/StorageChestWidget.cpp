#include "ProjectMK/UI/StorageChestWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/StorageChestActor.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "ProjectMK/UI/ItemSlotWidget.h"
#include "ProjectMK/UI/ItemSlotDragDropOperation.h"

TSharedRef<SWidget> UStorageChestWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget == nullptr) { BuildDefaultWidgetTree(); }
	return Super::RebuildWidget();
}

bool UStorageChestWidget::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	return Cast<UItemSlotDragDropOperation>(Operation) != nullptr || Super::NativeOnDrop(Geometry, Event, Operation);
}

void UStorageChestWidget::UpdateWidget() { RefreshSlots(); }

void UStorageChestWidget::BindEvents()
{
	Super::BindEvents();
	GetLocalInventoryComponent()->OnInventoryChangedDelegate.AddUObject(this, &UStorageChestWidget::RefreshSlots);
	CloseButton->OnClicked.AddUniqueDynamic(this, &UStorageChestWidget::CloseChest);
}

void UStorageChestWidget::UnbindEvents()
{
	CloseChest();
	if (UInventoryComponent* Items = GetLocalInventoryComponent()) { Items->OnInventoryChangedDelegate.RemoveAll(this); }
	CloseButton->OnClicked.RemoveDynamic(this, &UStorageChestWidget::CloseChest);
	Super::UnbindEvents();
}

bool UStorageChestWidget::OpenChest(AStorageChestActor* Chest)
{
	if (::IsValid(Chest) == false || Chest->CanInteract(GetLocalPlayerCharacter()) == false) { return false; }
	if (StorageChest.Get() == Chest && IsVisible()) { CloseChest(); return true; }
	SetChest(Chest);
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	GetWorld()->GetTimerManager().SetTimer(RangeTimer, this, &UStorageChestWidget::UpdateInteractionRange, 0.2f, true);
	return true;
}

void UStorageChestWidget::CloseChest()
{
	if (GetWorld() != nullptr) { GetWorld()->GetTimerManager().ClearTimer(RangeTimer); }
	SetChest(nullptr);
	SetVisibility(ESlateVisibility::Collapsed);
	if (UHUDWidget* HUD = GetHUDWidget()) { HUD->SetHoveredSlot(nullptr); }
}

void UStorageChestWidget::BuildDefaultWidgetTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(396.f);
	UCanvasPanelSlot* Position = Canvas->AddChildToCanvas(Size);
	Position->SetAnchors(FAnchors(0.5f));
	Position->SetAlignment(FVector2D(0.5f));
	Position->SetPosition(FVector2D(-190.f, -40.f));
	Position->SetAutoSize(true);
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StorageWindowFrame"));
	ApplyBorderStyle(Panel, TEXT("Panel"));
	Panel->SetPadding(FMargin(16.f));
	Size->SetContent(Panel);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Column);
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	Header->AddChildToHorizontalBox(BuildSectionHeader(NSLOCTEXT("Storage", "Title", "보관 상자"), TEXT("IconStorage")))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UTextBlock* UnusedLabel = nullptr;
	UButton* Close = nullptr;
	USizeBox* CloseSize = BuildActionButton(Close, UnusedLabel, FText::GetEmpty(), 36.f, 36.f, TEXT("ActionSecondary"));
	CloseButton = Close;
	UImage* CloseArt = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Close"));
	CloseArt->SetDesiredSizeOverride(FVector2D(20.f));
	CloseButton->SetContent(CloseArt);
	UHorizontalBoxSlot* CloseLayout = Header->AddChildToHorizontalBox(CloseSize);
	CloseLayout->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	CloseLayout->SetVerticalAlignment(VAlign_Center);
	UBorder* Drawer = WidgetTree->ConstructWidget<UBorder>();
	ApplyBorderStyle(Drawer, TEXT("InventoryDrawer"));
	Drawer->SetPadding(FMargin(12.f, 24.f, 12.f, 20.f));
	Column->AddChildToVerticalBox(Drawer)->SetHorizontalAlignment(HAlign_Center);
	StorageGrid = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), TEXT("StorageSlotGrid"));
	Drawer->SetContent(StorageGrid);
	for (int32 Index = 0; Index < AStorageChestActor::SlotCount; ++Index)
	{
		UItemSlotWidget* ItemSlot = CreateWidget<UItemSlotWidget>(this);
		StorageGrid->AddChildToGrid(ItemSlot, Index / 5, Index % 5)->SetPadding(FMargin(2.f));
		ItemSlot->SetSlotAddress(EItemContainer::Storage, Index, StorageChest.Get());
		ItemSlot->OnSecondaryAction.AddUObject(this, &UStorageChestWidget::OnStorageSecondaryAction);
		StorageSlots.Add(ItemSlot);
	}
}

void UStorageChestWidget::SetChest(AStorageChestActor* Chest)
{
	if (StorageChest.IsValid())
	{
		StorageChest->OnContentsChanged.RemoveAll(this);
		StorageChest->OnDestroyed.RemoveDynamic(this, &UStorageChestWidget::OnChestDestroyed);
	}
	StorageChest = Chest;
	if (StorageChest.IsValid())
	{
		StorageChest->OnContentsChanged.AddUObject(this, &UStorageChestWidget::RefreshSlots);
		StorageChest->OnDestroyed.AddUniqueDynamic(this, &UStorageChestWidget::OnChestDestroyed);
	}
	RefreshSlots();
}

void UStorageChestWidget::TransferStack(int32 Index)
{
	if (StorageChest.IsValid() == false) { return; }
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	const auto& Source = Inventory->GetItemSlots(EItemContainer::Storage, StorageChest.Get());
	if (Source.IsValidIndex(Index) == false || Source[Index].IsEmpty()) { return; }
	const FName Key = Source[Index].ItemUID;
	bool bMoved = false;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		const auto& Target = Inventory->GetItemSlots(EItemContainer::Inventory);
		for (int32 TargetIndex = 0; TargetIndex < Target.Num() && Source[Index].IsEmpty() == false; ++TargetIndex)
		{
			if ((Pass == 0 && Target[TargetIndex].IsEmpty() == false && Target[TargetIndex].ItemUID == Key) || (Pass == 1 && Target[TargetIndex].IsEmpty()))
			{
				bMoved |= Inventory->TryMoveItemSlot(Index, TargetIndex, EItemContainer::Storage, EItemContainer::Inventory, StorageChest.Get());
			}
		}
	}
	if (bMoved) { GetLocalPlayerCharacter()->TrySaveGame(); }
}

void UStorageChestWidget::RefreshSlots()
{
	for (int32 Index = 0; Index < StorageSlots.Num(); ++Index) { StorageSlots[Index]->SetSlotAddress(EItemContainer::Storage, Index, StorageChest.Get()); }
}

void UStorageChestWidget::UpdateInteractionRange()
{
	if (StorageChest.IsValid() == false || StorageChest->CanInteract(GetLocalPlayerCharacter()) == false) { CloseChest(); }
}

void UStorageChestWidget::OnStorageSecondaryAction(EItemContainer Container, int32 Index, FName Key) { TransferStack(Index); }
void UStorageChestWidget::OnChestDestroyed(AActor* Actor) { CloseChest(); }
