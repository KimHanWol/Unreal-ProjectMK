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

TSharedRef<SWidget> UItemQuantityDialog::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
		Dim->SetHorizontalAlignment(HAlign_Center);
		Dim->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Dim;
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(360.f);
		Dim->SetContent(Size);
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		Panel->SetPadding(FMargin(24.f));
		Size->SetContent(Panel);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(Column);
		UTextBlock* HeaderText = nullptr;
		Column->AddChildToVerticalBox(BuildSectionHeader(NSLOCTEXT("Sale", "Quantity", "판매 수량"), TEXT("Currency"), &HeaderText));
		ActionTitle = HeaderText;
		ItemTitle = WidgetTree->ConstructWidget<UTextBlock>();
		ApplyTextRole(ItemTitle, EMKTextRole::Label);
		Column->AddChildToVerticalBox(ItemTitle)->SetPadding(FMargin(0.f, 16.f));
		QuantityInput = WidgetTree->ConstructWidget<USpinBox>();
		QuantityInput->SetMinValue(1.f);
		QuantityInput->SetMinSliderValue(1.f);
		QuantityInput->SetDelta(1.f);
		QuantityInput->SetMinFractionalDigits(0);
		QuantityInput->SetMaxFractionalDigits(0);
		USizeBox* InputSize = WidgetTree->ConstructWidget<USizeBox>();
		InputSize->SetHeightOverride(FMKUILayout::ButtonHeight);
		InputSize->SetContent(QuantityInput);
		Column->AddChildToVerticalBox(InputSize)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
		UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Actions);
		UButton* Confirm = nullptr;
		UTextBlock* Label = nullptr;
		Actions->AddChildToHorizontalBox(BuildActionButton(Confirm, Label, NSLOCTEXT("Sale", "Sell", "판매")))->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		ConfirmButton = Confirm;
		ConfirmLabel = Label;
		UButton* Cancel = nullptr;
		Actions->AddChildToHorizontalBox(BuildActionButton(Cancel, Label, NSLOCTEXT("Sale", "Cancel", "취소")));
		CancelButton = Cancel;
	}
	return Super::RebuildWidget();
}

void UItemQuantityDialog::ShowQuantity(FName InItemKey, int32 InMaximumCount, bool bDrop)
{
	ItemKey = InItemKey;
	MaximumCount = FMath::Max(1, InMaximumCount);
	TakeWidget();
	ActionTitle->SetText(bDrop ? NSLOCTEXT("Item", "DropQuantity", "버릴 수량") : NSLOCTEXT("Sale", "Quantity", "판매 수량"));
	ConfirmLabel->SetText(bDrop ? NSLOCTEXT("Item", "Drop", "버리기") : NSLOCTEXT("Sale", "Sell", "판매"));
	QuantityInput->SetMaxValue(MaximumCount);
	QuantityInput->SetMaxSliderValue(MaximumCount);
	QuantityInput->SetValue(MaximumCount);
	const UDataManager* Data = UDataManager::Get(this);
	ItemTitle->SetText(::IsValid(Data) ? Data->GetItemDisplayName(ItemKey) : FText::FromName(ItemKey));
	SetVisibility(ESlateVisibility::Visible);
}

void UItemQuantityDialog::BindEvents()
{
	Super::BindEvents();
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &UItemQuantityDialog::OnConfirm);
	CancelButton->OnClicked.AddUniqueDynamic(this, &UItemQuantityDialog::OnCancel);
}

void UItemQuantityDialog::UnbindEvents()
{
	Super::UnbindEvents();
	ConfirmButton->OnClicked.RemoveDynamic(this, &UItemQuantityDialog::OnConfirm);
	CancelButton->OnClicked.RemoveDynamic(this, &UItemQuantityDialog::OnCancel);
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
