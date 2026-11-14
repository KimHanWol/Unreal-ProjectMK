#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "StorageChestData.generated.h"

USTRUCT(BlueprintType)
struct FStorageChestSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;

	UPROPERTY()
	FGuid MountedCartId;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FName ItemKey = TEXT("StorageChest");

	UPROPERTY()
	bool bBuried = false;

	UPROPERTY()
	TArray<FInventorySlotData> Slots;
};
