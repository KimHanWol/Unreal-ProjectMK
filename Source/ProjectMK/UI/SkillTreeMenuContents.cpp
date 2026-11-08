// LINK

#include "ProjectMK/UI/SkillTreeMenuContents.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"
#include "ProjectMK/UI/MenuDisplayCardWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Reply.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"
#include "ProjectMK/UI/SkillSlotWidget.h"

namespace
{
	constexpr float SkillNodeSize = 84.f;
	constexpr float SkillTreePadding = 40.f;
	constexpr float SkillConnectionThickness = 4.f;
	constexpr float SkillGridStepX = 84.f;
	constexpr float SkillGridStepY = 84.f;

	TMap<FName, FIntPoint> CreateSkillGridPositions()
	{
		TMap<FName, FIntPoint> Positions;
		Positions.Emplace(TEXT("Skill_DrillPower_1"), FIntPoint(12, 10));
		Positions.Emplace(TEXT("Skill_DrillPower_2"), FIntPoint(14, 10));
		Positions.Emplace(TEXT("Skill_DrillPower_3"), FIntPoint(16, 10));
		Positions.Emplace(TEXT("Skill_DrillSpeed_1"), FIntPoint(14, 12));
		Positions.Emplace(TEXT("Skill_DrillSpeed_2"), FIntPoint(16, 12));
		Positions.Emplace(TEXT("Skill_DrillSpeed_3"), FIntPoint(18, 12));
		Positions.Emplace(TEXT("Skill_DrillRange_1"), FIntPoint(14, 8));
		Positions.Emplace(TEXT("Skill_DrillRange_2"), FIntPoint(16, 8));
		Positions.Emplace(TEXT("Skill_ChainBreak_1"), FIntPoint(18, 8));
		Positions.Emplace(TEXT("Skill_ChainBreak_2"), FIntPoint(18, 10));
		Positions.Emplace(TEXT("Skill_Recycler"), FIntPoint(10, 12));
		Positions.Emplace(TEXT("Skill_LuckyStrike"), FIntPoint(12, 12));
		Positions.Emplace(TEXT("Skill_Scavenger"), FIntPoint(10, 14));
		Positions.Emplace(TEXT("Skill_CoinMaster"), FIntPoint(10, 16));

		Positions.Emplace(TEXT("Skill_OxygenTank_1"), FIntPoint(10, 8));
		Positions.Emplace(TEXT("Skill_OxygenTank_2"), FIntPoint(10, 6));
		Positions.Emplace(TEXT("Skill_OxygenTank_3"), FIntPoint(10, 4));
		Positions.Emplace(TEXT("Skill_LastOxygen"), FIntPoint(12, 6));
		Positions.Emplace(TEXT("Skill_AirPocket"), FIntPoint(14, 6));
		Positions.Emplace(TEXT("Skill_HealthBoost_1"), FIntPoint(8, 8));
		Positions.Emplace(TEXT("Skill_HealthBoost_2"), FIntPoint(6, 4));
		Positions.Emplace(TEXT("Skill_HealthRegen_1"), FIntPoint(6, 8));
		Positions.Emplace(TEXT("Skill_HealthRegen_2"), FIntPoint(6, 6));
		Positions.Emplace(TEXT("Skill_HealthRegen_3"), FIntPoint(6, 2));
		Positions.Emplace(TEXT("Skill_Insurance"), FIntPoint(4, 8));

		Positions.Emplace(TEXT("Skill_Magnet_1"), FIntPoint(8, 10));
		Positions.Emplace(TEXT("Skill_Magnet_2"), FIntPoint(4, 10));
		Positions.Emplace(TEXT("Skill_BagUpgrade_1"), FIntPoint(6, 10));
		Positions.Emplace(TEXT("Skill_BagUpgrade_2"), FIntPoint(2, 10));
		Positions.Emplace(TEXT("Skill_FieldRepair"), FIntPoint(6, 12));

		Positions.Emplace(TEXT("Skill_MoveSpeed_1"), FIntPoint(4, 14));
		Positions.Emplace(TEXT("Skill_MoveSpeed_2"), FIntPoint(4, 16));
		Positions.Emplace(TEXT("Skill_FlyingSpeed_1"), FIntPoint(6, 14));
		Positions.Emplace(TEXT("Skill_FlyingSpeed_2"), FIntPoint(6, 16));
		Positions.Emplace(TEXT("CharacterSkill_MinerDiscipline"), FIntPoint(10, 2));
		Positions.Emplace(TEXT("CharacterSkill_RecklessDig"), FIntPoint(12, 2));
		Positions.Emplace(TEXT("CharacterSkill_SpeedRunner"), FIntPoint(14, 2));
		Positions.Emplace(TEXT("CharacterSkill_HeavyCarrier"), FIntPoint(16, 2));
		return Positions;
	}

