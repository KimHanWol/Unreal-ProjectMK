#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Data/DataAsset/MKDataAssetBase.h"

#include "GameSettingDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTMK_API UGameSettingDataAsset : public UMKDataAssetBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Falling Block")
    int32 FallingBlockDamagePerBlock = 1;
};
