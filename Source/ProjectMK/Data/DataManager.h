//LINK

#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Data/DataAsset/MKDataAsset.h"
#include "ProjectMK/Data/DataTable/BlockDataTableRow.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"
#include "UObject/NoExportTypes.h"

#include "DataManager.generated.h"

UCLASS()
class PROJECTMK_API UDataManager : public UObject
{
    GENERATED_BODY()

public:
    static UDataManager* Get(UObject* WorldContextObject);
    void InitializeManager(UMKDataAsset* InDataTableAsset);

    template<typename T>
    const T* GetDataTableRow(EDataTableType DataTableType, FName Key) const
    {
        if (::IsValid(DataTableAsset) == false)
        {
            return nullptr;
        }

        UDataTable* DataTable = DataTableAsset->GetDataTable(DataTableType);
        if (DataTable == nullptr)
        {
            return nullptr;
        }

        return DataTable->FindRow<T>(Key, TEXT("GetDataTableRow"));
    }

    const FBlockDataTableRow* GetBlockDataTableRow(int32 TileIndex) const;


private:
    UPROPERTY(Transient)
    UMKDataAsset* DataTableAsset;
};
