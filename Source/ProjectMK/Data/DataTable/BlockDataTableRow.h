#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PaperSprite.h"

#include "BlockDataTableRow.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class ETileType : uint8
{
    Dirt,
    Stone,
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
    float BlockDurability = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsMineable = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag InteractEventTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FName, float> SpawnableItem;
};