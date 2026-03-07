// LINK

#include "ProjectMK/Core/Manager/DataManager.h"

#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Core/MKGameInstance.h"
#include "ProjectMK/Data/DataAsset/BlueprintDataAsset.h"
#include "ProjectMK/Data/DataAsset/DataTableDataAsset.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
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

const FSoundDataTableRow* UDataManager::GetSoundDataTableRow(ESFXType InSFXType) const
{
    if (::IsValid(DataTableDataAsset) == false)
    {
        return nullptr;
    }

    UDataTable* SoundDataTable = DataTableDataAsset->GetDataTable(EDataTableType::Sound);
    if (SoundDataTable == nullptr)
    {
        return nullptr;
    }

    TArray<FSoundDataTableRow*> AllRows;
    SoundDataTable->GetAllRows<FSoundDataTableRow>(TEXT("SoundDataTableRow"), AllRows);

    for (const auto& Row : AllRows)
    {
        if (Row->SFXType == InSFXType)
        {
            return Row;
        }
    }

    return nullptr;
}

const UGameSettingDataAsset* UDataManager::GetGameSettingDataAsset() const
{
    return GameSettingDataAsset;
}

TSubclassOf<UGameplayEffect> UDataManager::GetGameplayEffect(EGameplayEffectType EffectType) const
{
    if (::IsValid(GameplayEffectDataAsset))
    {
        return GameplayEffectDataAsset->GetGameplayEffect(EffectType);
    }

    return nullptr;
}

TSubclassOf<AActor> UDataManager::GetBlueprintClass(EBlueprintClassType BlueprintClassType) const
{
    if (::IsValid(BlueprintDataAsset))
    {
        return BlueprintDataAsset->GetBlueprintClass(BlueprintClassType);
    }

    return nullptr;
}
