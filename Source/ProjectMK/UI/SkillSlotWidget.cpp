// LINK

#include "ProjectMK/UI/SkillSlotWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"
#include "ProjectMK/UI/MKTooltipBase.h"
#include "ProjectMK/UI/SkillTreeMenuContents.h"

TSharedRef<SWidget> USkillSlotWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

FReply USkillSlotWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (USkillTreeMenuContents* SkillTreeMenuContents = GetOwningSkillTreeMenuContents())
		{
			if (SkillTreeMenuContents->UpdateSkillTreeDrag(InMouseEvent.GetScreenSpacePosition()))
			{
				return FReply::Handled();
			}
		}
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply USkillSlotWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USkillSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bPendingClickPurchase = CanClickSlot();
		PendingClickScreenPosition = InMouseEvent.GetScreenSpacePosition();

		if (USkillTreeMenuContents* SkillTreeMenuContents = GetOwningSkillTreeMenuContents())
		{
			if (SkillTreeMenuContents->BeginSkillTreeDrag(InMouseEvent.GetScreenSpacePosition()))
			{
				return FReply::Handled().CaptureMouse(SkillTreeMenuContents->TakeWidget());
			}
		}

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USkillSlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const bool bIsLeftMouseButton = InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
	const bool bCanHandleClick = bIsLeftMouseButton && bPendingClickPurchase;
	bool bEndedSkillTreeDrag = false;

	bPendingClickPurchase = false;

	if (USkillTreeMenuContents* SkillTreeMenuContents = GetOwningSkillTreeMenuContents())
	{
		bEndedSkillTreeDrag = SkillTreeMenuContents->HasActiveSkillTreeDrag();
		SkillTreeMenuContents->EndSkillTreeDrag();
	}

	if (bCanHandleClick)
	{
		const float ClickMovementDistance = FVector2D::Distance(PendingClickScreenPosition, InMouseEvent.GetScreenSpacePosition());
		if (ClickMovementDistance <= 4.f && IsHovered())
		{
			OnSlotClicked();
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	if (bIsLeftMouseButton && bEndedSkillTreeDrag)
	{
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void USkillSlotWidget::BindEvents()
{
	Super::BindEvents();

	if (::IsValid(OwnerASC))
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCoinAttribute()).AddUObject(this, &USkillSlotWidget::OnCoinChanged);
	}

	if (USkillComponent* SkillComponent = GetLocalSkillComponent())
	{
		SkillComponent->OnSkillPurchasedDelegate.AddUObject(this, &USkillSlotWidget::OnSkillPurchased);
	}
}

void USkillSlotWidget::UnbindEvents()
{
	Super::UnbindEvents();

	if (::IsValid(OwnerASC))
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCoinAttribute()).RemoveAll(this);
	}

	if (USkillComponent* SkillComponent = GetLocalSkillComponent())
	{
		SkillComponent->OnSkillPurchasedDelegate.RemoveAll(this);
	}
}

void USkillSlotWidget::UpdateWidget()
{
	UpdateSkillImage();
	UpdateCompleteImage();
	UpdatePriceText();
	UpdateEnabledState();
}

bool USkillSlotWidget::CanClickSlot() const
{
	USkillComponent* SkillComponent = GetLocalSkillComponent();
	return ::IsValid(SkillComponent) && SkillComponent->CanPurchaseSkill(SkillKeyName);
}

void USkillSlotWidget::OnSlotClicked()
{
	USkillComponent* SkillComponent = GetLocalSkillComponent();
	if (::IsValid(SkillComponent) == false)
	{
		return;
	}

	if (SkillComponent->TryPurchaseSkill(SkillKeyName))
	{
		UpdateCompleteImage();
	}
}

void USkillSlotWidget::UpdateTooltipWidget(UMKTooltipBase* TooltipWidget) const
{
	Super::UpdateTooltipWidget(TooltipWidget);

	if (::IsValid(TooltipWidget) == false || SkillKeyName.IsNone())
	{
		return;
	}

	TooltipWidget->ShowSkillTooltip(SkillKeyName);
}

void USkillSlotWidget::InitializeSkill(const FName InSkillKeyName)
{
	SkillKeyName = InSkillKeyName;
	UpdateWidget();
}

