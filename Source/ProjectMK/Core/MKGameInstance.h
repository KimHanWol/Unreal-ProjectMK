// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MKGameInstance.generated.h"

class UDataManager;

UCLASS(Blueprintable)
class PROJECTMK_API UMKGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	virtual void Init() override;

public:
	UDataManager* GetDataManager();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDataManager> DataManagerClass;

private:
	UPROPERTY()
	UDataManager* DataManager;
};