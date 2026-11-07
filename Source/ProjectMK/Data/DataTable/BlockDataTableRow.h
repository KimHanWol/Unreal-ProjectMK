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

	/** 빈 공간과 다른 재질의 경계를 다듬는다. 중심 무늬와 충돌 크기는 유지한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	bool bUseTerrainEdges = false;

	/** 더 높은 재질이 이웃 블록의 가장자리 안으로 이어진다. 같은 값은 경계를 만들지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	int32 TerrainBlendPriority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BlockDurability = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsMineable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBlockSpawnableItemData> SpawnableItemDataList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "NeedToBeHide"))
	bool bNeedTobeHide = false;

	/** 채굴 가능하지만 Rock 처럼 붙은 덩어리를 지탱한다 (버팀목) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsSupportAnchor = false;

	/** 발파 폭약으로도 부서지지 않는다 (Border, Shop 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsExplosionProof = false;

	/** 캐릭터는 통과한다. 드릴과 낙하 블록 충돌은 그대로 받는다 (캐릭터 칸에 설치하는 버팀목) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsPassableByCharacter = false;
};
