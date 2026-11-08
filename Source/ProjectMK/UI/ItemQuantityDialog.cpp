#include "ProjectMK/UI/ItemQuantityDialog.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "Components/Image.h"
#include "PaperSprite.h"
#include "ProjectMK/UI/MKMockupButton.h"

TSharedRef<SWidget> UItemQuantityDialog::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		SetIsFocusable(true);
		UBorder* Dim = BuildModalBackdrop();
		WidgetTree->RootWidget = Dim;
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(440.f);
		Dim->SetContent(Size);
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		Panel->SetPadding(FMargin(32.f));
		Size->SetContent(Panel);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(Column);
		UTextBlock* HeaderText = nullptr;
		Column->AddChildToVerticalBox(BuildSectionHeader(NSLOCTEXT("Sale", "Quantity", "판매 수량"), NAME_None, &HeaderText))->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
		ActionTitle = HeaderText;
		UHorizontalBox* Summary = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Summary)->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
		UImage* Icon = nullptr;
		Summary->AddChildToHorizontalBox(BuildItemIconFrame(Icon, 40.f))->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
		ItemIcon = Icon;
		UVerticalBox* ItemCopy = WidgetTree->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* CopyLayout = Summary->AddChildToHorizontalBox(ItemCopy);
		CopyLayout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CopyLayout->SetVerticalAlignment(VAlign_Center);
		ItemTitle = WidgetTree->ConstructWidget<UTextBlock>();
		ApplyTextRole(ItemTitle, EMKTextRole::Label);
		ItemCopy->AddChildToVerticalBox(ItemTitle);
		OwnedText = WidgetTree->ConstructWidget<UTextBlock>();
		ApplyTextRole(OwnedText, EMKTextRole::Body);
		ItemCopy->AddChildToVerticalBox(OwnedText)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
		UHorizontalBox* AmountRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(AmountRow)->SetHorizontalAlignment(HAlign_Center);
		UButton* Button = nullptr;
		UTextBlock* Label = nullptr;
		AmountRow->AddChildToHorizontalBox(BuildActionButton(Button, Label, FText::FromString(TEXT("−")), 44.f, 44.f, TEXT("Stepper")))->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		DecreaseButton = Button;
		DecreaseButton->Rename(TEXT("QuantityDecrease"));
		QuantityInput = WidgetTree->ConstructWidget<USpinBox>();
		QuantityInput->SetEnableSlider(false);
		QuantityInput->SetJustification(ETextJustify::Center);
		QuantityInput->SetMinValue(1.f);
		QuantityInput->SetMinSliderValue(1.f);
		QuantityInput->SetDelta(1.f);
		QuantityInput->SetMinFractionalDigits(0);
		QuantityInput->SetMaxFractionalDigits(0);
		USizeBox* InputSize = WidgetTree->ConstructWidget<USizeBox>();
		InputSize->SetHeightOverride(FMKUILayout::ButtonHeight);
		InputSize->SetWidthOverride(128.f);
		InputSize->SetContent(QuantityInput);
		AmountRow->AddChildToHorizontalBox(InputSize)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		AmountRow->AddChildToHorizontalBox(BuildActionButton(Button, Label, FText::FromString(TEXT("+")), 44.f, 44.f, TEXT("Stepper")));
		IncreaseButton = Button;
		IncreaseButton->Rename(TEXT("QuantityIncrease"));
		UHorizontalBox* Presets = WidgetTree->ConstructWidget<UHorizontalBox>();
		UVerticalBoxSlot* PresetLayout = Column->AddChildToVerticalBox(Presets);
		PresetLayout->SetHorizontalAlignment(HAlign_Center);
		PresetLayout->SetPadding(FMargin(0.f, 12.f, 0.f, 16.f));
		Presets->AddChildToHorizontalBox(BuildActionButton(Button, Label, NSLOCTEXT("Quantity", "One", "1개"), 72.f, 36.f, TEXT("QuantityPreset")))->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		OneButton = Button;
		OneButton->Rename(TEXT("QuantityOne"));
		Presets->AddChildToHorizontalBox(BuildActionButton(Button, Label, NSLOCTEXT("Quantity", "Half", "절반"), 72.f, 36.f, TEXT("QuantityPreset")))->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		HalfButton = Button;
		HalfButton->Rename(TEXT("QuantityHalf"));
		Presets->AddChildToHorizontalBox(BuildActionButton(Button, Label, NSLOCTEXT("Quantity", "All", "전체"), 72.f, 36.f, TEXT("QuantityPreset")));
		AllButton = Button;
		AllButton->Rename(TEXT("QuantityAll"));
		PriceRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("QuantityPriceRow"));
		UBorder* PriceFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SaleTotal"));
		PriceFrame->SetPadding(FMargin(12.f, 10.f));
		PriceFrame->SetContent(PriceRow);
		Column->AddChildToVerticalBox(PriceFrame)->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));
		UTextBlock* PriceCaption = BuildSectionLabel(NSLOCTEXT("Quantity", "SaleTotal", "판매 금액"));
		ApplyTextRole(PriceCaption, EMKTextRole::Body);
		PriceRow->AddChildToHorizontalBox(PriceCaption)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UTextBlock* Price = nullptr;
		PriceRow->AddChildToHorizontalBox(BuildCurrencyAmount(Price, FText::GetEmpty()));
		TotalPrice = Price;
		UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Actions)->SetHorizontalAlignment(HAlign_Center);
		UButton* Confirm = nullptr;
		Actions->AddChildToHorizontalBox(BuildActionButton(Confirm, Label, NSLOCTEXT("Sale", "Sell", "판매"), 160.f))->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		ConfirmButton = Confirm;
		ConfirmButton->Rename(TEXT("QuantityConfirm"));
		ConfirmLabel = Label;
		UButton* Cancel = nullptr;
		Actions->AddChildToHorizontalBox(BuildActionButton(Cancel, Label, NSLOCTEXT("Sale", "Cancel", "취소"), 160.f, 44.f, TEXT("ActionSecondary")));
		CancelButton = Cancel;
		CancelButton->Rename(TEXT("QuantityCancel"));
	}
	return Super::RebuildWidget();
}