	const TMap<FName, FIntPoint> SkillGridPositions = CreateSkillGridPositions();
}

USkillTreeMenuContents::USkillTreeMenuContents(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MenuContentsType = EMenuContentsType::SkillTree;
	SkillSlotWidgetClass = USkillSlotWidget::StaticClass();
}

TSharedRef<SWidget> USkillTreeMenuContents::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->FindWidget(TEXT("RuntimeSkillTreeViewport")) == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

void USkillTreeMenuContents::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshSkillTree();
}

void USkillTreeMenuContents::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bNeedsCentering && ::IsValid(SkillTreeViewportPanel))
	{
		const FVector2D ViewSize = SkillTreeViewportPanel->GetCachedGeometry().GetLocalSize();
		if (ViewSize.X > 100.f && ViewSize.Y > 100.f)
		{
			SkillTreeCanvasPanel->SetRenderTransformPivot(FVector2D::ZeroVector);
			SkillTreeCanvasPanel->SetRenderScale(FVector2D(CurrentZoomScale));
			CastChecked<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot)->SetPosition(CalculateClampedCanvasPosition(ViewSize * 0.5f - CentralPlayerPosition * CurrentZoomScale, CurrentZoomScale));
			bNeedsCentering = false;
		}
	}
}

FReply USkillTreeMenuContents::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && BeginSkillTreeDrag(InMouseEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USkillTreeMenuContents::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingSkillTree && UpdateSkillTreeDrag(InMouseEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply USkillTreeMenuContents::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDraggingSkillTree)
	{
		EndSkillTreeDrag();
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply USkillTreeMenuContents::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (UpdateSkillTreeZoom(InGeometry, InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

bool USkillTreeMenuContents::BeginSkillTreeDrag(const FVector2D& ScreenSpacePosition)
{
	UCanvasPanelSlot* CanvasPanelSlot = ::IsValid(SkillTreeCanvasPanel) ? Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot) : nullptr;
	if (::IsValid(CanvasPanelSlot) == false)
	{
		return false;
	}

	DragStartScreenPosition = ScreenSpacePosition;
	DragStartCanvasPosition = CanvasPanelSlot->GetPosition();
	bIsDraggingSkillTree = true;
	return true;
}

bool USkillTreeMenuContents::HasActiveSkillTreeDrag() const
{
	return bIsDraggingSkillTree;
}

bool USkillTreeMenuContents::UpdateSkillTreeDrag(const FVector2D& ScreenSpacePosition)
{
	UCanvasPanelSlot* CanvasPanelSlot = ::IsValid(SkillTreeCanvasPanel) ? Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot) : nullptr;
	if (bIsDraggingSkillTree == false || ::IsValid(CanvasPanelSlot) == false)
	{
		return false;
	}

	const FVector2D DragDelta = ScreenSpacePosition - DragStartScreenPosition;
	const FVector2D TargetPosition = DragStartCanvasPosition + DragDelta;
	CanvasPanelSlot->SetPosition(CalculateClampedCanvasPosition(TargetPosition, CurrentZoomScale));
	return true;
}

void USkillTreeMenuContents::EndSkillTreeDrag()
{
	bIsDraggingSkillTree = false;
}

void USkillTreeMenuContents::BuildDefaultWidgetTree()
{
	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InsetPanel"));
	RootBorder->SetPadding(FMargin(FMKUILayout::SpaceXL));
	WidgetTree->RootWidget = RootBorder;

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>();
	RootBorder->SetContent(RootBox);
	RootBox->AddChildToVerticalBox(BuildSectionHeader(NSLOCTEXT("SkillTreeMenu", "Title", "기술 설계도"), NAME_None))
		->SetPadding(FMargin(FMKUILayout::SpaceS, 0.f, FMKUILayout::SpaceS, FMKUILayout::SpaceS));

	SkillTreeViewportPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RuntimeSkillTreeViewport"));
	SkillTreeViewportPanel->SetClipping(EWidgetClipping::ClipToBounds);
	UVerticalBoxSlot* ViewportSlot = RootBox->AddChildToVerticalBox(SkillTreeViewportPanel);
	ViewportSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	SkillTreeCanvasPanel = WidgetTree->ConstructWidget<UCanvasPanel>();
	UCanvasPanelSlot* TreeSlot = SkillTreeViewportPanel->AddChildToCanvas(SkillTreeCanvasPanel);
	TreeSlot->SetPosition(FVector2D::ZeroVector);
	TreeSlot->SetSize(FVector2D(1024.f, 640.f));
}

void USkillTreeMenuContents::RefreshSkillTree()
{
	const UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false || ::IsValid(SkillTreeCanvasPanel) == false || ::IsValid(SkillSlotWidgetClass) == false)
	{
		return;
	}

	SkillTreeCanvasPanel->ClearChildren();
	TArray<FName> SkillKeys = DataManager->GetSkillRowNames();
	SkillKeys.RemoveAll([DataManager](const FName SkillKey)
	{
		const FSkillDataTableRow* SkillData = DataManager->GetSkillDataTableRow(SkillKey);
		return SkillData == nullptr || SkillData->DisplayName.IsEmpty() || SkillGridPositions.Contains(SkillKey) == false;
	});
	SkillKeys.Sort(FNameLexicalLess());

	FIntPoint MinimumGrid(MAX_int32, MAX_int32);
	FIntPoint MaximumGrid(MIN_int32, MIN_int32);
	for (const FName SkillKey : SkillKeys)
	{
		const FIntPoint GridPosition = SkillGridPositions.FindChecked(SkillKey);
		MinimumGrid.X = FMath::Min(MinimumGrid.X, GridPosition.X);
		MinimumGrid.Y = FMath::Min(MinimumGrid.Y, GridPosition.Y);
		MaximumGrid.X = FMath::Max(MaximumGrid.X, GridPosition.X);
		MaximumGrid.Y = FMath::Max(MaximumGrid.Y, GridPosition.Y);
	}

	const float ContentWidth = ((MaximumGrid.X - MinimumGrid.X) * SkillGridStepX) + SkillNodeSize + (SkillTreePadding * 2.f);
	const float ContentHeight = ((MaximumGrid.Y - MinimumGrid.Y) * SkillGridStepY) + SkillNodeSize + (SkillTreePadding * 2.f);
	if (UCanvasPanelSlot* TreeSlot = Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot))
	{
		TreeSlot->SetPosition(FVector2D::ZeroVector);
		TreeSlot->SetSize(FVector2D(ContentWidth, ContentHeight));
	}

	TMap<FName, FVector2D> NodePositions;
	for (const FName SkillKey : SkillKeys)
	{
		const FIntPoint GridPosition = SkillGridPositions.FindChecked(SkillKey);
		NodePositions.Add(SkillKey, FVector2D(
			SkillTreePadding + ((GridPosition.X - MinimumGrid.X) * SkillGridStepX),
			SkillTreePadding + ((GridPosition.Y - MinimumGrid.Y) * SkillGridStepY)));
	}

	for (const FName& SkillKey : SkillKeys)
	{
		const FSkillDataTableRow* SkillData = DataManager->GetSkillDataTableRow(SkillKey);
		const FVector2D* TargetPosition = NodePositions.Find(SkillKey);
		if (SkillData == nullptr || TargetPosition == nullptr)
		{
			continue;
		}
		for (const FName& RequiredSkillKey : SkillData->RequiredSkill)
		{
			if (const FVector2D* RequiredPosition = NodePositions.Find(RequiredSkillKey))
			{
				AddSkillConnection(*RequiredPosition, *TargetPosition);
			}
		}
	}

	for (const FName& SkillKey : SkillKeys)
	{
		const FVector2D* NodePosition = NodePositions.Find(SkillKey);
		USkillSlotWidget* SkillNode = CreateWidget<USkillSlotWidget>(this, SkillSlotWidgetClass);
		if (NodePosition == nullptr || ::IsValid(SkillNode) == false)
		{
			continue;
		}
		UCanvasPanelSlot* NodeSlot = SkillTreeCanvasPanel->AddChildToCanvas(SkillNode);
		NodeSlot->SetPosition(*NodePosition);
		NodeSlot->SetSize(FVector2D(SkillNodeSize, SkillNodeSize));
		NodeSlot->SetZOrder(2);
		SkillNode->InitializeSkill(SkillKey);
	}

	UMenuDisplayCardWidget* PlayerCard = CreateWidget<UMenuDisplayCardWidget>(this);
	PlayerCard->SetDisplayRole(TEXT("Player"));
	UCanvasPanelSlot* PlayerSlot = SkillTreeCanvasPanel->AddChildToCanvas(PlayerCard);
	PlayerSlot->SetPosition(FVector2D(SkillTreePadding + (10 - MinimumGrid.X) * SkillGridStepX + SkillNodeSize * 0.5f - 60.f,
		SkillTreePadding + (10 - MinimumGrid.Y) * SkillGridStepY + SkillNodeSize * 0.5f - 84.f));
	PlayerSlot->SetSize(FVector2D(120.f, 168.f));
	PlayerSlot->SetZOrder(2);
	CentralPlayerPosition = PlayerSlot->GetPosition() + FVector2D(60.f, 84.f);
	bNeedsCentering = true;
	CurrentZoomScale = 0.85f;
	SkillTreeCanvasPanel->SetRenderScale(FVector2D(CurrentZoomScale));
}

