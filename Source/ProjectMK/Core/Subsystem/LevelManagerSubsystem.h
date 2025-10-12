// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LevelManagerSubsystem.generated.h"

class APaperTileMapActor;

UCLASS(Blueprintable)
class PROJECTMK_API ULevelManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	void LoadTileMap(bool bForce = false);
	void GenerateTileActors();

private:
	void OnGenerateFinished();
	
	TWeakObjectPtr<APaperTileMapActor> TileMapActor;

	TMap<FVector2D, int32> TileMapIndices;
};