#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGameSubsystem.generated.h"

class UMKSaveGame;

UCLASS()
class PROJECTMK_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static USaveGameSubsystem* Get(const UObject* WorldContextObject);

	bool TrySaveGameToSlot();
	bool StartPlaytestSession();

	UMKSaveGame* GetSaveGame() const;
	bool IsPlaytestSession() const;

private:
	void LoadGameFromSlot();

private:
	UPROPERTY(Transient)
	TObjectPtr<UMKSaveGame> SaveGame;

	bool bIsPlaytestSession = false;
};
