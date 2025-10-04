// LINK

#include "ProjectMK/Data/DataAsset/DataTableDataAsset.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

UDataTable* UDataTableDataAsset::GetDataTable(EDataTableType Type) const
{
	const TSoftObjectPtr<UDataTable>* FoundPtr = DataTableList.Find(Type);
	if (FoundPtr && FoundPtr->IsNull() == false)
	{
		return FoundPtr->LoadSynchronous();
	}

	return nullptr;
}

void UDataTableDataAsset::AsyncLoadAsset()
{
	Super::AsyncLoadAsset();

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

	for (const auto& InDataTable : DataTableList)
	{
		Streamable.RequestAsyncLoad(InDataTable.Value.ToSoftObjectPath(), FStreamableDelegate());
	}
}
