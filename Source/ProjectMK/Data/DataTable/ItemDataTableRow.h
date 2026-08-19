#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "ItemDataTableRow.generated.h"

class UPaperSprite;

USTRUCT(BlueprintType)
struct FItemDataTableRowBase : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsOre;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SellPrice;
};

USTRUCT(BlueprintType)
struct FItemDataTableRow : public FItemDataTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsInventoryItem = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsSplitDrop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStackCount = 99;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UPaperSprite> ItemIcon;
};
