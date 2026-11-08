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
#include "Rendering/DrawElementTypes.h"
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

void USkillSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bPendingClickPurchase)
	{
		HoldProgress = FMath::Min(1.f, HoldProgress + InDeltaTime / PurchaseHoldDuration);
		if (HoldProgress >= 1.f)
		{
			bPendingClickPurchase = false;
			OnSlotClicked();
		}
		InvalidateLayoutAndVolatility();
	}
}

int32 USkillSlotWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& DrawElements, int32 LayerId, const FWidgetStyle& Style, bool bEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, DrawElements, LayerId, Style, bEnabled);
	if (bPendingClickPurchase && HoldProgress > 0.f)
	{
		TArray<FVector2D> Points;
		const FVector2D Center = Geometry.GetLocalSize() * 0.5f;
		const float Radius = FMath::Min(Center.X, Center.Y) - 3.f;
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(HoldProgress * 64.f));
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const float Angle = -HALF_PI + 2.f * PI * HoldProgress * Index / Steps;
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		FSlateDrawElement::MakeLines(DrawElements, Layer + 1, Geometry.ToPaintGeometry(), Points,
			ESlateDrawEffect::None, FLinearColor(1.f, 0.75f, 0.2f), true, 4.f);
	}
	return Layer + 1;
}

void USkillSlotWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
	bPendingClickPurchase = false;
	HoldProgress = 0.f;
}

FReply USkillSlotWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPendingClickPurchase)
	{
		if (FVector2D::Distance(PendingClickScreenPosition, InMouseEvent.GetScreenSpacePosition()) > 6.f)
		{
			bPendingClickPurchase = false;
			HoldProgress = 0.f;
		}
		return FReply::Handled();
	}
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
		HoldProgress = 0.f;
		if (bPendingClickPurchase)
		{
			return FReply::Handled().CaptureMouse(TakeWidget());
		}

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
	const bool bCanHandleClick = bIsLeftMouseButton && bPendingClickPurchase && HoldProgress >= 1.f;
	bool bEndedSkillTreeDrag = false;

	bPendingClickPurchase = false;
	HoldProgress = 0.f;

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
	DefaultSlotSize = FVector2D(96.f);
	Super::BuildDefaultWidgetTree();
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
	if (::IsValid(IconImage))
	{
		IconImage->SetColorAndOpacity(bIsPurchased ? FLinearColor(0.5f, 1.f, 0.8f) : (bShouldEnable ? FLinearColor::White : FLinearColor(0.4f, 0.4f, 0.4f)));
	}
}

void USkillSlotWidget::UpdateCompleteImage()
{
	USkillComponent* SkillComponent = GetLocalSkillComponent();
	const bool bIsPurchased = ::IsValid(SkillComponent) && SkillComponent->HasPurchasedSkill(SkillKeyName);
	SetSelected(false);
	if (::IsValid(SelectionText))
	{
		const FString Key = SkillKeyName.ToString();
		const FString Rank = Key.EndsWith(TEXT("_1")) ? TEXT("I") : (Key.EndsWith(TEXT("_2")) ? TEXT("II") : (Key.EndsWith(TEXT("_3")) ? TEXT("III") : TEXT("")));
		SelectionText->SetText(FText::FromString(Rank));
		SelectionText->SetVisibility(Rank.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (::IsValid(CountText))
	{
		CountText->SetText(bIsPurchased ? NSLOCTEXT("Skill", "Owned", "습득") : FText::GetEmpty());
		CountText->SetVisibility(bIsPurchased ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
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

	SetSlotIcon(IconTexture);
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
