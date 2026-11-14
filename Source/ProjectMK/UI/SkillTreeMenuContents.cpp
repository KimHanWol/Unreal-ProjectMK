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
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/UI/SkillSlotWidget.h"
#include "ProjectMK/UI/SkillConnectionWidget.h"
#include "ProjectMK/UI/SkillTreeLayout.h"

namespace
{
	constexpr float SkillNodeSize = 84.f;
	constexpr float SkillTreePadding = 128.f;

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
	if (::IsValid(SkillTreeViewportPanel))
	{
		const FVector2D ViewSize = SkillTreeViewportPanel->GetCachedGeometry().GetLocalSize();
		if (ViewSize.X > 100.f && ViewSize.Y > 100.f && (bNeedsCentering || ViewSize.Equals(LastViewportSize) == false))
		{
			SkillTreeCanvasPanel->SetRenderTransformPivot(FVector2D::ZeroVector);
			SkillTreeCanvasPanel->SetRenderScale(FVector2D(CurrentZoomScale));
			CastChecked<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot)->SetPosition(CalculateClampedCanvasPosition(ViewSize * 0.5f - CentralPlayerPosition * CurrentZoomScale, CurrentZoomScale));
			bNeedsCentering = false;
			LastViewportSize = ViewSize;
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

void USkillTreeMenuContents::NativeOnMouseCaptureLost(const FCaptureLostEvent& Event)
{
	Super::NativeOnMouseCaptureLost(Event);
	EndSkillTreeDrag();
}

void USkillTreeMenuContents::BindEvents()
{
	Super::BindEvents();
	OnVisibilityChanged.AddUniqueDynamic(this, &USkillTreeMenuContents::OnMenuVisibilityChanged);
	if (USkillComponent* Skills = GetLocalSkillComponent())
	{
		Skills->OnSkillPurchasedDelegate.AddUObject(this, &USkillTreeMenuContents::OnSkillPurchased);
	}
}

void USkillTreeMenuContents::UnbindEvents()
{
	OnVisibilityChanged.RemoveDynamic(this, &USkillTreeMenuContents::OnMenuVisibilityChanged);
	if (USkillComponent* Skills = GetLocalSkillComponent()) { Skills->OnSkillPurchasedDelegate.RemoveAll(this); }
	Super::UnbindEvents();
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
	Connections.Reset();
	TArray<FName> SkillKeys = DataManager->GetSkillRowNames();
	SkillKeys.RemoveAll([DataManager](const FName SkillKey)
	{
		const FSkillDataTableRow* SkillData = DataManager->GetSkillDataTableRow(SkillKey);
		return SkillData == nullptr || SkillData->AcquisitionType != ESkillAcquisitionType::SkillTree
			|| SkillData->DisplayName.IsEmpty();
	});
	SkillKeys.Sort(FNameLexicalLess());

	TMap<FName, TArray<FName>> Prerequisites;
	for (FName Key : SkillKeys) { Prerequisites.Add(Key, DataManager->GetSkillDataTableRow(Key)->RequiredSkill); }
	FSkillTreeLayout Layout;
	if (FSkillTreeLayout::Calculate(Prerequisites, Layout) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to lay out skill prerequisites without overlapping nodes or connections."));
		return;
	}
	// Keep enough room on both sides of the player for the initial centered view.
	const FVector2D Extent(FMath::Max(FMath::Abs(Layout.Bounds.Min.X), FMath::Abs(Layout.Bounds.Max.X)),
		FMath::Max(FMath::Abs(Layout.Bounds.Min.Y), FMath::Abs(Layout.Bounds.Max.Y)));
	Layout.Bounds = FBox2D(-Extent, Extent);
	const FVector2D Offset = FVector2D(SkillTreePadding) - Layout.Bounds.Min;
	const FVector2D ContentSize = Layout.Bounds.GetSize() + FVector2D(SkillTreePadding * 2.f);
	if (UCanvasPanelSlot* TreeSlot = Cast<UCanvasPanelSlot>(SkillTreeCanvasPanel->Slot))
	{
		TreeSlot->SetPosition(FVector2D::ZeroVector);
		TreeSlot->SetSize(ContentSize);
	}
	TMap<FName, FVector2D> NodePositions = Layout.Positions;
	for (auto& Pair : NodePositions) { Pair.Value += Offset; }
	const FVector2D PlayerNodePosition = NodePositions.FindChecked(NAME_None);
	for (FSkillTreeLayoutEdge& Edge : Layout.Edges)
	{
		for (FVector2D& Point : Edge.Points) { Point += Offset; }
		AddSkillConnection(Edge.Points, Edge.Source, Edge.Target);
	}
	UpdateConnectionStates();

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
		NodeSlot->SetZOrder(10);
		SkillNode->InitializeSkill(SkillKey);
	}

	UMenuDisplayCardWidget* PlayerCard = CreateWidget<UMenuDisplayCardWidget>(this);
	PlayerCard->SetDisplayRole(TEXT("Player"));
	UCanvasPanelSlot* PlayerSlot = SkillTreeCanvasPanel->AddChildToCanvas(PlayerCard);
	PlayerSlot->SetPosition(PlayerNodePosition + FVector2D(SkillNodeSize * 0.5f) - FVector2D(60.f, 84.f));
	PlayerSlot->SetSize(FVector2D(120.f, 168.f));
	PlayerSlot->SetZOrder(10);
	CentralPlayerPosition = PlayerSlot->GetPosition() + FVector2D(60.f, 84.f);
	bNeedsCentering = true;
	CurrentZoomScale = 0.75f;
	SkillTreeCanvasPanel->SetRenderScale(FVector2D(CurrentZoomScale));
}

void USkillTreeMenuContents::AddSkillConnection(const TArray<FVector2D>& RoutePoints, FName SourceSkillKey, FName TargetSkillKey)
{
	TArray<FVector2D> Points = RoutePoints;
	if (Points.Num() < 2) { return; }
	FBox2D Bounds(ForceInit);
	float PathLength = 0.f;
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		Bounds += Points[Index];
		if (Index > 0) { PathLength += FVector2D::Distance(Points[Index - 1], Points[Index]); }
	}
	FVector2D ArrowPosition = Points[0];
	FVector2D ArrowDirection = FVector2D(1.f, 0.f);
	float Remaining = PathLength * 0.5f;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		const FVector2D Segment = Points[Index] - Points[Index - 1];
		if (Remaining <= Segment.Size())
		{
			ArrowDirection = Segment.GetSafeNormal();
			ArrowPosition = Points[Index - 1] + ArrowDirection * Remaining;
			break;
		}
		Remaining -= Segment.Size();
	}
	FSkillConnectionVisual& Connection = Connections.AddDefaulted_GetRef();
	Connection.TargetSkillKey = TargetSkillKey;
	Connection.SourceSkillKey = SourceSkillKey;
	Connection.RoutePoints = Points;
	UImage* Arrow = WidgetTree->ConstructWidget<UImage>();
	Connection.Elements.Add(Arrow);
	const UDataManager* Data = UDataManager::Get(this);
	const UVisualStyleDataAsset* Style = ::IsValid(Data) ? Data->GetVisualStyleDataAsset() : nullptr;
	if (::IsValid(Style))
	{
		if (const FSlateBrush* Brush = Style->Brushes.Find(TEXT("ArrowRight"))) { Arrow->SetBrush(*Brush); }
	}
	Arrow->SetVisibility(ESlateVisibility::HitTestInvisible);
	Arrow->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(ArrowDirection.Y, ArrowDirection.X)));
	UCanvasPanelSlot* ArrowSlot = SkillTreeCanvasPanel->AddChildToCanvas(Arrow);
	ArrowSlot->SetSize(FVector2D(18.f));
	ArrowSlot->SetPosition(ArrowPosition - FVector2D(9.f));
	USkillConnectionWidget* Rail = CreateWidget<USkillConnectionWidget>(this);
	Connection.Elements.Add(Rail);
	const FVector2D Origin = Bounds.Min - FVector2D(8.f);
	for (FVector2D& Point : Points) { Point -= Origin; }
	Rail->InitializePath(Points);
	UCanvasPanelSlot* RailSlot = SkillTreeCanvasPanel->AddChildToCanvas(Rail);
	RailSlot->SetPosition(Origin);
	RailSlot->SetSize(Bounds.GetSize() + FVector2D(16.f));
}

