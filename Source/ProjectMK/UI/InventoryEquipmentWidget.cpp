// LINK

#include "ProjectMK/UI/InventoryEquipmentWidget.h"

#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/UI/ItemSlotWidget.h"

void UInventoryEquipmentWidget::NativeConstruct()
{
	Super::NativeConstruct();

	OnInventoryChanged();
}

void UInventoryEquipmentWidget::BindEvents()
{
	Super::BindEvents();

	if (::IsValid(LocalPlayerCharacter) == false)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryChangedDelegate.AddUObject(this, &UInventoryEquipmentWidget::OnInventoryChanged);
	}
}

void UInventoryEquipmentWidget::UnbindEvents()
{
	Super::UnbindEvents();

	if (::IsValid(LocalPlayerCharacter) == false)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
	}
}

void UInventoryEquipmentWidget::OnInventoryChanged()
{
	if (::IsValid(HelmetSlot))
	{
		HelmetSlot->ClearItem();
	}

	if (::IsValid(ArmorSlot))
	{
		ArmorSlot->ClearItem();
	}

	if (::IsValid(GlovesSlot))
	{
		GlovesSlot->ClearItem();
	}

	if (::IsValid(ShoesSlot))
	{
		ShoesSlot->ClearItem();
	}

	if (::IsValid(DrillSlot))
	{
		DrillSlot->ClearItem();
	}

	if (::IsValid(BalloonSlot))
	{
		BalloonSlot->ClearItem();
	}

	RefreshEquipmentSlot(EEuipmentType::Halmet, HelmetSlot);
	RefreshEquipmentSlot(EEuipmentType::Armor, ArmorSlot);
	RefreshEquipmentSlot(EEuipmentType::Gloves, GlovesSlot);
	RefreshEquipmentSlot(EEuipmentType::Shoes, ShoesSlot);
	RefreshEquipmentSlot(EEuipmentType::Drill, DrillSlot);
	RefreshEquipmentSlot(EEuipmentType::Balloon, BalloonSlot);
}

void UInventoryEquipmentWidget::RefreshEquipmentSlot(EEuipmentType EquipmentType, UItemSlotWidget* PreferredSlot, UItemSlotWidget* FallbackSlot)
{
	UItemSlotWidget* TargetSlot = ::IsValid(PreferredSlot) ? PreferredSlot : FallbackSlot;
	if (::IsValid(TargetSlot) == false || ::IsValid(LocalPlayerCharacter) == false)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = LocalPlayerCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent) == false)
	{
		return;
	}

	const FName EquippedItemKey = InventoryComponent->GetEquippedItem(EquipmentType);
	if (EquippedItemKey.IsNone())
	{
		return;
	}

	TargetSlot->SetItem(EquippedItemKey, InventoryComponent->GetItemCount(EquippedItemKey));
}
