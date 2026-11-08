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
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/UI/ItemSlotWidget.h"

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
	Frame->SetPadding(FMargin(12.f));
	WidgetTree->RootWidget = Frame;
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Column);
	Column->AddChildToVerticalBox(BuildSectionHeader(NSLOCTEXT("Inventory", "Title", "소지품"), TEXT("Crafting")))
		->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	ItemSlotPanel = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), TEXT("ItemSlotPanel"));
	Column->AddChildToVerticalBox(ItemSlotPanel)->SetHorizontalAlignment(HAlign_Center);
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

	const int32 DesiredSlotCount = GetDesiredSlotCount();
	const int32 ColumnCount = FMath::Max(1, InventoryColumnCount);
	const int32 RowCount = FMath::Max(1, FMath::CeilToInt(static_cast<float>(DesiredSlotCount) / static_cast<float>(ColumnCount)));

	for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
	{
		ItemSlotPanel->SetColumnFill(ColumnIndex, 0.f);
	}

	for (int32 RowIndex = 0; RowIndex < RowCount; ++RowIndex)
	{
		ItemSlotPanel->SetRowFill(RowIndex, 0.f);
	}

	for (int32 SlotIndex = 0; SlotIndex < DesiredSlotCount; ++SlotIndex)
	{
		UItemSlotWidget* NewItemSlot = CreateWidget<UItemSlotWidget>(this, UItemSlotWidget::StaticClass());
		if (::IsValid(NewItemSlot) == false)
		{
			continue;
		}

		const int32 Row = SlotIndex / ColumnCount;
		const int32 Column = SlotIndex % ColumnCount;

		if (UGridSlot* ItemGridSlot = ItemSlotPanel->AddChildToGrid(NewItemSlot, Row, Column))
		{
			ItemGridSlot->SetPadding(FMargin(4.f));
			ItemGridSlot->SetHorizontalAlignment(HAlign_Center);
			ItemGridSlot->SetVerticalAlignment(VAlign_Center);
		}

		NewItemSlot->SetSlotIndex(SlotIndex);
		const UInventoryComponent* Inventory = GetLocalInventoryComponent();
		NewItemSlot->SetSlotLocked(::IsValid(Inventory) && SlotIndex >= Inventory->GetMaxInventoryCount());
		NewItemSlot->SetKeyGuide(SlotIndex < 8 ? FText::AsNumber(SlotIndex + 1) : FText::GetEmpty());
		if (Row >= 2)
		{
			CastChecked<UGridSlot>(NewItemSlot->Slot)->SetPadding(FMargin(4.f, Row == 2 ? 16.f : 4.f, 4.f, 4.f));
		}
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

	int32 MaximumSlots = InventoryComponent->GetMaxInventoryCount();
	const UDataManager* DataManager = UDataManager::Get(const_cast<UInventoryWidget*>(this));
	const USkillComponent* Skills = GetLocalSkillComponent();
	if (::IsValid(DataManager) && ::IsValid(Skills))
	{
		for (const FName SkillKey : DataManager->GetSkillRowNames())
		{
			if (Skills->HasPurchasedSkill(SkillKey)) { continue; }
			const FSkillDataTableRow* Skill = DataManager->GetSkillDataTableRow(SkillKey);
			if (Skill == nullptr) { continue; }
			for (const FSkillGameplayEffectData& Effect : Skill->GameplayEffectList)
			{
				const UClass* EffectClass = Effect.GameplayEffect.LoadSynchronous();
				const UGameplayEffect* Definition = ::IsValid(EffectClass) ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
				if (::IsValid(Definition) == false) { continue; }
				for (const FGameplayModifierInfo& Modifier : Definition->Modifiers)
				{
					if (Modifier.Attribute != UAttributeSet_Character::GetInventorySlotCountAttribute()
						|| Modifier.ModifierMagnitude.GetMagnitudeCalculationType() != EGameplayEffectMagnitudeCalculation::SetByCaller) { continue; }
					const FGameplayTag Tag = Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag;
					for (const FSkillSetByCallerData& Value : Effect.SetByCallerDataList)
					{
						if (Value.SetByCallerTag == Tag) { MaximumSlots += FMath::Max(0, FMath::RoundToInt(Value.SetByCallerValue)); }
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
