//LINK

#pragma once

#include "CoreMinimal.h"
#include "MKDataAssetBase.generated.h"

UCLASS(BlueprintType)
class PROJECTMK_API UMKDataAssetBase : public UPrimaryDataAsset
{
    GENERATED_BODY()

protected:
    virtual void PostLoad() override;

    virtual void AsyncLoadAsset() {};
};
