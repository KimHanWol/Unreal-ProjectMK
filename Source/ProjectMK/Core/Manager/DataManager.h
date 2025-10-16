//LINK

#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Data/DataAsset/DataTableDataAsset.h"
#include "ProjectMK/Data/DataTable/BlockDataTableRow.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"
#include "UObject/NoExportTypes.h"

#include "DataManager.generated.h"

class UGameplayEffectDataAsset;
class UGameplayAbility;

enum class EGameplayEffectType : uint8;
enum class EGameplayAbilityType : uint8;

UCLASS(Blueprintable)
class PROJECTMK_API UDataManager : public UObject
{
    GENERATED_BODY()

public:
    static UDataManager* Get(UObject* WorldContextObject);

    template<typename T>
    const T* GetDataTableRow(EDataTableType DataTableType, FName Key) const
    {
        if (::IsValid(DataTableDataAsset) == false)
        {
            return nullptr;
        }

        UDataTable* DataTable = DataTableDataAsset->GetDataTable(DataTableType);
        if (DataTable == nullptr)
        {
            return nullptr;
        }

        return DataTable->FindRow<T>(Key, TEXT("GetDataTableRow"));
    }

    const FBlockDataTableRow* GetBlockDataTableRow(int32 TileIndex) const;

    UFUNCTION(BlueprintCallable)
    FItemDataTableRow BP_GetItemDataTableRow(FName ItemUID) const;

    TSubclassOf<UGameplayEffect> GetGameplayEffect(EGameplayEffectType EffectType);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UDataTableDataAsset* DataTableDataAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UGameplayEffectDataAsset* GameplayEffectDataAsset;
};