void USkillTreeMenuContents::AddSkillConnection(const FVector2D& FromPosition, const FVector2D& ToPosition)
{
	const FVector2D ConnectionDirection = ToPosition - FromPosition;
	const bool bHorizontal = FMath::Abs(ConnectionDirection.X) >= FMath::Abs(ConnectionDirection.Y);
	const FVector2D Axis = bHorizontal
		? (FMath::IsNearlyZero(ConnectionDirection.Y) ? FVector2D(FMath::Sign(ConnectionDirection.X), 0.f) : FVector2D(0.f, FMath::Sign(ConnectionDirection.Y)))
		: (FMath::IsNearlyZero(ConnectionDirection.X) ? FVector2D(0.f, FMath::Sign(ConnectionDirection.Y)) : FVector2D(FMath::Sign(ConnectionDirection.X), 0.f));
	UImage* Arrow = WidgetTree->ConstructWidget<UImage>();
	const UDataManager* Data = UDataManager::Get(this);
	const UVisualStyleDataAsset* Style = ::IsValid(Data) ? Data->GetVisualStyleDataAsset() : nullptr;
	if (::IsValid(Style))
	{
		if (const FSlateBrush* Brush = Style->Brushes.Find(TEXT("ArrowRight"))) { Arrow->SetBrush(*Brush); }
	}
	Arrow->SetVisibility(ESlateVisibility::HitTestInvisible);
	Arrow->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Axis.Y, Axis.X)));
	UCanvasPanelSlot* ArrowSlot = SkillTreeCanvasPanel->AddChildToCanvas(Arrow);
	ArrowSlot->SetSize(FVector2D(20.f));
	ArrowSlot->SetPosition((FromPosition + ToPosition) * 0.5f + FVector2D(SkillNodeSize * 0.5f) - FVector2D(10.f));
	ArrowSlot->SetZOrder(1);
	const FVector2D FromCenter = FromPosition + FVector2D(SkillNodeSize * 0.5f);
	const FVector2D ToCenter = ToPosition + FVector2D(SkillNodeSize * 0.5f);
	const FVector2D Delta = ToCenter - FromCenter;
	if (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y))
	{
		const float Direction = FMath::Sign(Delta.X);
		const FVector2D Start(FromCenter.X + (Direction * SkillNodeSize * 0.5f), FromCenter.Y);
		const FVector2D End(ToCenter.X - (Direction * SkillNodeSize * 0.5f), ToCenter.Y);
		const float MiddleX = (Start.X + End.X) * 0.5f;
		AddConnectionSegment(FVector2D(FMath::Min(Start.X, MiddleX), Start.Y - (SkillConnectionThickness * 0.5f)),
			FVector2D(FMath::Max(SkillConnectionThickness, FMath::Abs(MiddleX - Start.X)), SkillConnectionThickness));
		AddConnectionSegment(FVector2D(MiddleX - (SkillConnectionThickness * 0.5f), FMath::Min(Start.Y, End.Y)),
			FVector2D(SkillConnectionThickness, FMath::Max(SkillConnectionThickness, FMath::Abs(End.Y - Start.Y))));
		AddConnectionSegment(FVector2D(FMath::Min(MiddleX, End.X), End.Y - (SkillConnectionThickness * 0.5f)),
			FVector2D(FMath::Max(SkillConnectionThickness, FMath::Abs(End.X - MiddleX)), SkillConnectionThickness));
		return;
	}

	const float Direction = FMath::Sign(Delta.Y);
	const FVector2D Start(FromCenter.X, FromCenter.Y + (Direction * SkillNodeSize * 0.5f));
	const FVector2D End(ToCenter.X, ToCenter.Y - (Direction * SkillNodeSize * 0.5f));
	const float MiddleY = (Start.Y + End.Y) * 0.5f;
	AddConnectionSegment(FVector2D(Start.X - (SkillConnectionThickness * 0.5f), FMath::Min(Start.Y, MiddleY)),
		FVector2D(SkillConnectionThickness, FMath::Max(SkillConnectionThickness, FMath::Abs(MiddleY - Start.Y))));
	AddConnectionSegment(FVector2D(FMath::Min(Start.X, End.X), MiddleY - (SkillConnectionThickness * 0.5f)),
		FVector2D(FMath::Max(SkillConnectionThickness, FMath::Abs(End.X - Start.X)), SkillConnectionThickness));
	AddConnectionSegment(FVector2D(End.X - (SkillConnectionThickness * 0.5f), FMath::Min(MiddleY, End.Y)),
		FVector2D(SkillConnectionThickness, FMath::Max(SkillConnectionThickness, FMath::Abs(End.Y - MiddleY))));
}

