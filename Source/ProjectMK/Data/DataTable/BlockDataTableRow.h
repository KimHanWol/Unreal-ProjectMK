#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PaperSprite.h"

#include "BlockDataTableRow.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct FBlockSpawnableItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetItemRowNames"))
	FName SpawnableItemKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OreSpawnWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 MinSpawnItemCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 MaxSpawnItemCount = 1;
};

USTRUCT(BlueprintType)
struct FBlockDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

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
	TArray<FBlockSpawnableItemData> SpawnableItemDataList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "NeedToBeHide"))
	bool bNeedTobeHide = false;
};
