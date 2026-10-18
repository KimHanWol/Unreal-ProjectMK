#include "ProjectMK/UI/MinimapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElementTypes.h"
#include "TimerManager.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"

TSharedRef<SWidget> UMinimapWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultWidgetTree();
	}
	return Super::RebuildWidget();
}

void UMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	GetWorld()->GetTimerManager().SetTimer(MinimapTimerHandle, this, &UMinimapWidget::UpdateMinimap, FMath::Max(0.05f, UpdateInterval), true);
}

void UMinimapWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MinimapTimerHandle);
	}
	Super::NativeDestruct();
}

int32 UMinimapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (::IsValid(MapArea) == false || MapBounds.bIsValid == false)
	{
		return BaseLayer;
	}
	const FGeometry& MapGeometry = MapArea->GetPaintSpaceGeometry();
	const FVector2D AreaSize = MapGeometry.GetLocalSize();
	const FVector2D BoundsSize = MapBounds.GetSize();
	if (AreaSize.X <= 8.f || AreaSize.Y <= 8.f || BoundsSize.X <= 0.f || BoundsSize.Y <= 0.f)
	{
		return BaseLayer;
	}
	const double Scale = FMath::Min((AreaSize.X - 8.f) / BoundsSize.X, (AreaSize.Y - 8.f) / BoundsSize.Y);
	const FVector2D Origin = (AreaSize - BoundsSize * Scale) * 0.5f;
	const FVector2D CellSize(FMath::Max(1.0, TileWorldSize * Scale));
	static const FSlateColorBrush PixelBrush(FLinearColor::White);
	const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
	OutDrawElements.PushClip(FSlateClippingZone(MapGeometry));
	for (const FMinimapTile& Tile : Tiles)
	{
		const FVector2D Position = Origin + (Tile.Position - MapBounds.Min - FVector2D(TileWorldSize * 0.5f)) * Scale;
		FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 1,
			MapGeometry.ToPaintGeometry(CellSize, FSlateLayoutTransform(Position)), &PixelBrush, ESlateDrawEffect::None, Tile.Color * Tint);
	}
	for (const FVector2D& ShopPosition : ShopPositions)
	{
		if (MapBounds.IsInside(ShopPosition) == false)
		{
			continue;
		}
		const FVector2D Position = Origin + (ShopPosition - MapBounds.Min) * Scale;
		FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 2,
			MapGeometry.ToPaintGeometry(FVector2D(6.f), FSlateLayoutTransform(Position - FVector2D(3.f))),
			&PixelBrush, ESlateDrawEffect::None, FLinearColor(1.f, 0.7f, 0.16f) * Tint);
	}
	if (bHasPlayer && MapBounds.IsInside(PlayerPosition))
	{
		const FVector2D Position = Origin + (PlayerPosition - MapBounds.Min) * Scale;
		FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 3,
			MapGeometry.ToPaintGeometry(FVector2D(8.f), FSlateLayoutTransform(Position - FVector2D(4.f))),
			&PixelBrush, ESlateDrawEffect::None, FLinearColor(0.01f, 0.04f, 0.06f) * Tint);
		FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 4,
			MapGeometry.ToPaintGeometry(FVector2D(4.f), FSlateLayoutTransform(Position - FVector2D(2.f))),
			&PixelBrush, ESlateDrawEffect::None, FLinearColor(0.2f, 1.f, 0.95f) * Tint);
	}
	OutDrawElements.PopClip();
	return BaseLayer + 4;
}

void UMinimapWidget::UpdateWidget()
{
	UpdateMinimap();
}

void UMinimapWidget::BuildDefaultWidgetTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
	Frame->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.065f, 0.95f));
	Frame->SetPadding(FMargin(10.f));
	UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame);
	FrameSlot->SetAnchors(FAnchors(1.f, 0.f));
	FrameSlot->SetAlignment(FVector2D(1.f, 0.f));
	FrameSlot->SetPosition(ViewportOffset);
	FrameSlot->SetAutoSize(true);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Column);
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
	Title->SetText(NSLOCTEXT("Minimap", "Title", "미니맵 · 주변 지형"));
	FSlateFontInfo Font = Title->GetFont();
	Font.Size = 13;
	Title->SetFont(Font);
	Column->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	MapArea = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MapArea"));
	MapArea->SetWidthOverride(MinimapSize.X);
	MapArea->SetHeightOverride(MinimapSize.Y);
	Column->AddChildToVerticalBox(MapArea);
	UBorder* MapBackground = WidgetTree->ConstructWidget<UBorder>();
	MapBackground->SetBrushColor(FLinearColor(0.008f, 0.015f, 0.025f, 1.f));
	MapArea->SetContent(MapBackground);
	UTextBlock* Legend = WidgetTree->ConstructWidget<UTextBlock>();
	Legend->SetText(NSLOCTEXT("Minimap", "Legend", "청록: 나  ·  금색: 상점"));
	Font.Size = 11;
	Legend->SetFont(Font);
	Legend->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.85f, 0.9f)));
	Column->AddChildToVerticalBox(Legend)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	PositionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PositionText"));
	PositionText->SetFont(Font);
	Column->AddChildToVerticalBox(PositionText);
}

