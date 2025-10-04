#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PaperSprite.h"
#include "BlockDataTableRow.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class ETileType : uint8
{
    Dirt,
    Stone,
    Ore_Copper,
    Ore_Iron,
};

USTRUCT(BlueprintType)
struct FBlockDataTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETileType TileType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TileIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasCollision = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UPaperSprite> TileSprite;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UGameplayEffect> BlockInitEffect;
};