FReply UItemQuantityDialog::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape) { OnCancel(); return FReply::Handled(); }
	if (Event.GetKey() == EKeys::Enter) { OnConfirm(); return FReply::Handled(); }
	return Super::NativeOnKeyDown(Geometry, Event);
}

void UItemQuantityDialog::ShowQuantity(FName InItemKey, int32 InMaximumCount, bool bDrop)
{
	ItemKey = InItemKey;
	MaximumCount = FMath::Max(1, InMaximumCount);
	TakeWidget();
	ActionTitle->SetText(bDrop ? NSLOCTEXT("Item", "DropQuantity", "버릴 수량") : NSLOCTEXT("Sale", "Quantity", "판매 수량"));
	ConfirmLabel->SetText(bDrop ? NSLOCTEXT("Item", "Drop", "버리기") : NSLOCTEXT("Sale", "Sell", "판매"));
	CastChecked<UMKMockupButton>(ConfirmButton)->SetVisualRole(bDrop ? TEXT("ActionDanger") : TEXT("ActionPrimary"));
	QuantityInput->SetMaxValue(MaximumCount);
	QuantityInput->SetMaxSliderValue(MaximumCount);
	QuantityInput->SetValue(bDrop ? 1 : MaximumCount);
	const UDataManager* Data = UDataManager::Get(this);
	ItemTitle->SetText(::IsValid(Data) ? Data->GetItemDisplayName(ItemKey) : FText::FromName(ItemKey));
	const FItemDataTableRow* Item = ::IsValid(Data) ? Data->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemKey) : nullptr;
	if (Item != nullptr) { ItemIcon->SetBrushResourceObject(Item->ItemIcon.LoadSynchronous()); }
	OwnedText->SetText(FText::Format(NSLOCTEXT("Quantity", "Owned", "보유 {0}개"), FText::AsNumber(MaximumCount)));
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	UnitPrice = ::IsValid(Inventory) ? Inventory->CalculateSellPrice(ItemKey, 1) : 0;
	PriceRow->SetVisibility(bDrop ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	PriceRow->GetParent()->SetVisibility(bDrop ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	OnQuantityChanged(QuantityInput->GetValue());
	SetVisibility(ESlateVisibility::Visible);
	QuantityInput->SetKeyboardFocus();
}

void UItemQuantityDialog::BindEvents()
{
	Super::BindEvents();
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &UItemQuantityDialog::OnConfirm);
	CancelButton->OnClicked.AddUniqueDynamic(this, &UItemQuantityDialog::OnCancel);
	OneButton->OnClicked.AddUniqueDynamic(this, &UItemQuantityDialog::OnOne);
	HalfButton->OnClicked.AddUniqueDynamic(this, &UItemQuantityDialog::OnHalf);
	AllButton->OnClicked.AddUniqueDynamic(this, &UItemQuantityDialog::OnAll);
	DecreaseButton->OnClicked.AddUniqueDynamic(this, &UItemQuantityDialog::OnDecrease);
	IncreaseButton->OnClicked.AddUniqueDynamic(this, &UItemQuantityDialog::OnIncrease);
	QuantityInput->OnValueChanged.AddUniqueDynamic(this, &UItemQuantityDialog::OnQuantityChanged);
	QuantityInput->OnValueCommitted.AddUniqueDynamic(this, &UItemQuantityDialog::OnQuantityCommitted);
}

