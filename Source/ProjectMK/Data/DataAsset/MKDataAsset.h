//LINK

#pragma once

#include "CoreMinimal.h"
#include "MKDataAsset.generated.h"

enum class EDataTableType : uint8;
class UDataTable;

UCLASS(BlueprintType)
class PROJECTMK_API UMKDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

protected:
    virtual void PostLoad() override;

public:
    UDataTable* GetDataTable(EDataTableType Type) const;

private:
    void AsyncLoadAsset();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<EDataTableType, TSoftObjectPtr<UDataTable>> DataTableList;
};
