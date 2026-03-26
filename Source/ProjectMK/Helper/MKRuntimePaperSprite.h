#pragma once

#include "CoreMinimal.h"
#include "PaperSprite.h"

#include "MKRuntimePaperSprite.generated.h"

UCLASS(Transient)
class PROJECTMK_API UMKRuntimePaperSprite : public UPaperSprite
{
    GENERATED_BODY()

public:
    void InitializeFromAtlasCell(UTexture2D* AtlasTexture, const FIntPoint& CellOrigin, const FIntPoint& CellSize, float InPixelsPerUnrealUnit = 2.56f);
};
