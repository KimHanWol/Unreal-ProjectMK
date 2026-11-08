#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "MinimapWidget.generated.h"

class USizeBox;
class UTextBlock;
class UMinimapTerrainWidget;

UCLASS()
class PROJECTMK_API UMinimapWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void UpdateWidget() override;

private:
	friend class UMinimapTerrainWidget;
	void BuildDefaultWidgetTree();
	void UpdateMinimap();
	bool CalculateCameraBounds(FBox2D& OutBounds) const;
	int32 PaintTerrain(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	FVector2D MinimapSize = FVector2D(240.f, 160.f);

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	FVector2D ViewportOffset = FVector2D(-12.f, 112.f);

	UPROPERTY(EditDefaultsOnly, Category = "Minimap", meta = (ClampMin = "1.0"))
	float CameraRangeMultiplier = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Minimap", meta = (ClampMin = "0.05"))
	float UpdateInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Minimap", meta = (ClampMin = "0.01"))
	float MetersPerBlock = 1.f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> MapArea;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PositionText;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMinimapTerrainWidget> TerrainWidget;
	UPROPERTY(Transient)
	FSlateBrush PlayerMarkerBrush;

	UPROPERTY(Transient)
	FSlateBrush ShopMarkerBrush;

	struct FMinimapTile
	{
		FVector2D Position = FVector2D::ZeroVector;
		FLinearColor Color = FLinearColor::White;
	};

	TArray<FMinimapTile> Tiles;
	TArray<FVector2D> ShopPositions;
	FBox2D MapBounds = FBox2D(ForceInit);
	FVector2D PlayerPosition = FVector2D::ZeroVector;
	float TileWorldSize = 16.f;
	bool bHasPlayer = false;
	FTimerHandle MinimapTimerHandle;
};

UCLASS()
class PROJECTMK_API UMinimapTerrainWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

public:
	void SetMapOwner(UMinimapWidget* Map);

private:
	TWeakObjectPtr<UMinimapWidget> MapOwner;
};
