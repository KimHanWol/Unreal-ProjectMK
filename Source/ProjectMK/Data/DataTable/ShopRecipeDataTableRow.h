#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ShopRecipeDataTableRow.generated.h"

USTRUCT(BlueprintType)
struct FShopRecipeItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetItemRowNames"))
	FName ItemKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 ItemCount = 1;

	FName GetItemKey() const
	{
		return ItemKey;
	}
};

USTRUCT(BlueprintType)
struct FShopRecipeDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FShopRecipeItem> RequiredItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetItemRowNames"))
	FName ResultItemKey;

	FName GetResultItemKey() const
	{
		return ResultItemKey;
	}
};
