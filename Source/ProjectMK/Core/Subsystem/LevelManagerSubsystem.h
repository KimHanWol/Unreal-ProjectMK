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

	int32 GetTileSize();

private:
	void OnGenerateFinished();
	void OnBlockDestroyed(TWeakObjectPtr<ABlockBase> DestroyedBlock);

	bool CheckBlockIsAllDisconnected(const FVector2D& StartPosition);
	const TArray<FVector2D> GetSurroundBlockPositions(const FVector2D& TargetBlockPosition) const;

	TArray<FVector2D> BFSCheckArray;

	TWeakObjectPtr<APaperTileMapActor> TileMapActor;
	TMap<TWeakObjectPtr<ABlockBase>, FVector2D> TilePositionMap;
	TMap<FVector2D, TWeakObjectPtr<ABlockBase>> TileMap;
};