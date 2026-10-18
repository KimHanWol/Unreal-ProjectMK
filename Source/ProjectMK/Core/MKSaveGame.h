#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "MKSaveGame.generated.h"

UCLASS()
class PROJECTMK_API UMKSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 SaveVersion = 1;

	UPROPERTY()
	int32 Coin = 0;

	UPROPERTY()
	TArray<FName> PurchasedSkillKeys;

	UPROPERTY()
	TArray<FInventorySlotData> InventorySlotDataList;

	/** INDEX_NONE 이면 저장된 지형이 없어 새 시드로 생성한다 */
	UPROPERTY()
	int32 GenerationSeed = INDEX_NONE;

	UPROPERTY()
	TArray<FName> UnlockedRecipeKeys;
};
