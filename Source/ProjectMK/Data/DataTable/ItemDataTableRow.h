#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "ItemDataTableRow.generated.h"

USTRUCT(BlueprintType)
struct FItemDataTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UPaperSprite> ItemIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsOre;
};
