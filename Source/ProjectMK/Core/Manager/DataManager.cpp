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

bool UDataManager::GetCharacterDataTableRow(FName PreferredRowKey, FCharacterDataTableRow& OutRow) const
{
	if (::IsValid(DataTableDataAsset) == false)
	{
		return false;
	}

	UDataTable* CharacterDataTable = DataTableDataAsset->GetDataTable(EDataTableType::Character);
	if (::IsValid(CharacterDataTable) == false)
	{
		return false;
	}

	const TArray<FName> RowNames = CharacterDataTable->GetRowNames();
	if (RowNames.IsEmpty())
	{
		return false;
	}

	const FName TargetRowName = PreferredRowKey.IsNone() == false && RowNames.Contains(PreferredRowKey)
		? PreferredRowKey
		: RowNames[0];

	if (CharacterDataTable->GetRowStruct() == FCharacterDataTableRow::StaticStruct())
	{
		const FCharacterDataTableRow* CharacterData = CharacterDataTable->FindRow<FCharacterDataTableRow>(TargetRowName, TEXT("GetCharacterDataTableRow"));
		if (CharacterData == nullptr)
		{
			return false;
		}

		OutRow = *CharacterData;
		return true;
	}

	if (CharacterDataTable->GetRowStruct() == FCharacterAnimationDataTableRow::StaticStruct())
	{
		// Temporary compatibility path for older Character DataTable assets that still use the legacy row struct.
		const FCharacterAnimationDataTableRow* LegacyCharacterData = CharacterDataTable->FindRow<FCharacterAnimationDataTableRow>(TargetRowName, TEXT("GetCharacterDataTableRowLegacy"));
		if (LegacyCharacterData == nullptr)
		{
			return false;
		}

		OutRow = LegacyCharacterData->ToCharacterDataTableRow();
		return true;
	}

	return false;
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

TArray<FName> UDataManager::GetShopRecipeRowNames() const
{
	if (::IsValid(DataTableDataAsset) == false)
	{
		return {};
	}

	UDataTable* ShopRecipeDataTable = DataTableDataAsset->GetDataTable(EDataTableType::ShopRecipe);
	if (ShopRecipeDataTable == nullptr)
	{
		return {};
	}

	return ShopRecipeDataTable->GetRowNames();
}

const FShopRecipeDataTableRow* UDataManager::GetShopRecipeDataTableRow(FName RowName) const
{
	return GetDataTableRow<FShopRecipeDataTableRow>(EDataTableType::ShopRecipe, RowName);
}

const UGameSettingDataAsset* UDataManager::GetGameSettingDataAsset() const
{
	return GameSettingDataAsset;
}

UDataTable* UDataManager::GetDataTable(EDataTableType DataTableType) const
{
	if (::IsValid(DataTableDataAsset) == false)
	{
		return nullptr;
	}

	return DataTableDataAsset->GetDataTable(DataTableType);
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
