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
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "SceneView.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElementTypes.h"
#include "TimerManager.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"
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
	const UDataManager* DataManager = UDataManager::Get(this);
	const UVisualStyleDataAsset* Style = ::IsValid(DataManager) ? DataManager->GetVisualStyleDataAsset() : nullptr;
	if (::IsValid(Style))
	{
		if (const FSlateBrush* Brush = Style->Brushes.Find(TEXT("PlayerMarker")))
		{
			PlayerMarkerBrush = *Brush;
		}
		if (const FSlateBrush* Brush = Style->Brushes.Find(TEXT("ShopMarker")))
		{
			ShopMarkerBrush = *Brush;
		}
	}
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

void UMinimapWidget::UpdateWidget()
{
	UpdateMinimap();
}

void UMinimapWidget::BuildDefaultWidgetTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	USizeBox* Shell = WidgetTree->ConstructWidget<USizeBox>();
	Shell->SetWidthOverride(MinimapSize.X + 12.f);
	Shell->SetHeightOverride(MinimapSize.Y + 12.f);
	UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Shell);
	FrameSlot->SetAnchors(FAnchors(1.f, 0.f));
	FrameSlot->SetAlignment(FVector2D(1.f, 0.f));
	FrameSlot->SetPosition(ViewportOffset);
	FrameSlot->SetAutoSize(true);
	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>();
	Shell->SetContent(Layers);
	MapArea = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MapArea"));
	UOverlaySlot* MapLayout = Layers->AddChildToOverlay(MapArea);
	MapLayout->SetHorizontalAlignment(HAlign_Fill);
	MapLayout->SetVerticalAlignment(VAlign_Fill);
	MapLayout->SetPadding(FMargin(6.f));
	TerrainWidget = CreateWidget<UMinimapTerrainWidget>(this);
	TerrainWidget->SetMapOwner(this);
	MapArea->SetContent(TerrainWidget);
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MinimapFrame"));
	Frame->SetPadding(FMargin(0.f));
	UOverlaySlot* RimLayout = Layers->AddChildToOverlay(Frame);
	RimLayout->SetHorizontalAlignment(HAlign_Fill);
	RimLayout->SetVerticalAlignment(VAlign_Fill);
	PositionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PositionText"));
	ApplyTextRole(PositionText, EMKTextRole::HUDMetric);
	PositionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.97f, 0.86f, 0.57f)));
	PositionText->SetJustification(ETextJustify::Center);
	UOverlaySlot* DepthLayout = Layers->AddChildToOverlay(PositionText);
	DepthLayout->SetHorizontalAlignment(HAlign_Center);
	DepthLayout->SetVerticalAlignment(VAlign_Top);
	DepthLayout->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
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
	if (CalculateCameraBounds(CameraBounds) == false)
	{
		if (::IsValid(TerrainWidget)) { TerrainWidget->InvalidateLayoutAndVolatility(); }
		InvalidateLayoutAndVolatility();
		return;
	}
	FVector2D Extent = CameraBounds.GetExtent() * FMath::Max(1.f, CameraRangeMultiplier);
	const FVector2D Area = ::IsValid(MapArea) && MapArea->GetCachedGeometry().GetLocalSize().X > 8.f
		? MapArea->GetCachedGeometry().GetLocalSize() : MinimapSize;
	const double Aspect = Area.X / FMath::Max(1.0, Area.Y);
	if (Extent.X / Extent.Y < Aspect) { Extent.X = Extent.Y * Aspect; }
	else { Extent.Y = Extent.X / Aspect; }
	MapBounds = FBox2D(CameraBounds.GetCenter() - Extent, CameraBounds.GetCenter() + Extent);
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
			const int32 DepthInMeters = FMath::RoundToInt(static_cast<float>(Depth) * MetersPerBlock);
			PositionText->SetText(FText::Format(NSLOCTEXT("Minimap", "Position", "{0}m"), DepthInMeters));
		}
	}
	if (::IsValid(TerrainWidget)) { TerrainWidget->InvalidateLayoutAndVolatility(); }
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
	FIntRect ViewRect(0, 0, ViewportWidth, ViewportHeight);
	const ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer();
	FSceneViewProjectionData Projection;
	if (::IsValid(LocalPlayer) && LocalPlayer->ViewportClient != nullptr
		&& LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, Projection))
	{
		ViewRect = Projection.GetConstrainedViewRect();
	}
	const FVector2D ScreenCorners[] = {
		FVector2D(ViewRect.Min), FVector2D(ViewRect.Max.X, ViewRect.Min.Y),
		FVector2D(ViewRect.Min.X, ViewRect.Max.Y), FVector2D(ViewRect.Max)
	};
	FBox2D ProjectedBounds(ForceInit);
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
		ProjectedBounds += FVector2D(Position.X, -Position.Z);
	}
	if (ProjectedBounds.GetExtent().X <= 0.0 || ProjectedBounds.GetExtent().Y <= 0.0)
	{
		return false;
	}
	OutBounds = ProjectedBounds;
	return true;
}

