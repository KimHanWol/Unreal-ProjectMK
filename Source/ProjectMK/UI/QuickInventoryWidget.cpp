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
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
	Frame->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.055f, 0.95f));
	Frame->SetPadding(FMargin(12.f, 8.f));
	UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame);
	FrameSlot->SetAnchors(FAnchors(ViewportAnchor.X, ViewportAnchor.Y));
	FrameSlot->SetAlignment(ViewportAnchor);
	FrameSlot->SetPosition(ViewportOffset);
	FrameSlot->SetAutoSize(true);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Column);
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
	Title->SetText(NSLOCTEXT("QuickInventory", "Title", "빠른 사용   1 — 8"));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.8f, 0.9f)));
	FSlateFontInfo Font = Title->GetFont();
	Font.Size = 12;
	Title->SetFont(Font);
	Column->AddChildToVerticalBox(Title)->SetPadding(FMargin(4.f, 0.f, 0.f, 6.f));
	HBox_Slot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HBox_Slot"));
	Column->AddChildToVerticalBox(HBox_Slot);
	RecallText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RecallText"));
	RecallText->SetFont(Font);
	RecallText->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(RecallText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
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
				HBox_Slot->AddChildToHorizontalBox(QuickSlot)->SetPadding(FMargin(3.f, 0.f));
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
	RecallText->SetText(::IsValid(Consumables) && Consumables->IsRecalling()
		? FText::Format(NSLOCTEXT("QuickInventory", "Recall", "귀환까지 {0}초 · 움직이면 취소"), FMath::CeilToInt(Consumables->GetRecallRemainingTime()))
		: NSLOCTEXT("QuickInventory", "Hint", "인벤토리 첫 8칸 · 숫자 키 / 클릭 사용 · 보험은 소지 시 자동 적용"));
}

void UQuickInventoryWidget::OnInventoryChanged()
{
	UpdateWidget();
}
