#include "ProjectMK/Helper/Utils/EquipmentItemDataTableUtil.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DataTable.h"
#include "Modules/ModuleManager.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"

namespace
{
	TArray<FAssetData> GetEquipmentItemDataTableAssetListForLookup()
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		FARFilter Filter;
		Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> DataTableAssets;
		AssetRegistryModule.Get().GetAssets(Filter, DataTableAssets);
		return DataTableAssets;
	}
}

const FEquipmentItemDataTableRow* FEquipmentItemDataTableUtil::FindEquipmentItemData(UObject* WorldContextObject, FName EquipmentKey)
{
	if (EquipmentKey.IsNone())
	{
		return nullptr;
	}

	const UDataManager* DataManager = UDataManager::Get(WorldContextObject);
	if (::IsValid(DataManager))
	{
		if (const FEquipmentItemDataTableRow* EquipmentData = DataManager->GetDataTableRow<FEquipmentItemDataTableRow>(EDataTableType::EquipmentItem, EquipmentKey))
		{
			return EquipmentData;
		}
	}

	const TArray<FAssetData> DataTableAssets = GetEquipmentItemDataTableAssetListForLookup();
	for (const FAssetData& DataTableAsset : DataTableAssets)
	{
		const FString RowStructPath = DataTableAsset.TagsAndValues.FindTag(TEXT("RowStructure")).AsString();
		if (RowStructPath.Contains(TEXT("EquipmentItemDataTableRow")) == false)
		{
			continue;
		}

		UDataTable* DataTable = Cast<UDataTable>(DataTableAsset.GetAsset());
		if (::IsValid(DataTable) == false || DataTable->GetRowStruct() != FEquipmentItemDataTableRow::StaticStruct())
		{
			continue;
		}

		if (const FEquipmentItemDataTableRow* EquipmentData = DataTable->FindRow<FEquipmentItemDataTableRow>(EquipmentKey, TEXT("FindEquipmentItemDataFallback")))
		{
			return EquipmentData;
		}
	}

	return nullptr;
}

TArray<FName> FEquipmentItemDataTableUtil::GetEquipmentItemRowNames(UObject* WorldContextObject)
{
	TSet<FName> UniqueRowNames;

	const UDataManager* DataManager = UDataManager::Get(WorldContextObject);
	if (::IsValid(DataManager))
	{
		if (UDataTable* EquipmentDataTable = DataManager->GetDataTable(EDataTableType::EquipmentItem))
		{
			for (const FName& RowName : EquipmentDataTable->GetRowNames())
			{
				UniqueRowNames.Add(RowName);
			}
		}
	}

	const TArray<FAssetData> DataTableAssets = GetEquipmentItemDataTableAssetListForLookup();
	for (const FAssetData& DataTableAsset : DataTableAssets)
	{
		const FString RowStructPath = DataTableAsset.TagsAndValues.FindTag(TEXT("RowStructure")).AsString();
		if (RowStructPath.Contains(TEXT("EquipmentItemDataTableRow")) == false)
		{
			continue;
		}

		const UDataTable* DataTable = Cast<UDataTable>(DataTableAsset.GetAsset());
		if (::IsValid(DataTable) == false || DataTable->GetRowStruct() != FEquipmentItemDataTableRow::StaticStruct())
		{
			continue;
		}

		for (const FName& RowName : DataTable->GetRowNames())
		{
			UniqueRowNames.Add(RowName);
		}
	}

	TArray<FName> SortedRowNames = UniqueRowNames.Array();
	SortedRowNames.Sort(FNameLexicalLess());
	return SortedRowNames;
}
