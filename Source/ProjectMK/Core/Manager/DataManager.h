//LINK

#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Data/DataAsset/DataTableDataAsset.h"
#include "ProjectMK/Data/DataTable/BlockDataTableRow.h"
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

    TSubclassOf<UGameplayEffect> GetGameplayEffect(EGameplayEffectType EffectType);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UDataTableDataAsset* DataTableDataAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UGameplayEffectDataAsset* GameplayEffectDataAsset;

    // GameplayAbility 관련해서 DataAsset 으로 하나로 묶고
    // Ability 불러와서 적용
    // 대미지 입으면 파괴
    // 모든 블록 로딩되면 기존에 있던 맵 화면 가리도록 함
};
