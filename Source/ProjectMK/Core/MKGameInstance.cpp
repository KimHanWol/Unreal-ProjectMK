// LINK

#include "ProjectMK/Core/MKGameInstance.h"

#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Manager/SoundManager.h"

void UMKGameInstance::Init()
{
	Super::Init();

	if (::IsValid(DataManagerClass))
	{
		DataManager = NewObject<UDataManager>(this, DataManagerClass);
	}

	if (::IsValid(SoundManagerClass))
	{
		SoundManager = NewObject<USoundManager>(this, SoundManagerClass);
		SoundManager->Init();
	}
}

UDataManager* UMKGameInstance::GetDataManager()
{
	return DataManager;
}

USoundManager* UMKGameInstance::GetSoundManager()
{
	return SoundManager;
}