void USkillSlotWidget::BuildDefaultWidgetTree()
{
	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>();
	RootSize->SetWidthOverride(96.f);
	RootSize->SetHeightOverride(96.f);
	WidgetTree->RootWidget = RootSize;

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Slot"));
	Frame->SetPadding(FMargin(FMKUILayout::SpaceS));
	RootSize->SetContent(Frame);
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
	Frame->SetContent(Overlay);

	FocusImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectedSlot"));
	FocusImage->SetVisibility(ESlateVisibility::Collapsed);
	Overlay->AddChildToOverlay(FocusImage);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	UOverlaySlot* ColumnSlot = Overlay->AddChildToOverlay(Column);
	ColumnSlot->SetHorizontalAlignment(HAlign_Center);
	ColumnSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
	IconSize->SetWidthOverride(44.f);
	IconSize->SetHeightOverride(44.f);
	IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
	IconSize->SetContent(IconImage);
	UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconSize);
	IconSlot->SetHorizontalAlignment(HAlign_Center);

	UTextBlock* RawPriceText = nullptr;
	PriceRow = BuildCurrencyAmount(RawPriceText, FText::GetEmpty(), EMKTextRole::Caption);
	PriceText = RawPriceText;
	UVerticalBoxSlot* PriceSlot = Column->AddChildToVerticalBox(PriceRow);
	PriceSlot->SetHorizontalAlignment(HAlign_Center);
	PriceSlot->SetPadding(FMargin(0.f, FMKUILayout::SpaceXS, 0.f, 0.f));

	SelectionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectionText"));
	SelectionText->SetText(FText::FromString(TEXT("✓")));
	ApplyTextRole(SelectionText, EMKTextRole::Label);
	SelectionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 1.f, 0.72f)));
	SelectionText->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* SelectionSlot = Overlay->AddChildToOverlay(SelectionText);
	SelectionSlot->SetHorizontalAlignment(HAlign_Right);
	SelectionSlot->SetVerticalAlignment(VAlign_Top);
}

USkillTreeMenuContents* USkillSlotWidget::GetOwningSkillTreeMenuContents() const
{
	return GetTypedOuter<USkillTreeMenuContents>();
}

void USkillSlotWidget::UpdateEnabledState()
{
	USkillComponent* SkillComponent = GetLocalSkillComponent();
	const bool bIsPurchased = ::IsValid(SkillComponent) && SkillComponent->HasPurchasedSkill(SkillKeyName);
	const bool bShouldEnable = bIsPurchased || AreRequiredSkillsPurchased();

	SetIsEnabled(true);
	SetRenderOpacity(bShouldEnable ? 1.f : 0.45f);
}

void USkillSlotWidget::UpdateCompleteImage()
{
	USkillComponent* SkillComponent = GetLocalSkillComponent();
	const bool bIsPurchased = ::IsValid(SkillComponent) && SkillComponent->HasPurchasedSkill(SkillKeyName);
	SetSelected(bIsPurchased);
	if (::IsValid(CompleteImage))
	{
		CompleteImage->SetVisibility(bIsPurchased ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void USkillSlotWidget::UpdateSkillImage()
{
	if (::IsValid(IconImage) == false)
	{
		return;
	}

	IconImage->SetBrush(FSlateBrush());

	if (SkillKeyName.IsNone())
	{
		return;
	}

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	const FSkillDataTableRow* SkillDataTableRow = DataManager->GetSkillDataTableRow(SkillKeyName);
	if (SkillDataTableRow == nullptr || SkillDataTableRow->IconPath.IsNull())
	{
		return;
	}

	UTexture2D* IconTexture = SkillDataTableRow->IconPath.LoadSynchronous();
	if (::IsValid(IconTexture) == false)
	{
		return;
	}

	IconImage->SetBrushFromTexture(IconTexture, true);
}

void USkillSlotWidget::UpdatePriceText()
{
	if (::IsValid(PriceText) == false)
	{
		return;
	}

	const UDataManager* DataManager = UDataManager::Get(this);
	const FSkillDataTableRow* SkillDataTableRow = ::IsValid(DataManager) ? DataManager->GetSkillDataTableRow(SkillKeyName) : nullptr;
	PriceText->SetText(SkillDataTableRow != nullptr ? FText::AsNumber(SkillDataTableRow->Price) : FText::GetEmpty());

	const USkillComponent* SkillComponent = GetLocalSkillComponent();
	const bool bIsPurchased = ::IsValid(SkillComponent) && SkillComponent->HasPurchasedSkill(SkillKeyName);
	if (::IsValid(PriceRow))
	{
		PriceRow->SetVisibility(bIsPurchased ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

bool USkillSlotWidget::AreRequiredSkillsPurchased() const
{
	const UDataManager* DataManager = UDataManager::Get(const_cast<USkillSlotWidget*>(this));
	const USkillComponent* SkillComponent = GetLocalSkillComponent();
	const FSkillDataTableRow* SkillDataTableRow = ::IsValid(DataManager) ? DataManager->GetSkillDataTableRow(SkillKeyName) : nullptr;
	if (SkillDataTableRow == nullptr || ::IsValid(SkillComponent) == false)
	{
		return false;
	}

	for (const FName& RequiredSkillKey : SkillDataTableRow->RequiredSkill)
	{
		if (SkillComponent->HasPurchasedSkill(RequiredSkillKey) == false)
		{
			return false;
		}
	}
	return true;
}

void USkillSlotWidget::OnCoinChanged(const FOnAttributeChangeData& Data)
{
	UpdateWidget();
}

void USkillSlotWidget::OnSkillPurchased(FName PurchasedSkillKeyName)
{
	UpdateWidget();
}
