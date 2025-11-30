#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "EquipmentItemDataTableRow.generated.h"

UENUM(BlueprintType)
enum class EEuipmentType : uint8
{
    Halmet,
    Drill,
    balloon,
    shoes,
};

USTRUCT(BlueprintType)
struct FEquipmentRecipe
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "FItemDataTableRow"))
    FName MaterialKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaterialCount;
};

USTRUCT(BlueprintType)
struct FEquipmentItemDataTableRow : public FItemDataTableRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText EquipmentName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UPaperSprite> EquipmentSprite;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EEuipmentType EquipmentType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EquipmentPrice;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FEquipmentRecipe> CraftRecipe;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<TSubclassOf<UGameplayEffect>> EqiupEffectClasses;
};
