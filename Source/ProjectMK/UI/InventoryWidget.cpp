// LINK

#include "ProjectMK/UI/InventoryWidget.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/SizeBox.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/UI/ItemSlotWidget.h"
#include "ProjectMK/UI/ItemSlotDragDropOperation.h"

void UInventoryWidget::SetShowQuickSlots(bool bShow)
{
	bShowQuickSlots = bShow;
}

TSharedRef<SWidget> UInventoryWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->FindWidget(TEXT("RuntimeInventoryRoot")) == nullptr)
	{
		BuildDefaultWidgetTree();
	}
	return Super::RebuildWidget();
}

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateItemSlotPanel();
	RefreshInventory();
}

bool UInventoryWidget::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	return Cast<UItemSlotDragDropOperation>(Operation) != nullptr || Super::NativeOnDrop(Geometry, Event, Operation);
}

void UInventoryWidget::BindEvents()
{
	Super::BindEvents();

	if (UInventoryComponent* InventoryComponent = GetLocalInventoryComponent())
	{
		InventoryComponent->OnInventoryChangedDelegate.AddUObject(this, &UInventoryWidget::OnInventoryChanged);
	}

	if (::IsValid(OwnerASC))
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetInventorySlotCountAttribute()).AddUObject(this, &UInventoryWidget::OnInventorySlotCountChanged);
	}
	if (USkillComponent* Skills = GetLocalSkillComponent())
	{
		Skills->OnSkillPurchasedDelegate.AddUObject(this, &UInventoryWidget::OnSkillPurchased);
	}
}

void UInventoryWidget::UnbindEvents()
{
	Super::UnbindEvents();

	if (UInventoryComponent* InventoryComponent = GetLocalInventoryComponent())
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
	}

	if (::IsValid(OwnerASC))
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetInventorySlotCountAttribute()).RemoveAll(this);
	}
	if (USkillComponent* Skills = GetLocalSkillComponent())
	{
		Skills->OnSkillPurchasedDelegate.RemoveAll(this);
	}
}

void UInventoryWidget::BuildDefaultWidgetTree()
{
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RuntimeInventoryRoot"));
	ApplyBorderStyle(Frame, TEXT("InsetPanel"));
	Frame->SetPadding(FMargin(bShowQuickSlots ? 8.f : 12.f));
	WidgetTree->RootWidget = Frame;
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Column);
	if (bShowQuickSlots)
	{
		QuickSlotPanel = WidgetTree->ConstructWidget<UGridPanel>();
		Column->AddChildToVerticalBox(QuickSlotPanel)->SetHorizontalAlignment(HAlign_Center);
		Column->AddChildToVerticalBox(BuildSectionDivider());
	}
	ItemSlotPanel = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), TEXT("ItemSlotPanel"));
	Column->AddChildToVerticalBox(ItemSlotPanel)->SetHorizontalAlignment(HAlign_Center);
}

void UInventoryWidget::UpdateItemSlotPanel()
{
	if (::IsValid(ItemSlotPanel) == false) { return; }
	ItemSlotPanel->ClearChildren();
	ItemSlotList.Reset();
	for (int32 Index = 0; Index < GetDesiredSlotCount(); ++Index)
	{
		UItemSlotWidget* ItemWidget = CreateWidget<UItemSlotWidget>(this);
		UGridSlot* Layout = ItemSlotPanel->AddChildToGrid(ItemWidget, Index / InventoryColumnCount, Index % InventoryColumnCount);
		Layout->SetPadding(FMargin(4.f));
		Layout->SetHorizontalAlignment(HAlign_Center);
		Layout->SetVerticalAlignment(VAlign_Center);
		ItemWidget->SetSlotAddress(EItemContainer::Inventory, Index);
		ItemSlotList.Add(ItemWidget);
	}
	if (::IsValid(QuickSlotPanel))
	{
		QuickSlotPanel->ClearChildren();
		QuickSlotList.Reset();
		for (int32 Index = 0; Index < UInventoryComponent::QuickSlotCount; ++Index)
		{
			UItemSlotWidget* ItemWidget = CreateWidget<UItemSlotWidget>(this);
			UGridSlot* Layout = QuickSlotPanel->AddChildToGrid(ItemWidget, Index / 4, Index % 4);
			Layout->SetPadding(FMargin(4.f, 4.f, 4.f, 12.f));
			ItemWidget->SetSlotAddress(EItemContainer::QuickSlot, Index);
			QuickSlotList.Add(ItemWidget);
		}
	}
}

void UInventoryWidget::RefreshInventory()
{
	if (ItemSlotList.Num() != GetDesiredSlotCount()) { UpdateItemSlotPanel(); }
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	if (::IsValid(Inventory) == false) { return; }
	const TArray<FInventorySlotData>& Slots = Inventory->GetInventorySlotDataList();
	for (int32 Index = 0; Index < ItemSlotList.Num(); ++Index)
	{
		if (Slots.IsValidIndex(Index) && Slots[Index].IsEmpty() == false) { ItemSlotList[Index]->SetItem(Slots[Index].ItemUID, Slots[Index].ItemCount); }
		else { ItemSlotList[Index]->ClearItem(); }
	}
	const TArray<FInventorySlotData>& QuickSlots = Inventory->GetItemSlots(EItemContainer::QuickSlot);
	for (int32 Index = 0; Index < QuickSlotList.Num(); ++Index)
	{
		if (QuickSlots[Index].IsEmpty() == false) { QuickSlotList[Index]->SetItem(QuickSlots[Index].ItemUID, QuickSlots[Index].ItemCount); }
		else { QuickSlotList[Index]->ClearItem(); }
	}
}

int32 UInventoryWidget::GetDesiredSlotCount() const
{
	const UInventoryComponent* Inventory = GetLocalInventoryComponent();
	return ::IsValid(Inventory) ? Inventory->GetMaxInventoryCount() : 0;
}

void UInventoryWidget::OnInventoryChanged()
{
	RefreshInventory();
}

void UInventoryWidget::OnInventorySlotCountChanged(const FOnAttributeChangeData& Data)
{
	UpdateItemSlotPanel();
	RefreshInventory();
}

void UInventoryWidget::OnSkillPurchased(FName SkillKey)
{
	UpdateItemSlotPanel();
	RefreshInventory();
}
