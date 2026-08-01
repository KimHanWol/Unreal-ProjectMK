// LINK

#include "ProjectMK/UI/InventoryWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/UI/ItemSlotWidget.h"

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateItemSlotPanel();
	RefreshInventory();
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
}

void UInventoryWidget::UpdateItemSlotPanel()
{
	if (::IsValid(ItemSlotPanel) == false)
	{
		ensureMsgf(ItemSlotPanel != nullptr, TEXT("InventoryWidget requires a GridPanel named 'ItemSlotPanel' in the widget blueprint."));
		return;
	}

	ItemSlotPanel->ClearChildren();
	ItemSlotList.Reset();

	if (::IsValid(ItemSlotClass) == false)
	{
		return;
	}

	const int32 DesiredSlotCount = GetDesiredSlotCount();
	const int32 ColumnCount = FMath::Max(1, InventoryColumnCount);
	const int32 RowCount = FMath::Max(1, FMath::CeilToInt(static_cast<float>(DesiredSlotCount) / static_cast<float>(ColumnCount)));

	for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
	{
		ItemSlotPanel->SetColumnFill(ColumnIndex, 1.f);
	}

	for (int32 RowIndex = 0; RowIndex < RowCount; ++RowIndex)
	{
		ItemSlotPanel->SetRowFill(RowIndex, 1.f);
	}

	for (int32 SlotIndex = 0; SlotIndex < DesiredSlotCount; ++SlotIndex)
	{
		UItemSlotWidget* NewItemSlot = CreateWidget<UItemSlotWidget>(this, ItemSlotClass);
		if (::IsValid(NewItemSlot) == false)
		{
			continue;
		}

		const int32 Row = SlotIndex / ColumnCount;
		const int32 Column = SlotIndex % ColumnCount;

		if (UGridSlot* ItemGridSlot = ItemSlotPanel->AddChildToGrid(NewItemSlot, Row, Column))
		{
			ItemGridSlot->SetPadding(FMargin(0.f));
			ItemGridSlot->SetHorizontalAlignment(HAlign_Fill);
			ItemGridSlot->SetVerticalAlignment(VAlign_Fill);
		}

		NewItemSlot->SetSlotIndex(SlotIndex);
		ItemSlotList.Add(NewItemSlot);
	}
}

void UInventoryWidget::RefreshInventory()
{
	if (ItemSlotList.Num() != GetDesiredSlotCount())
	{
		UpdateItemSlotPanel();
	}

	for (UItemSlotWidget* ItemSlot : ItemSlotList)
	{
		if (::IsValid(ItemSlot))
		{
			ItemSlot->ClearItem();
		}
	}

	UInventoryComponent* InventoryComponent = GetLocalInventoryComponent();
	if (::IsValid(InventoryComponent) == false)
	{
		return;
	}

	const TArray<FInventorySlotData>& InventorySlotDataList = InventoryComponent->GetInventorySlotDataList();

	for (int32 SlotIndex = 0; SlotIndex < InventorySlotDataList.Num(); ++SlotIndex)
	{
		if (ItemSlotList.IsValidIndex(SlotIndex) == false)
		{
			break;
		}

		const FInventorySlotData& InventorySlotData = InventorySlotDataList[SlotIndex];
		if (InventorySlotData.IsEmpty())
		{
			continue;
		}

		ItemSlotList[SlotIndex]->SetItem(InventorySlotData.ItemUID, InventorySlotData.ItemCount);
	}
}

int32 UInventoryWidget::GetDesiredSlotCount() const
{
	const UInventoryComponent* InventoryComponent = GetLocalInventoryComponent();
	if (::IsValid(InventoryComponent) == false)
	{
		return 0;
	}

	return InventoryComponent->GetMaxInventoryCount();
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
