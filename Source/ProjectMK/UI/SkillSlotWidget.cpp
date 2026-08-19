// LINK

#include "ProjectMK/UI/SkillSlotWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Engine/Texture2D.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"
#include "ProjectMK/UI/MKTooltipBase.h"
#include "ProjectMK/UI/SkillTreeMenuContents.h"

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

USkillTreeMenuContents* USkillSlotWidget::GetOwningSkillTreeMenuContents() const
{
	UPanelWidget* ParentWidget = GetParent();
	while (::IsValid(ParentWidget))
	{
		if (USkillTreeMenuContents* SkillTreeMenuContents = Cast<USkillTreeMenuContents>(ParentWidget))
		{
			return SkillTreeMenuContents;
		}

		ParentWidget = ParentWidget->GetParent();
	}

	return nullptr;
}

void USkillSlotWidget::UpdateEnabledState()
{
	USkillComponent* SkillComponent = GetLocalSkillComponent();
	const bool bIsPurchased = ::IsValid(SkillComponent) && SkillComponent->HasPurchasedSkill(SkillKeyName);
	const bool bCanPurchase = ::IsValid(SkillComponent) && SkillComponent->CanPurchaseSkill(SkillKeyName);
	const bool bShouldEnable = bIsPurchased || bCanPurchase;

	SetIsEnabled(true);
	SetRenderOpacity(bShouldEnable ? 1.f : 0.45f);
}

void USkillSlotWidget::UpdateCompleteImage()
{
	if (::IsValid(CompleteImage) == false)
	{
		return;
	}

	USkillComponent* SkillComponent = GetLocalSkillComponent();
	const bool bIsPurchased = ::IsValid(SkillComponent) && SkillComponent->HasPurchasedSkill(SkillKeyName);
	CompleteImage->SetVisibility(bIsPurchased ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
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

void USkillSlotWidget::OnCoinChanged(const FOnAttributeChangeData& Data)
{
	UpdateWidget();
}

void USkillSlotWidget::OnSkillPurchased(FName PurchasedSkillKeyName)
{
	UpdateWidget();
}
