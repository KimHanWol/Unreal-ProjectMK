#include "ProjectMK/Helper/MKRuntimePaperSprite.h"

#include "Engine/Texture2D.h"
#include "SpriteEditorOnlyTypes.h"

void UMKRuntimePaperSprite::InitializeFromAtlasCell(UTexture2D* AtlasTexture, const FIntPoint& CellOrigin, const FIntPoint& CellSize, float InPixelsPerUnrealUnit)
{
    if (AtlasTexture == nullptr || CellSize.X <= 0 || CellSize.Y <= 0)
    {
        return;
    }

    FSpriteAssetInitParameters InitParams;
    InitParams.Texture = AtlasTexture;
    InitParams.Offset = CellOrigin;
    InitParams.Dimension = CellSize;
    InitParams.SetPixelsPerUnrealUnit(FMath::Max(InPixelsPerUnrealUnit, KINDA_SMALL_NUMBER));

    InitializeSprite(InitParams, false);

    AdditionalSourceTextures.Reset();
    AlternateMaterialSplitIndex = INDEX_NONE;
    BodySetup = nullptr;
    SpriteCollisionDomain = ESpriteCollisionMode::None;
    RenderGeometry.GeometryType = ESpritePolygonMode::TightBoundingBox;
    RenderGeometry.Shapes.Reset();
    SetPivotMode(ESpritePivotMode::Center_Center, FVector2D::ZeroVector, false);
    RebuildData();
    RefreshBakedData();
}
