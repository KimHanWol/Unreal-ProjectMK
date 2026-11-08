// LINK

#include "ProjectMK/UI/QuickInventoryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/UI/PlaytestQuickSlotWidget.h"
#include "ProjectMK/UI/ItemSlotDragDropOperation.h"

UQuickInventoryWidget::UQuickInventoryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	QuickSlotClass = UPlaytestQuickSlotWidget::StaticClass();
}

TSharedRef<SWidget> UQuickInventoryWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultWidgetTree();
	}
	return Super::RebuildWidget();
}

void UQuickInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	GetWorld()->GetTimerManager().SetTimer(RecallTimerHandle, this, &UQuickInventoryWidget::UpdateRecall, 0.25f, true);
}

void UQuickInventoryWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecallTimerHandle);
	}
	Super::NativeDestruct();
}

bool UQuickInventoryWidget::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	return Cast<UItemSlotDragDropOperation>(Operation) != nullptr || Super::NativeOnDrop(Geometry, Event, Operation);
}

void UQuickInventoryWidget::BindEvents()
{
	Super::BindEvents();
	if (UInventoryComponent* Inventory = GetLocalInventoryComponent())
	{
		Inventory->OnInventoryChangedDelegate.RemoveAll(this);
		Inventory->OnInventoryChangedDelegate.AddUObject(this, &UQuickInventoryWidget::OnInventoryChanged);
	}
}

void UQuickInventoryWidget::UnbindEvents()
{
	Super::UnbindEvents();
	if (UInventoryComponent* Inventory = GetLocalInventoryComponent())
	{
		Inventory->OnInventoryChangedDelegate.RemoveAll(this);
	}
}

void UQuickInventoryWidget::UpdateWidget()
{
	UpdateQuickInventorySlots();
	UpdateRecall();
}

void UQuickInventoryWidget::BuildDefaultWidgetTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Quickbar"));
	Frame->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.055f, 0.95f));
	Frame->SetPadding(FMargin(32.f, FMKUILayout::SpaceM, 32.f, 20.f));
	UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame);
	FrameSlot->SetAnchors(FAnchors(ViewportAnchor.X, ViewportAnchor.Y));
	FrameSlot->SetAlignment(ViewportAnchor);
	FrameSlot->SetPosition(ViewportOffset);
	FrameSlot->SetAutoSize(true);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Column);
	HBox_Slot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HBox_Slot"));
	Column->AddChildToVerticalBox(HBox_Slot);
	RecallText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RecallText"));
	ApplyTextRole(RecallText, EMKTextRole::Caption, true);
	RecallText->SetJustification(ETextJustify::Center);
	RecallText->SetVisibility(ESlateVisibility::Collapsed);
	Column->AddChildToVerticalBox(RecallText)->SetPadding(FMargin(0.f, FMKUILayout::SpaceS, 0.f, 0.f));
}

void UQuickInventoryWidget::UpdateQuickInventorySlots()
{
	if (::IsValid(HBox_Slot) == false || ::IsValid(QuickSlotClass) == false)
	{
		return;
	}
	if (ItemSlotList.IsEmpty())
	{
		for (int32 SlotIndex = 0; SlotIndex < QuickInventorySlotCount; ++SlotIndex)
		{
			UPlaytestQuickSlotWidget* QuickSlot = CreateWidget<UPlaytestQuickSlotWidget>(this, QuickSlotClass);
			if (::IsValid(QuickSlot))
			{
				HBox_Slot->AddChildToHorizontalBox(QuickSlot)->SetPadding(FMargin(FMKUILayout::SpaceXS, 0.f));
				QuickSlot->SetSlotIndex(SlotIndex);
				ItemSlotList.Add(QuickSlot);
			}
		}
	}
	for (UPlaytestQuickSlotWidget* QuickSlot : ItemSlotList)
	{
		QuickSlot->RefreshSlot();
	}
}

void UQuickInventoryWidget::UpdateRecall()
{
	if (::IsValid(RecallText) == false)
	{
		return;
	}
	const AMKCharacter* Character = GetLocalPlayerCharacter();
	const UConsumableComponent* Consumables = ::IsValid(Character) ? Character->GetConsumableComponent() : nullptr;
	const bool bIsRecalling = ::IsValid(Consumables) && Consumables->IsRecalling();
	RecallText->SetText(bIsRecalling
		? FText::Format(NSLOCTEXT("QuickInventory", "Recall", "귀환까지 {0}초"), FMath::CeilToInt(Consumables->GetRecallRemainingTime()))
		: FText::GetEmpty());
	RecallText->SetVisibility(bIsRecalling ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UQuickInventoryWidget::OnInventoryChanged()
{
	UpdateWidget();
}
