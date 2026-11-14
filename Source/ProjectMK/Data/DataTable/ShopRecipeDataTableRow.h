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

UENUM(BlueprintType)
enum class EShopOfferType : uint8
{
	Recipe,
	Item,
};

USTRUCT(BlueprintType)
struct FShopRecipeDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	EShopOfferType OfferType = EShopOfferType::Recipe;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "0", EditCondition = "OfferType == EShopOfferType::Item"))
	int32 PurchasePrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "1", EditCondition = "OfferType == EShopOfferType::Item"))
	int32 PurchaseLimit = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = "true"))
	FText ShortDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetItemRowNames"))
	FName BlueprintItemKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FShopRecipeItem> RequiredItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetItemRowNames"))
	FName ResultItemKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 ResultItemCount = 1;

	/** 상점에서 레시피를 해금하는 가격. 0 이면 처음부터 해금되어 있다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 UnlockPrice = 0;

	FName GetResultItemKey() const
	{
		return ResultItemKey;
	}
};
