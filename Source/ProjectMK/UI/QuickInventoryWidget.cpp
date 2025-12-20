// LINK

#include "ProjectMK/UI/QuickInventoryWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "ItemSlotWidget.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "GameFramework/Character.h"

#define QUICK_INVENTORY_SLOT_COUNT 8

void UQuickInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetInventorySlot();
}

void UQuickInventoryWidget::BindEvents()
{
	Super::BindEvents();

	ACharacter* LocalCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (::IsValid(LocalCharacter))
	{
		UInventoryComponent* InventoryComponent = LocalCharacter->GetComponentByClass<UInventoryComponent>();
		if (::IsValid(InventoryComponent))
		{
			InventoryComponent->OnInventoryChangedDelegate.AddUObject(this, &UQuickInventoryWidget::OnInventoryChanged);
		}
	}
}

void UQuickInventoryWidget::SetInventorySlot()
{
	if (::IsValid(HBox_Slot) == false)
	{
		return;
	}

	for (int32 i = 0; i < QUICK_INVENTORY_SLOT_COUNT; i++)
	{
		UItemSlotWidget* NewItemSlot = CreateWidget<UItemSlotWidget>(this, ItemSlotClass);
		if (::IsValid(NewItemSlot))
		{
			HBox_Slot->AddChildToHorizontalBox(NewItemSlot);
			ItemSlotList.Add(NewItemSlot);
		}
	}
}

void UQuickInventoryWidget::OnInventoryChanged()
{
	ACharacter* LocalCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (::IsValid(LocalCharacter) == false)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = LocalCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent) == false)
	{
		return;
	}

	const TMap<FName, int32> InventoryItems = InventoryComponent->GetInventoryItems();

	int32 SlotIndex = 0;
	for (const auto& InventoryItem : InventoryItems)
	{
		if (ItemSlotList.Num() > SlotIndex)
		{
			ItemSlotList[SlotIndex]->SetItem(InventoryItem.Key, InventoryItem.Value);
		}
		++SlotIndex;
	}
}
