#include "ProjectMK/UI/PlaytestQuickSlotWidget.h"

#include "PaperSprite.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/UI/MKTooltipBase.h"

void UPlaytestQuickSlotWidget::UpdateWidget()
{
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	const UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(Inventory) == false || ::IsValid(DataManager) == false || SlotIndex == INDEX_NONE)
	{
		return;
	}
	const TArray<FInventorySlotData>& Slots = Inventory->GetInventorySlotDataList();
	const FInventorySlotData SlotData = Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex] : FInventorySlotData();
	const FItemDataTableRow* Item = SlotData.IsEmpty() == false
		? DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, SlotData.ItemUID) : nullptr;
	bCanUseItem = Item != nullptr && Item->ConsumableEffect != EConsumableEffectType::None
		&& Item->ConsumableEffect != EConsumableEffectType::Insurance;
	ItemDataKey = SlotData.ItemUID;
	SetKeyGuide(bShowKeyGuide ? FText::AsNumber(SlotIndex + 1) : FText::GetEmpty());
	SetSlotCount(SlotData.ItemCount, SlotData.IsEmpty() == false && SlotData.ItemCount > 1);
	SetSelected(bCanUseItem);
	UPaperSprite* Sprite = Item != nullptr ? Item->ItemIcon.LoadSynchronous() : nullptr;
	SetSlotIcon(Sprite);
}

bool UPlaytestQuickSlotWidget::CanClickSlot() const
{
	return bCanUseItem;
}

void UPlaytestQuickSlotWidget::OnSlotClicked()
{
	AMKCharacter* Character = GetLocalPlayerCharacter();
	UConsumableComponent* Consumables = ::IsValid(Character) ? Character->GetConsumableComponent() : nullptr;
	if (::IsValid(Consumables))
	{
		Consumables->TryUseInventorySlot(SlotIndex);
	}
}

void UPlaytestQuickSlotWidget::UpdateTooltipWidget(UMKTooltipBase* TooltipWidget) const
{
	if (::IsValid(TooltipWidget) && ItemDataKey.IsNone() == false)
	{
		TooltipWidget->ShowItemTooltip(ItemDataKey);
	}
}

void UPlaytestQuickSlotWidget::SetSlotIndex(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
	UpdateWidget();
}

void UPlaytestQuickSlotWidget::SetShowKeyGuide(bool bInShowKeyGuide)
{
	bShowKeyGuide = bInShowKeyGuide;
	SetKeyGuide(bShowKeyGuide && SlotIndex != INDEX_NONE ? FText::AsNumber(SlotIndex + 1) : FText::GetEmpty());
}

void UPlaytestQuickSlotWidget::RefreshSlot()
{
	UpdateWidget();
}