void USkillTreeMenuContents::UpdateConnectionStates()
{
	const USkillComponent* Skills = GetLocalSkillComponent();
	const UDataManager* Data = UDataManager::Get(this);
	if (::IsValid(Skills) == false || ::IsValid(Data) == false) { return; }
	for (const FSkillConnectionVisual& Connection : Connections)
	{
		const FSkillDataTableRow* Target = Data->GetSkillDataTableRow(Connection.TargetSkillKey);
		bool bAvailable = Target != nullptr;
		if (Target != nullptr)
		{
			for (FName Required : Target->RequiredSkill) { bAvailable &= Skills->HasPurchasedSkill(Required); }
		}
		const bool bPurchased = Skills->HasPurchasedSkill(Connection.TargetSkillKey);
		const FLinearColor Color = bPurchased ? FLinearColor(0.35f, 0.9f, 0.68f)
			: (bAvailable ? FLinearColor(0.75f, 0.48f, 0.18f) : FLinearColor(0.22f, 0.22f, 0.18f));
		const int32 LineLayer = bPurchased ? 4 : (bAvailable ? 3 : 2);
		for (UWidget* Element : Connection.Elements)
		{
			UCanvasPanelSlot* Layout = CastChecked<UCanvasPanelSlot>(Element->Slot);
			if (USkillConnectionWidget* Rail = Cast<USkillConnectionWidget>(Element))
			{
				Rail->SetLineColor(Color);
				Layout->SetZOrder(LineLayer);
			}
			if (UImage* Arrow = Cast<UImage>(Element))
			{
				Arrow->SetColorAndOpacity(Color);
				Layout->SetZOrder(8);
			}
		}
	}
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

void USkillTreeMenuContents::OnSkillPurchased(FName SkillKey)
{
	UpdateConnectionStates();
}

void USkillTreeMenuContents::OnMenuVisibilityChanged(ESlateVisibility InVisibility)
{
	if (InVisibility != ESlateVisibility::Collapsed && InVisibility != ESlateVisibility::Hidden) { UpdateConnectionStates(); }
}
