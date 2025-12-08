#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "SoundDataTableRow.generated.h"

enum class ESFXParamType : uint8;

USTRUCT(BlueprintType)
struct FSoundDataTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESFXType SFXType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USoundBase> SoundSource;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<ESFXInstanceParamType> InitialParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<ESFXContinuousParamType> ContinuousParams;
};