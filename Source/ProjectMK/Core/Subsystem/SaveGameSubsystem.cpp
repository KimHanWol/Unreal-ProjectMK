#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Core/MKSaveGame.h"

namespace
{
	const FString SaveSlotName = TEXT("MKSave");
	const FString BackupSaveSlotName = TEXT("MKSave_Backup");
	constexpr int32 SaveUserIndex = 0;

	UMKSaveGame* LoadSaveGameFromSlot(const FString& SlotName)
	{
		if (UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex) == false)
		{
			return nullptr;
		}

		return Cast<UMKSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex));
	}
}

void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadGameFromSlot();
}

USaveGameSubsystem* USaveGameSubsystem::Get(const UObject* WorldContextObject)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (::IsValid(GameInstance) == false)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<USaveGameSubsystem>();
}

bool USaveGameSubsystem::TrySaveGameToSlot()
{
	if (::IsValid(SaveGame) == false || bIsPlaytestSession)
	{
		return false;
	}

	// 정상적으로 읽히는 기존 세이브만 백업으로 옮겨, 손상된 파일이 백업을 덮어쓰지 않게 한다.
	TArray<uint8> PreviousSaveData;
	if (::IsValid(LoadSaveGameFromSlot(SaveSlotName)) && UGameplayStatics::LoadDataFromSlot(PreviousSaveData, SaveSlotName, SaveUserIndex))
	{
		UGameplayStatics::SaveDataToSlot(PreviousSaveData, BackupSaveSlotName, SaveUserIndex);
	}

	return UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, SaveUserIndex);
}

bool USaveGameSubsystem::StartPlaytestSession()
{
#if WITH_EDITOR
	if (GetWorld() != nullptr && GetWorld()->WorldType == EWorldType::PIE)
	{
		// 테스트 지급 후 종료할 때까지 자동 저장이 실제 진행을 덮어쓰지 않게 한다.
		bIsPlaytestSession = true;
		return true;
	}
#endif
	return false;
}

UMKSaveGame* USaveGameSubsystem::GetSaveGame() const
{
	return SaveGame;
}

bool USaveGameSubsystem::IsPlaytestSession() const
{
	return bIsPlaytestSession;
}

void USaveGameSubsystem::LoadGameFromSlot()
{
	SaveGame = LoadSaveGameFromSlot(SaveSlotName);

	if (::IsValid(SaveGame) == false && UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("Save slot '%s' is corrupted. Loading backup slot '%s'."), *SaveSlotName, *BackupSaveSlotName);
		SaveGame = LoadSaveGameFromSlot(BackupSaveSlotName);
	}

	if (::IsValid(SaveGame) == false)
	{
		SaveGame = Cast<UMKSaveGame>(UGameplayStatics::CreateSaveGameObject(UMKSaveGame::StaticClass()));
	}
}
