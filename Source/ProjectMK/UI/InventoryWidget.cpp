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
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"
#include "Brushes/SlateNoResource.h"

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
	Frame->SetBrush(FSlateNoResource());
	Frame->SetPadding(FMargin(0.f));
	WidgetTree->RootWidget = Frame;
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Column);
	if (bShowQuickSlots)
	{
		UBorder* QuickFrame = WidgetTree->ConstructWidget<UBorder>();
		ApplyBorderStyle(QuickFrame, TEXT("InsetPanel"));
		QuickFrame->SetPadding(FMargin(12.f, 12.f, 12.f, 8.f));
		Column->AddChildToVerticalBox(QuickFrame)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
		QuickSlotPanel = WidgetTree->ConstructWidget<UGridPanel>();
		QuickFrame->SetContent(QuickSlotPanel);
	}
	UBorder* BagFrame = WidgetTree->ConstructWidget<UBorder>();
	ApplyBorderStyle(BagFrame, TEXT("InsetPanel"));
	BagFrame->SetPadding(FMargin(12.f));
	Column->AddChildToVerticalBox(BagFrame);
	ItemSlotPanel = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), TEXT("ItemSlotPanel"));
	BagFrame->SetContent(ItemSlotPanel);
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
		ItemWidget->OnSecondaryAction.AddUObject(this, &UInventoryWidget::OnSlotSecondaryAction);
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
			ItemWidget->OnSecondaryAction.AddUObject(this, &UInventoryWidget::OnSlotSecondaryAction);
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
		ItemSlotList[Index]->SetSlotLocked(Index >= Inventory->GetMaxInventoryCount());
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
	if (::IsValid(Inventory) == false) { return 0; }
	int32 MaximumSlots = Inventory->GetMaxInventoryCount();
	if (bShowQuickSlots == false) { return MaximumSlots; }
	const UDataManager* Data = UDataManager::Get(const_cast<UInventoryWidget*>(this));
	const USkillComponent* Skills = GetLocalSkillComponent();
	if (::IsValid(Data) && ::IsValid(Skills))
	{
		for (const FName Key : Data->GetSkillRowNames())
		{
			const FSkillDataTableRow* Skill = Data->GetSkillDataTableRow(Key);
			if (Skill == nullptr || Skill->AcquisitionType != ESkillAcquisitionType::SkillTree || Skills->HasPurchasedSkill(Key)) { continue; }
			for (const FSkillGameplayEffectData& Effect : Skill->GameplayEffectList)
			{
				const UClass* EffectClass = Effect.GameplayEffect.LoadSynchronous();
				const UGameplayEffect* Definition = ::IsValid(EffectClass) ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
				if (::IsValid(Definition) == false) { continue; }
				for (const FGameplayModifierInfo& Modifier : Definition->Modifiers)
				{
					if (Modifier.Attribute != UAttributeSet_Character::GetInventorySlotCountAttribute()
						|| Modifier.ModifierMagnitude.GetMagnitudeCalculationType() != EGameplayEffectMagnitudeCalculation::SetByCaller) { continue; }
					for (const FSkillSetByCallerData& Value : Effect.SetByCallerDataList)
					{
						if (Value.SetByCallerTag == Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag) { MaximumSlots += FMath::Max(0, FMath::RoundToInt(Value.SetByCallerValue)); }
					}
				}
			}
		}
	}
	return MaximumSlots;
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

void UInventoryWidget::OnSlotSecondaryAction(EItemContainer Container, int32 Index, FName ItemKey)
{
	OnItemSecondaryAction.Broadcast(Container, Index, ItemKey);
}
