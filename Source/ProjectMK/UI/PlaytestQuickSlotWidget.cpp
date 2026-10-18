#include "ProjectMK/UI/PlaytestQuickSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "PaperSprite.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/UI/MKMockupButton.h"

TSharedRef<SWidget> UPlaytestQuickSlotWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultWidgetTree();
	}
	return Super::RebuildWidget();
}

void UPlaytestQuickSlotWidget::BindEvents()
{
	if (::IsValid(UseButton))
	{
		UseButton->OnClicked.AddUniqueDynamic(this, &UPlaytestQuickSlotWidget::OnUseButtonClicked);
	}
}

void UPlaytestQuickSlotWidget::UnbindEvents()
{
	if (::IsValid(UseButton))
	{
		UseButton->OnClicked.RemoveDynamic(this, &UPlaytestQuickSlotWidget::OnUseButtonClicked);
	}
}

void UPlaytestQuickSlotWidget::UpdateWidget()
{
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	const UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(Inventory) == false || ::IsValid(DataManager) == false || SlotIndex == INDEX_NONE
		|| ::IsValid(UseButton) == false || ::IsValid(FrameBorder) == false || ::IsValid(KeyText) == false
		|| ::IsValid(CountText) == false || ::IsValid(ItemNameText) == false || ::IsValid(ItemIcon) == false)
	{
		return;
	}
	const TArray<FInventorySlotData>& Slots = Inventory->GetInventorySlotDataList();
	const FInventorySlotData SlotData = Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex] : FInventorySlotData();
	const FItemDataTableRow* Item = SlotData.IsEmpty() == false
		? DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, SlotData.ItemUID) : nullptr;
	const bool bCanUse = Item != nullptr && Item->ConsumableEffect != EConsumableEffectType::None
		&& Item->ConsumableEffect != EConsumableEffectType::Insurance;
	FrameBorder->SetBrushColor(bCanUse ? FLinearColor(0.04f, 0.18f, 0.23f) : FLinearColor(0.055f, 0.07f, 0.09f));
	KeyText->SetText(FText::AsNumber(SlotIndex + 1));
	CountText->SetText(SlotData.IsEmpty() ? FText::GetEmpty() : FText::AsNumber(SlotData.ItemCount));
	FText ItemName = SlotData.IsEmpty() ? NSLOCTEXT("QuickInventory", "Empty", "빈 슬롯") : FText::FromName(SlotData.ItemUID);
	if (SlotData.IsEmpty() == false)
	{
		for (const FName& RecipeKey : DataManager->GetShopRecipeRowNames())
		{
			const FShopRecipeDataTableRow* Recipe = DataManager->GetShopRecipeDataTableRow(RecipeKey);
			if (Recipe != nullptr && Recipe->GetResultItemKey() == SlotData.ItemUID && Recipe->DisplayName.IsEmpty() == false)
			{
				ItemName = Recipe->DisplayName;
				break;
			}
		}
	}
	ItemNameText->SetText(ItemName);
	UseButton->SetToolTipText(SlotData.IsEmpty()
		? NSLOCTEXT("Playtest", "Empty", "빈 인벤토리 슬롯")
		: FText::Format(NSLOCTEXT("QuickInventory", "ItemHint", "{0}\n숫자 키 / 클릭으로 사용 · 보험은 소지 시 자동 적용"), ItemName));
	UPaperSprite* Sprite = Item != nullptr ? Item->ItemIcon.LoadSynchronous() : nullptr;
	ItemIcon->SetBrushResourceObject(Sprite);
	ItemIcon->SetVisibility(::IsValid(Sprite) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void UPlaytestQuickSlotWidget::SetSlotIndex(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
	UpdateWidget();
}

void UPlaytestQuickSlotWidget::RefreshSlot()
{
	UpdateWidget();
}

void UPlaytestQuickSlotWidget::BuildDefaultWidgetTree()
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(SlotSize.X);
	SizeBox->SetHeightOverride(SlotSize.Y);
	WidgetTree->RootWidget = SizeBox;
	UseButton = WidgetTree->ConstructWidget<UMKMockupButton>();
	UseButton->SetBackgroundColor(FLinearColor(0.2f, 0.35f, 0.4f));
	SizeBox->SetContent(UseButton);
	FrameBorder = WidgetTree->ConstructWidget<UBorder>();
	FrameBorder->SetPadding(FMargin(4.f));
	UseButton->SetContent(FrameBorder);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	FrameBorder->SetContent(Column);
	UOverlay* IconPanel = WidgetTree->ConstructWidget<UOverlay>();
	Column->AddChildToVerticalBox(IconPanel)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ItemIcon = WidgetTree->ConstructWidget<UImage>();
	ItemIcon->SetDesiredSizeOverride(FVector2D(32.f));
	UOverlaySlot* ImageSlot = IconPanel->AddChildToOverlay(ItemIcon);
	ImageSlot->SetHorizontalAlignment(HAlign_Center);
	ImageSlot->SetVerticalAlignment(VAlign_Center);
	KeyText = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo Font = KeyText->GetFont();
	Font.Size = 10;
	KeyText->SetFont(Font);
	KeyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.8f, 0.45f)));
	UOverlaySlot* KeySlot = IconPanel->AddChildToOverlay(KeyText);
	KeySlot->SetHorizontalAlignment(HAlign_Left);
	KeySlot->SetVerticalAlignment(VAlign_Top);
	CountText = WidgetTree->ConstructWidget<UTextBlock>();
	CountText->SetFont(Font);
	UOverlaySlot* CountSlot = IconPanel->AddChildToOverlay(CountText);
	CountSlot->SetHorizontalAlignment(HAlign_Right);
	CountSlot->SetVerticalAlignment(VAlign_Bottom);
	ItemNameText = WidgetTree->ConstructWidget<UTextBlock>();
	ItemNameText->SetFont(Font);
	ItemNameText->SetJustification(ETextJustify::Center);
	ItemNameText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	Column->AddChildToVerticalBox(ItemNameText)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
}

void UPlaytestQuickSlotWidget::OnUseButtonClicked()
{
	AMKCharacter* Character = GetLocalPlayerCharacter();
	UConsumableComponent* Consumables = ::IsValid(Character) ? Character->GetConsumableComponent() : nullptr;
	if (::IsValid(Consumables))
	{
		Consumables->TryUseInventorySlot(SlotIndex);
	}
}
