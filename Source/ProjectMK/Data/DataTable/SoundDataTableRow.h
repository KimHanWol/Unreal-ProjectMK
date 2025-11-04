#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "SoundDataTableRow.generated.h"


USTRUCT(BlueprintType)
struct FSoundDataTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USoundBase> SoundSource;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsLoop;
};