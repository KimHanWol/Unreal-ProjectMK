// LINK

#pragma once

#include "CoreMinimal.h"

#include "ProjectMK/Data/DataAsset/DataTableDataAsset.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Data/DataTable/BlockDataTableRow.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/ShopRecipeDataTableRow.h"
#include "ProjectMK/Data/DataTable/SoundDataTableRow.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"
#include "UObject/NoExportTypes.h"

#include "DataManager.generated.h"

class UBlueprintDataAsset;
class UGameplayEffectDataAsset;
class UGameplayAbility;

enum class EBlueprintClassType : uint8;
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
	const FSoundDataTableRow* GetSoundDataTableRow(ESFXType InSFXType) const;
	const UGameSettingDataAsset* GetGameSettingDataAsset() const;
	UDataTable* GetDataTable(EDataTableType DataTableType) const;

	TSubclassOf<UGameplayEffect> GetGameplayEffect(EGameplayEffectType EffectType) const;
	TSubclassOf<AActor> GetBlueprintClass(EBlueprintClassType BlueprintClassType) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UDataTableDataAsset* DataTableDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UGameplayEffectDataAsset* GameplayEffectDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBlueprintDataAsset* BlueprintDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UGameSettingDataAsset* GameSettingDataAsset;
};