void USkillTreeMenuContents::AddConnectionSegment(const FVector2D& Position, const FVector2D& Size)
{
	if (Size.X <= SkillConnectionThickness && Size.Y <= SkillConnectionThickness)
	{
		return;
	}
	UBorder* Segment = WidgetTree->ConstructWidget<UBorder>();
	Segment->SetPadding(FMargin(0.f));
	Segment->SetBrushColor(FLinearColor(0.75f, 0.48f, 0.18f, 0.9f));
	UCanvasPanelSlot* SegmentSlot = SkillTreeCanvasPanel->AddChildToCanvas(Segment);
	SegmentSlot->SetPosition(Position);
	SegmentSlot->SetSize(Size);
}

bool USkillTreeMenuContents::UpdateSkillTreeZoom(const FGeometry& InGeometry, const FVector2D& ScreenSpacePosition, const float WheelDelta)
{
	UCanvasPanelSlot* CanvasPanelSlot = ::IsValid(SkillTreeCanvasPanel) ? Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot) : nullptr;
	if (::IsValid(CanvasPanelSlot) == false || FMath::IsNearlyZero(WheelDelta) || ZoomStep <= 0.f)
	{
		return false;
	}

	const float MinimumZoomScale = CalculateMinimumZoomScale();
	const float NewZoomScale = FMath::Clamp(CurrentZoomScale + (WheelDelta * ZoomStep), MinimumZoomScale, 1.f);
	if (FMath::IsNearlyEqual(NewZoomScale, CurrentZoomScale))
	{
		return true;
	}

	const FGeometry& ViewGeometry = ::IsValid(SkillTreeViewportPanel) ? SkillTreeViewportPanel->GetCachedGeometry() : InGeometry;
	const FVector2D CursorPosition = ViewGeometry.AbsoluteToLocal(ScreenSpacePosition);
	const FVector2D CurrentContentPosition = ViewGeometry.AbsoluteToLocal(SkillTreeCanvasPanel->GetCachedGeometry().GetAbsolutePosition());
	const FVector2D ContentPositionUnderCursor = (CursorPosition - CurrentContentPosition) / CurrentZoomScale;
	const FVector2D TargetContentPosition = CursorPosition - (ContentPositionUnderCursor * NewZoomScale);
	const FVector2D TargetCanvasPosition = CanvasPanelSlot->GetPosition() + (TargetContentPosition - CurrentContentPosition);

	SkillTreeCanvasPanel->SetRenderTransformPivot(FVector2D::ZeroVector);
	SkillTreeCanvasPanel->SetRenderScale(FVector2D(NewZoomScale));
	CurrentZoomScale = NewZoomScale;
	CanvasPanelSlot->SetPosition(CalculateClampedCanvasPosition(TargetCanvasPosition, CurrentZoomScale));

	if (bIsDraggingSkillTree)
	{
		DragStartScreenPosition = ScreenSpacePosition;
		DragStartCanvasPosition = CanvasPanelSlot->GetPosition();
	}

	return true;
}