int32 UMinimapWidget::PaintTerrain(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = LayerId;
	if (::IsValid(MapArea) == false || MapBounds.bIsValid == false)
	{
		return BaseLayer;
	}
	const FGeometry& MapGeometry = AllottedGeometry;
	const FVector2D AreaSize = MapGeometry.GetLocalSize();
	const FVector2D BoundsSize = MapBounds.GetSize();
	if (AreaSize.X <= 8.f || AreaSize.Y <= 8.f || BoundsSize.X <= 0.f || BoundsSize.Y <= 0.f)
	{
		return BaseLayer;
	}
	const double Scale = FMath::Min(AreaSize.X / BoundsSize.X, AreaSize.Y / BoundsSize.Y);
	const FVector2D Origin = (AreaSize - BoundsSize * Scale) * 0.5f;
	const FVector2D CellSize(FMath::Max(1.0, TileWorldSize * Scale));
	static const FSlateColorBrush PixelBrush(FLinearColor::White);
	const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
	OutDrawElements.PushClip(FSlateClippingZone(MapGeometry));
	FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer,
		MapGeometry.ToPaintGeometry(AreaSize, FSlateLayoutTransform()), &PixelBrush, ESlateDrawEffect::None, FLinearColor(0.008f, 0.015f, 0.025f) * Tint);
	for (const FMinimapTile& Tile : Tiles)
	{
		const FVector2D Position = Origin + (Tile.Position - MapBounds.Min - FVector2D(TileWorldSize * 0.5f)) * Scale;
		FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 1,
			MapGeometry.ToPaintGeometry(CellSize, FSlateLayoutTransform(Position)), &PixelBrush, ESlateDrawEffect::None, Tile.Color * Tint);
	}
	if (CameraBounds.bIsValid)
	{
		const FVector2D Min = Origin + (CameraBounds.Min - MapBounds.Min) * Scale;
		const FVector2D Max = Origin + (CameraBounds.Max - MapBounds.Min) * Scale;
		const TArray<FVector2D> Frame = { Min, FVector2D(Max.X, Min.Y), Max, FVector2D(Min.X, Max.Y), Min };
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 2, MapGeometry.ToPaintGeometry(), Frame,
			ESlateDrawEffect::None, FLinearColor(0.6f, 0.88f, 0.83f, 0.7f) * Tint, false, 1.f);
	}
	for (const FVector2D& ShopPosition : ShopPositions)
	{
		if (MapBounds.IsInside(ShopPosition) == false)
		{
			continue;
		}
		const FVector2D Position = Origin + (ShopPosition - MapBounds.Min) * Scale;
		if (ShopMarkerBrush.GetResourceObject())
		{
			FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 2,
				MapGeometry.ToPaintGeometry(FVector2D(16.f), FSlateLayoutTransform(Position - FVector2D(8.f))),
				&ShopMarkerBrush, ESlateDrawEffect::None, Tint);
			continue;
		}
		FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 2,
			MapGeometry.ToPaintGeometry(FVector2D(6.f), FSlateLayoutTransform(Position - FVector2D(3.f))),
			&PixelBrush, ESlateDrawEffect::None, FLinearColor(1.f, 0.7f, 0.16f) * Tint);
	}
	if (bHasPlayer && MapBounds.IsInside(PlayerPosition))
	{
		const FVector2D Position = Origin + (PlayerPosition - MapBounds.Min) * Scale;
		if (PlayerMarkerBrush.GetResourceObject())
		{
			FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 4,
				MapGeometry.ToPaintGeometry(FVector2D(16.f), FSlateLayoutTransform(Position - FVector2D(8.f))),
				&PlayerMarkerBrush, ESlateDrawEffect::None, Tint);
			OutDrawElements.PopClip();
			return BaseLayer + 4;
		}
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

int32 UMinimapTerrainWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	return MapOwner.IsValid() ? MapOwner->PaintTerrain(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) : LayerId;
}

void UMinimapTerrainWidget::SetMapOwner(UMinimapWidget* Map)
{
	MapOwner = Map;
}
