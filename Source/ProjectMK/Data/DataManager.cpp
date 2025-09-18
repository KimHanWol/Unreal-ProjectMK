// LINK

#include "ProjectMK/Data/DataManager.h"

#include "../Core/MKGameInstance.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Data/DataAsset/MKDataAsset.h"

UDataManager* UDataManager::Get(UObject* WorldContextObject)
{
    UMKGameInstance* GameInstance = Cast<UMKGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
    if (::IsValid(GameInstance))
    {
        return GameInstance->GetDataManager();
    }

    return nullptr; 
}

void UDataManager::InitializeManager(UMKDataAsset* InDataTableAsset)
{
    DataTableAsset = InDataTableAsset;
}

const FBlockDataTableRow* UDataManager::GetBlockDataTableRow(int32 TileIndex) const
{
    if (::IsValid(DataTableAsset) == false)
    {
        return nullptr;
    }

    UDataTable* BlockDataTable  = DataTableAsset->GetDataTable(EDataTableType::Block);
    if (BlockDataTable == nullptr)
    {
        return nullptr;
    }

    TArray<FBlockDataTableRow*> AllRows;
    BlockDataTable->GetAllRows<FBlockDataTableRow>(TEXT("BlockDataTableRow"), AllRows);

    for (const auto& Row : AllRows)
    {
        if (Row->TileIndex == TileIndex)
        {
            return Row;
        }
    }
    
    return nullptr;
}
