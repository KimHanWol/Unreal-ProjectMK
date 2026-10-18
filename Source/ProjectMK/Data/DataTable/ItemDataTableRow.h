#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"

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

	/** None 이 아니면 퀵슬롯 숫자 키로 사용할 수 있는 소모품이다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	EConsumableEffectType ConsumableEffect = EConsumableEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	float EffectValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	float EffectDuration = 0.f;

	/** PlaceBlock 효과로 설치할 BlockDataTable 행 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	FName PlaceBlockKey = NAME_None;
};
