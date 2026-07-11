// LINK

#include "ProjectMK/UI/InventoryWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/GridPanel.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/UI/ItemSlotWidget.h"
#include "UObject/ConstructorHelpers.h"

UInventoryWidget::UInventoryWidget()
{
	static ConstructorHelpers::FClassFinder<UItemSlotWidget> ItemSlotWidgetClass(TEXT("/Game/UI/Inventory/W_ItemSlot"));
	if (ItemSlotWidgetClass.Succeeded())
	{
		ItemSlotClass = ItemSlotWidgetClass.Class;
	}
}

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildItemSlotPanel();
	RefreshInventory();
}

void UInventoryWidget::BindEvents()
{
	Super::BindEvents();

	if (::IsValid(LocalPlayerCharacter))
	{
		if (UInventoryComponent* InventoryComponent = LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>())
		{
			InventoryComponent->OnInventoryChangedDelegate.AddUObject(this, &UInventoryWidget::OnInventoryChanged);
		}
	}

	if (::IsValid(OwnerASC))
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetInventorySlotCountAttribute()).AddUObject(this, &UInventoryWidget::OnInventorySlotCountChanged);
	}
}

void UInventoryWidget::UnbindEvents()
{
	Super::UnbindEvents();

	if (::IsValid(LocalPlayerCharacter))
	{
		if (UInventoryComponent* InventoryComponent = LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>())
		{
			InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
		}
	}

	if (::IsValid(OwnerASC))
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetInventorySlotCountAttribute()).RemoveAll(this);
	}
}

void UInventoryWidget::RebuildItemSlotPanel()
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

	for (int32 SlotIndex = 0; SlotIndex < DesiredSlotCount; ++SlotIndex)
	{
		UItemSlotWidget* NewItemSlot = CreateWidget<UItemSlotWidget>(this, ItemSlotClass);
		if (::IsValid(NewItemSlot) == false)
		{
			continue;
		}

		const int32 Row = SlotIndex / ColumnCount;
		const int32 Column = SlotIndex % ColumnCount;

		ItemSlotPanel->AddChildToGrid(NewItemSlot, Row, Column);
		ItemSlotList.Add(NewItemSlot);
	}
}

void UInventoryWidget::RefreshInventory()
{
	if (ItemSlotList.Num() != GetDesiredSlotCount())
	{
		RebuildItemSlotPanel();
	}

	for (UItemSlotWidget* ItemSlot : ItemSlotList)
	{
		if (::IsValid(ItemSlot))
		{
			ItemSlot->ClearItem();
		}
	}

	if (::IsValid(LocalPlayerCharacter) == false)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent) == false)
	{
		return;
	}

	const TMap<FName, int32> InventoryItems = InventoryComponent->GetInventoryItems();
	const TArray<FName>& InventoryItemOrder = InventoryComponent->GetInventoryItemOrder();

	for (int32 SlotIndex = 0; SlotIndex < InventoryItemOrder.Num(); ++SlotIndex)
	{
		if (ItemSlotList.IsValidIndex(SlotIndex) == false)
		{
			break;
		}

		const FName& ItemKey = InventoryItemOrder[SlotIndex];
		const int32* ItemCountPtr = InventoryItems.Find(ItemKey);
		if (ItemCountPtr == nullptr)
		{
			continue;
		}

		ItemSlotList[SlotIndex]->SetItem(ItemKey, *ItemCountPtr);
	}
}

int32 UInventoryWidget::GetDesiredSlotCount() const
{
	if (::IsValid(LocalPlayerCharacter) == false)
	{
		return 0;
	}

	const UInventoryComponent* InventoryComponent = LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>();
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
	RebuildItemSlotPanel();
	RefreshInventory();
}
