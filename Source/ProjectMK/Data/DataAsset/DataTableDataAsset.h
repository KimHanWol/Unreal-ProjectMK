//LINK

#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Data/DataAsset/MKDataAssetBase.h"
#include "DataTableDataAsset.generated.h"

enum class EDataTableType : uint8;
class UDataTable;

UCLASS(BlueprintType)
class PROJECTMK_API UDataTableDataAsset : public UMKDataAssetBase
{
    GENERATED_BODY()

public:
    UDataTable* GetDataTable(EDataTableType Type) const;

private:
    virtual void AsyncLoadAsset() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<EDataTableType, TSoftObjectPtr<UDataTable>> DataTableList;
};