void UItemQuantityDialog::UnbindEvents()
{
	Super::UnbindEvents();
	ConfirmButton->OnClicked.RemoveDynamic(this, &UItemQuantityDialog::OnConfirm);
	CancelButton->OnClicked.RemoveDynamic(this, &UItemQuantityDialog::OnCancel);
	OneButton->OnClicked.RemoveAll(this);
	HalfButton->OnClicked.RemoveAll(this);
	AllButton->OnClicked.RemoveAll(this);
	DecreaseButton->OnClicked.RemoveAll(this);
	IncreaseButton->OnClicked.RemoveAll(this);
	QuantityInput->OnValueChanged.RemoveAll(this);
	QuantityInput->OnValueCommitted.RemoveAll(this);
}

void UItemQuantityDialog::SetQuantity(int32 Count)
{
	QuantityInput->SetValue(FMath::Clamp(Count, 1, MaximumCount));
	OnQuantityChanged(QuantityInput->GetValue());
}

void UItemQuantityDialog::OnConfirm()
{
	const int32 Count = FMath::Clamp(FMath::RoundToInt(QuantityInput->GetValue()), 1, MaximumCount);
	SetVisibility(ESlateVisibility::Collapsed);
	OnQuantityConfirmed.Broadcast(ItemKey, Count);
}

void UItemQuantityDialog::OnCancel()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnQuantityCancelled.Broadcast();
}

void UItemQuantityDialog::OnQuantityChanged(float Value)
{
	const int32 Count = FMath::Clamp(FMath::RoundToInt(Value), 1, MaximumCount);
	TotalPrice->SetText(FText::AsNumber(static_cast<int64>(UnitPrice) * Count));
	DecreaseButton->SetIsEnabled(Count > 1);
	IncreaseButton->SetIsEnabled(Count < MaximumCount);
	CastChecked<UMKMockupButton>(OneButton)->SetVisualRole(Count == 1 ? TEXT("QuantityPresetSelected") : TEXT("QuantityPreset"));
	CastChecked<UMKMockupButton>(HalfButton)->SetVisualRole(Count > 1 && Count == MaximumCount / 2 ? TEXT("QuantityPresetSelected") : TEXT("QuantityPreset"));
	CastChecked<UMKMockupButton>(AllButton)->SetVisualRole(Count > 1 && Count == MaximumCount ? TEXT("QuantityPresetSelected") : TEXT("QuantityPreset"));
}

void UItemQuantityDialog::OnQuantityCommitted(float Value, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter) { OnConfirm(); }
}

void UItemQuantityDialog::OnOne() { SetQuantity(1); }
void UItemQuantityDialog::OnHalf() { SetQuantity(MaximumCount / 2); }
void UItemQuantityDialog::OnAll() { SetQuantity(MaximumCount); }
void UItemQuantityDialog::OnDecrease() { SetQuantity(FMath::RoundToInt(QuantityInput->GetValue()) - 1); }
void UItemQuantityDialog::OnIncrease() { SetQuantity(FMath::RoundToInt(QuantityInput->GetValue()) + 1); }
