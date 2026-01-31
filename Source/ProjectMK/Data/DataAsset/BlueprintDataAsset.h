//LINK

#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Data/DataAsset/MKDataAssetBase.h"
#include "BlueprintDataAsset.generated.h"

class ABlockBase;

UENUM()
enum class EBlueprintClassType : uint8
{
    None = 0,
    BlockBase,
};

UCLASS(BlueprintType)
class PROJECTMK_API UBlueprintDataAsset : public UMKDataAssetBase
{
    GENERATED_BODY()

public:
    TSubclassOf<AActor> GetBlueprintClass(EBlueprintClassType BlueprintClassType);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<EBlueprintClassType, TSubclassOf<AActor>> BlueprintClassMap; 
};