float USkillTreeMenuContents::CalculateMinimumZoomScale() const
{
	if (::IsValid(SkillTreeCanvasPanel) == false)
	{
		return 1.f;
	}

	const FVector2D ViewSize = ::IsValid(SkillTreeViewportPanel)
		? SkillTreeViewportPanel->GetCachedGeometry().GetLocalSize() : GetCachedGeometry().GetLocalSize();
	FVector2D ContentSize = SkillTreeCanvasPanel->GetCachedGeometry().GetLocalSize();
	if (ContentSize.IsNearlyZero())
	{
		ContentSize = SkillTreeCanvasPanel->GetDesiredSize();
	}

	if (ContentSize.IsNearlyZero())
	{
		const UCanvasPanelSlot* CanvasPanelSlot = Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot);
		ContentSize = ::IsValid(CanvasPanelSlot) ? CanvasPanelSlot->GetSize() : FVector2D::ZeroVector;
	}

	if (ViewSize.IsNearlyZero() || ContentSize.IsNearlyZero())
	{
		return 1.f;
	}

	const float FitScale = FMath::Min(ViewSize.X / ContentSize.X, ViewSize.Y / ContentSize.Y);
	return FMath::Clamp(FitScale, UE_KINDA_SMALL_NUMBER, 1.f);
}

