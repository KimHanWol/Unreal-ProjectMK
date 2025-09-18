// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MKGameInstance.generated.h"

class UMKDataAsset;
class UDataManager;

UCLASS(Blueprintable)
class PROJECTMK_API UMKGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	virtual void Init() override;

public:
	UDataManager* GetDataManager();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMKDataAsset* DataTableAsset;

	UPROPERTY()
	UDataManager* DataManager;
};