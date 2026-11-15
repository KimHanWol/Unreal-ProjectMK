#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Data/StorageChestData.h"
#include "ProjectMK/Data/MineTownData.h"
#include "MKSaveGame.generated.h"

UCLASS()
class PROJECTMK_API UMKSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 SaveVersion = 3;

	UPROPERTY()
	int32 Coin = 0;

	UPROPERTY() int32 MaximumReachedDepth = 0;
	UPROPERTY() TArray<FTownQuestState> TownQuests;
	UPROPERTY() TSet<FName> OwnedRelics;
	UPROPERTY() TArray<FTreeSaveData> Trees;
	UPROPERTY() TArray<FRailSaveData> Rails;
	UPROPERTY() TArray<FMineCartSaveData> MineCarts;
	UPROPERTY() TArray<FInstalledStructureSaveData> InstalledStructures;

	UPROPERTY()
	TArray<FName> PurchasedSkillKeys;

	UPROPERTY()
	TArray<FInventorySlotData> InventorySlotDataList;

	UPROPERTY()
	TArray<FInventorySlotData> QuickSlotDataList;

	UPROPERTY()
	TSet<FName> DiscoveredItemKeys;

	/** INDEX_NONE 이면 저장된 지형이 없어 새 시드로 생성한다 */
	UPROPERTY()
	int32 GenerationSeed = INDEX_NONE;

	UPROPERTY()
	TArray<FName> UnlockedRecipeKeys;

	UPROPERTY()
	TArray<FStorageChestSaveData> StorageChests;

	UPROPERTY()
	TArray<FGuid> DestroyedStorageChestIds;
};
