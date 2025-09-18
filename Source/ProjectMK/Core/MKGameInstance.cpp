// LINK

#include "ProjectMK/Core/MKGameInstance.h"

#include "ProjectMK/Data/DataManager.h"

void UMKGameInstance::Init()
{
	Super::Init();

	DataManager = NewObject<UDataManager>();
	DataManager->InitializeManager(DataTableAsset);
}

UDataManager* UMKGameInstance::GetDataManager()
{
	return DataManager;
}
