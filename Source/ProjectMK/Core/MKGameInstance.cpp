// LINK

#include "ProjectMK/Core/MKGameInstance.h"

#include "ProjectMK/Core/Manager/DataManager.h"

void UMKGameInstance::Init()
{
	Super::Init();

	if (::IsValid(DataManagerClass))
	{
		DataManager = NewObject<UDataManager>(this, DataManagerClass);
	}
}

UDataManager* UMKGameInstance::GetDataManager()
{
	return DataManager;
}
