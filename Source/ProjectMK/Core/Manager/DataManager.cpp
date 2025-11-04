// LINK

#include "ProjectMK/Core/Manager/DataManager.h"

#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Core/MKGameInstance.h"
#include "ProjectMK/Data/DataAsset/DataTableDataAsset.h"
#include "ProjectMK/Data/DataAsset/GameplayEffectDataAsset.h"

UDataManager* UDataManager::Get(UObject* WorldContextObject)
{
    UMKGameInstance* GameInstance = Cast<UMKGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
    if (::IsValid(GameInstance))
    {
        return GameInstance->GetDataManager();
    }

    return nullptr; 
}

const FBlockDataTableRow* UDataManager::GetBlockDataTableRow(int32 TileIndex) const
{
    if (::IsValid(DataTableDataAsset) == false)
    {
        return nullptr;
    }

    UDataTable* BlockDataTable  = DataTableDataAsset->GetDataTable(EDataTableType::Block);
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

FItemDataTableRow UDataManager::BP_GetItemDataTableRow(FName ItemUID) const
{
    FItemDataTableRow DataTableRow;
    if (::IsValid(DataTableDataAsset) == false)
    {
        return DataTableRow;
    }

    UDataTable* BlockDataTable = DataTableDataAsset->GetDataTable(EDataTableType::Block);
    if (BlockDataTable == nullptr)
    {
        return DataTableRow;
    }

    const FItemDataTableRow* ItemDataRow = GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemUID);
    if (ItemDataRow)
    {
        DataTableRow = *ItemDataRow;
    }

    return DataTableRow;
}

TSubclassOf<UGameplayEffect> UDataManager::GetGameplayEffect(EGameplayEffectType EffectType)
{
    if (::IsValid(GameplayEffectDataAsset))
    {
        return GameplayEffectDataAsset->GetGameplayEffect(EffectType);
    }

    return nullptr;
}