void UMinimapWidget::UpdateMinimap()
{
	const UWorld* World = GetWorld();
	ULevelManagerSubsystem* Level = ::IsValid(World) ? World->GetSubsystem<ULevelManagerSubsystem>() : nullptr;
	const UDataManager* Data = UDataManager::Get(this);
	if (::IsValid(Level) == false || ::IsValid(Data) == false)
	{
		return;
	}
	Tiles.Reset();
	ShopPositions.Reset();
	MapBounds = FBox2D(ForceInit);
	if (CalculateCameraBounds(MapBounds) == false)
	{
		InvalidateLayoutAndVolatility();
		return;
	}
	TileWorldSize = FMath::Max(1, Level->GetTileSize());
	const FBox2D TileBounds = MapBounds.ExpandBy(TileWorldSize * 0.5f);
	const FBlockDataTableRow* ShopRow = Data->GetBlockDataTableRow(FName(TEXT("Shop")));
	const int32 ShopIndex = ShopRow != nullptr ? ShopRow->TileIndex : INDEX_NONE;
	TMap<int32, FLinearColor> TileColors;
	TSet<int32> HiddenTileIndices;
	for (ABlockBase* Block : Level->GetBlocks())
	{
		if (::IsValid(Block) == false || Block->IsBreaking())
		{
			continue;
		}
		const FVector Location = Block->GetActorLocation();
		const FVector2D MapPosition(Location.X, -Location.Z);
		if (TileBounds.IsInside(MapPosition) == false)
		{
			continue;
		}
		const int32 TileIndex = Block->GetTileIndex();
		if (HiddenTileIndices.Contains(TileIndex))
		{
			continue;
		}
		FLinearColor* Color = TileColors.Find(TileIndex);
		if (Color == nullptr)
		{
			const FBlockDataTableRow* Row = Data->GetBlockDataTableRow(TileIndex);
			if (Row != nullptr && Row->bNeedTobeHide)
			{
				HiddenTileIndices.Add(TileIndex);
				continue;
			}
			const FLinearColor TileColor = Row != nullptr && Row->bIsSupportAnchor
				? FLinearColor(0.24f, 0.6f, 0.45f)
				: (Row != nullptr && Row->bIsMineable ? FLinearColor(0.5f, 0.32f, 0.16f) : FLinearColor(0.3f, 0.36f, 0.43f));
			Color = &TileColors.Add(TileIndex, TileColor);
		}
		FMinimapTile& Tile = Tiles.AddDefaulted_GetRef();
		Tile.Position = MapPosition;
		Tile.Color = Block->IsCushioned() ? FLinearColor(0.45f, 0.9f, 0.5f) : *Color;
		if (TileIndex == ShopIndex)
		{
			ShopPositions.Add(MapPosition);
		}
	}
	const AMKCharacter* Character = GetLocalPlayerCharacter();
	bHasPlayer = ::IsValid(Character);
	if (bHasPlayer)
	{
		const FVector Location = Character->GetActorLocation();
		PlayerPosition = FVector2D(Location.X, -Location.Z);
		if (::IsValid(PositionText))
		{
			const FVector2D BlockPosition = UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(Location);
			const UGameSettingDataAsset* Settings = Data->GetGameSettingDataAsset();
			const int32 Depth = FMath::Max(0, FMath::RoundToInt(BlockPosition.Y) - (::IsValid(Settings) ? Settings->SurfaceBlockPositionY : 0));
			PositionText->SetText(FText::Format(NSLOCTEXT("Minimap", "Position", "X {0}  ·  깊이 {1}칸"), FMath::RoundToInt(BlockPosition.X), Depth));
		}
	}
	InvalidateLayoutAndVolatility();
}

bool UMinimapWidget::CalculateCameraBounds(FBox2D& OutBounds) const
{
	const APlayerController* Controller = GetOwningPlayer();
	const AMKCharacter* Character = GetLocalPlayerCharacter();
	if (::IsValid(Controller) == false || ::IsValid(Character) == false)
	{
		return false;
	}
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	Controller->GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return false;
	}
	const FVector2D ScreenCorners[] = {
		FVector2D::ZeroVector, FVector2D(ViewportWidth, 0),
		FVector2D(0, ViewportHeight), FVector2D(ViewportWidth, ViewportHeight)
	};
	FBox2D CameraBounds(ForceInit);
	const double GameplayPlaneY = Character->GetActorLocation().Y;
	for (const FVector2D& Corner : ScreenCorners)
	{
		FVector RayOrigin;
		FVector RayDirection;
		if (Controller->DeprojectScreenPositionToWorld(Corner.X, Corner.Y, RayOrigin, RayDirection) == false
			|| FMath::IsNearlyZero(RayDirection.Y))
		{
			return false;
		}
		const double Distance = (GameplayPlaneY - RayOrigin.Y) / RayDirection.Y;
		if (Distance < 0.0)
		{
			return false;
		}
		const FVector Position = RayOrigin + RayDirection * Distance;
		CameraBounds += FVector2D(Position.X, -Position.Z);
	}
	const FVector2D Extent = CameraBounds.GetExtent() * FMath::Max(1.f, CameraRangeMultiplier);
	if (Extent.X <= 0.0 || Extent.Y <= 0.0)
	{
		return false;
	}
	OutBounds = FBox2D(CameraBounds.GetCenter() - Extent, CameraBounds.GetCenter() + Extent);
	return true;
}
