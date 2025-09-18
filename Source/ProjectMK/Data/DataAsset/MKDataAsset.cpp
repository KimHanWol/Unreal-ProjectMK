// LINK

#include "ProjectMK/Data/DataAsset/MKDataAsset.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

void UMKDataAsset::PostLoad()
{
	Super::PostLoad();

	AsyncLoadAsset();
}

UDataTable* UMKDataAsset::GetDataTable(EDataTableType Type) const
{
	const TSoftObjectPtr<UDataTable>* FoundPtr = DataTableList.Find(Type);
	if (FoundPtr && FoundPtr->IsNull() == false)
	{
		return FoundPtr->LoadSynchronous();
	}

	return nullptr;
}

void UMKDataAsset::AsyncLoadAsset()
{
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

	for (const auto& InDataTable : DataTableList)
	{
		Streamable.RequestAsyncLoad(InDataTable.Value.ToSoftObjectPath(), FStreamableDelegate());
	}
}