FVector2D USkillTreeMenuContents::CalculateClampedCanvasPosition(const FVector2D& TargetPosition, const float ContentScale) const
{
	UCanvasPanelSlot* CanvasPanelSlot = ::IsValid(SkillTreeCanvasPanel) ? Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot) : nullptr;
	if (::IsValid(SkillTreeCanvasPanel) == false || ::IsValid(CanvasPanelSlot) == false)
	{
		return TargetPosition;
	}

	const FVector2D ViewSize = ::IsValid(SkillTreeViewportPanel)
		? SkillTreeViewportPanel->GetCachedGeometry().GetLocalSize() : GetCachedGeometry().GetLocalSize();
	FVector2D ContentSize = SkillTreeCanvasPanel->GetCachedGeometry().GetLocalSize();
	if (ContentSize.IsNearlyZero())
	{
		ContentSize = SkillTreeCanvasPanel->GetDesiredSize();
	}

	if (ContentSize.IsNearlyZero())
	{
		ContentSize = CanvasPanelSlot->GetSize();
	}
	ContentSize *= ContentScale;

	const FVector2D CurrentPosition = CanvasPanelSlot->GetPosition();
	const FVector2D PositionDelta = TargetPosition - CurrentPosition;
	const FGeometry& ViewGeometry = ::IsValid(SkillTreeViewportPanel) ? SkillTreeViewportPanel->GetCachedGeometry() : GetCachedGeometry();
	const FVector2D CurrentContentPosition = ViewGeometry.AbsoluteToLocal(SkillTreeCanvasPanel->GetCachedGeometry().GetAbsolutePosition());
	const FVector2D TargetContentPosition = CurrentContentPosition + PositionDelta;

	FVector2D MinContentPosition = CurrentContentPosition;
	FVector2D MaxContentPosition = CurrentContentPosition;

	if (ContentSize.X > ViewSize.X)
	{
		MinContentPosition.X = ViewSize.X - ContentSize.X;
		MaxContentPosition.X = 0.f;
	}
	else
	{
		MinContentPosition.X = (ViewSize.X - ContentSize.X) * 0.5f;
		MaxContentPosition.X = MinContentPosition.X;
	}

	if (ContentSize.Y > ViewSize.Y)
	{
		MinContentPosition.Y = ViewSize.Y - ContentSize.Y;
		MaxContentPosition.Y = 0.f;
	}
	else
	{
		MinContentPosition.Y = (ViewSize.Y - ContentSize.Y) * 0.5f;
		MaxContentPosition.Y = MinContentPosition.Y;
	}

	const FVector2D ClampedContentPosition(
		FMath::Clamp(TargetContentPosition.X, MinContentPosition.X, MaxContentPosition.X),
		FMath::Clamp(TargetContentPosition.Y, MinContentPosition.Y, MaxContentPosition.Y)
	);

	return TargetPosition + (ClampedContentPosition - TargetContentPosition);
}
