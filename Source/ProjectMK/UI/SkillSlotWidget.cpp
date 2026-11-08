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
#include "Framework/Application/SlateApplication.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"
#include "ProjectMK/UI/MKTooltipBase.h"
#include "ProjectMK/UI/HUDWidget.h"
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
	UnlockFlashTime = FMath::Max(0.f, UnlockFlashTime - InDeltaTime);
	if (bPendingClickPurchase)
	{
		HoldProgress = FMath::Min(1.f, HoldProgress + InDeltaTime / PurchaseHoldDuration);
		if (HoldProgress >= 1.f)
		{
			bPendingClickPurchase = false;
			OnSlotClicked();
			UnlockFlashTime = 0.25f;
			if (HasMouseCapture()) { FSlateApplication::Get().ReleaseAllPointerCapture(); }
		}
		InvalidateLayoutAndVolatility();
	}
}

int32 USkillSlotWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& DrawElements, int32 LayerId, const FWidgetStyle& Style, bool bEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, DrawElements, LayerId, Style, bEnabled);
	if ((bPendingClickPurchase && HoldProgress > 0.f) || UnlockFlashTime > 0.f)
	{
		const FVector2D Size = Geometry.GetLocalSize();
		const TArray<FVector2D> Track = {
			{ Size.X * 0.5f, 5.f }, { Size.X - 10.f, 5.f }, { Size.X - 5.f, 10.f },
			{ Size.X - 5.f, Size.Y - 10.f }, { Size.X - 10.f, Size.Y - 5.f },
			{ 10.f, Size.Y - 5.f }, { 5.f, Size.Y - 10.f }, { 5.f, 10.f },
			{ 10.f, 5.f }, { Size.X * 0.5f, 5.f }
		};
		float Length = 0.f;
		for (int32 Index = 1; Index < Track.Num(); ++Index) { Length += FVector2D::Distance(Track[Index - 1], Track[Index]); }
		float Remaining = Length * (UnlockFlashTime > 0.f ? 1.f : HoldProgress);
		TArray<FVector2D> Progress = { Track[0] };
		for (int32 Index = 1; Index < Track.Num(); ++Index)
		{
			const FVector2D Segment = Track[Index] - Track[Index - 1];
			Progress.Add(Track[Index - 1] + Segment.GetSafeNormal() * FMath::Min(Remaining, Segment.Size()));
			Remaining -= Segment.Size();
			if (Remaining <= 0.f) { break; }
		}
		const FLinearColor Color = UnlockFlashTime > 0.f ? FLinearColor(0.35f, 0.9f, 0.68f) : FLinearColor(0.95f, 0.68f, 0.28f);
		FSlateDrawElement::MakeLines(DrawElements, Layer + 1, Geometry.ToPaintGeometry(), Track,
			ESlateDrawEffect::None, FLinearColor(0.02f, 0.04f, 0.05f), false, 8.f);
		FSlateDrawElement::MakeLines(DrawElements, Layer + 2, Geometry.ToPaintGeometry(), Track,
			ESlateDrawEffect::None, FLinearColor(0.23f, 0.29f, 0.29f), false, 5.f);
		FSlateDrawElement::MakeLines(DrawElements, Layer + 3, Geometry.ToPaintGeometry(), Progress,
			ESlateDrawEffect::None, Color, false, 4.f);
		FSlateDrawElement::MakeLines(DrawElements, Layer + 4, Geometry.ToPaintGeometry(), Progress,
			ESlateDrawEffect::None, FMath::Lerp(Color, FLinearColor::White, 0.3f), false, 1.f);
		const FVector2D Tip = Progress.Last();
		const TArray<FVector2D> Marker = { Tip + FVector2D(0.f, -3.f), Tip + FVector2D(3.f, 0.f), Tip + FVector2D(0.f, 3.f), Tip + FVector2D(-3.f, 0.f), Tip + FVector2D(0.f, -3.f) };
		FSlateDrawElement::MakeLines(DrawElements, Layer + 5, Geometry.ToPaintGeometry(), Marker,
			ESlateDrawEffect::None, FLinearColor(1.f, 0.91f, 0.65f), false, 2.f);
	}
	return Layer + 5;
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
			if (USkillTreeMenuContents* Tree = GetOwningSkillTreeMenuContents())
			{
				Tree->BeginSkillTreeDrag(PendingClickScreenPosition);
				Tree->UpdateSkillTreeDrag(InMouseEvent.GetScreenSpacePosition());
				return FReply::Handled().CaptureMouse(Tree->TakeWidget());
			}
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

	bPendingClickPurchase = false;
	HoldProgress = 0.f;

	if (USkillTreeMenuContents* SkillTreeMenuContents = GetOwningSkillTreeMenuContents())
	{
		SkillTreeMenuContents->EndSkillTreeDrag();
	}

	if (bIsLeftMouseButton)
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
	if (SkillKeyName.IsNone())
	{
		return;
	}
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
	DefaultSlotSize = FVector2D(84.f);
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
	SetRenderOpacity(1.f);
	if (UBorder* Frame = Cast<UBorder>(WidgetTree->FindWidget(TEXT("Slot"))))
	{
		Frame->SetBrushColor(bShouldEnable ? FLinearColor::White : FLinearColor(0.3f, 0.33f, 0.34f));
	}
	if (::IsValid(RankBorder)) { RankBorder->SetRenderOpacity(bShouldEnable ? 1.f : 0.4f); }
	if (::IsValid(LockImage))
	{
		LockImage->SetVisibility(bShouldEnable ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (::IsValid(IconImage))
	{
		IconImage->SetColorAndOpacity(bIsPurchased ? FLinearColor(0.5f, 1.f, 0.8f) : (bShouldEnable ? FLinearColor::White : FLinearColor(0.22f, 0.25f, 0.27f)));
	}
}

void USkillSlotWidget::UpdateCompleteImage()
{
	USkillComponent* SkillComponent = GetLocalSkillComponent();
	const bool bIsPurchased = ::IsValid(SkillComponent) && SkillComponent->HasPurchasedSkill(SkillKeyName);
	SetSelected(false);
	SetAcquired(bIsPurchased);
	const FString Key = SkillKeyName.ToString();
	const FString Rank = Key.EndsWith(TEXT("_1")) ? TEXT("I") : (Key.EndsWith(TEXT("_2")) ? TEXT("II") : (Key.EndsWith(TEXT("_3")) ? TEXT("III") : TEXT("")));
	SetRankBadge(FText::FromString(Rank));
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
	if (IsHovered()) { if (UHUDWidget* HUD = GetHUDWidget()) { HUD->SetHoveredSlot(this); } }
}

void USkillSlotWidget::OnSkillPurchased(FName PurchasedSkillKeyName)
{
	UpdateWidget();
	if (IsHovered()) { if (UHUDWidget* HUD = GetHUDWidget()) { HUD->SetHoveredSlot(this); } }
}
