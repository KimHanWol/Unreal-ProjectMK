// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LevelManagerSubsystem.generated.h"

class APaperTileMapActor;

USTRUCT(BlueprintType)
struct FFallingVerticalBlocksData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bFallFinished = false;

	UPROPERTY()
	TArray<TWeakObjectPtr<ABlockBase>> FallingBlocks;

	UPROPERTY()
	TWeakObjectPtr<ABlockBase> BottomBlock;

	TMap<TWeakObjectPtr<ABlockBase>, FVector> BlockOriginPositionMap;
};

USTRUCT(BlueprintType)
struct FFallingBlockGroupData
{
    GENERATED_BODY()

	UPROPERTY()
	int32 BlockCount = 0;

    UPROPERTY()
    float FallingVelocity;

	UPROPERTY()
	float ShakeTime = 0.f;

	UPROPERTY()
	float ShakeDuration = 1.f; //HARD CODE

	UPROPERTY()
	float ShakeInterval = 0.05f; //HARD CODE

	UPROPERTY()
	float ShakeAccumulatedTime = 0.f;

	TMap <int32, FFallingVerticalBlocksData> FallingBlockMap; // BlockPosition X, VerticalBlocks

	void FinishFallingBlock(int32 FallFinishedBlockPositionX);

	void Tick_ShakeBlocks(float DeltaTime);
	void Tick_FallBlocks(float DeltaTime);

	bool IsShaking() { return ShakeTime < ShakeDuration; }
	bool IsFallingFinished();
	TWeakObjectPtr<ABlockBase> GetBottomBlock(int32 BlockPositionX);
	TArray<ABlockBase*> GetFallingBlocks();
	float GetFallingDamage();
};

UCLASS(Blueprintable)
class PROJECTMK_API ULevelManagerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	//UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	//~UTickableWorldSubsystem

	void LoadTileMap(bool bForce = false);
	void GenerateTileActors();

	int32 GetTileSize();

private:
	void OnGenerateFinished();
	void OnBlockDestroyed(ABlockBase* DestroyedBlock);

	bool CheckBlockIsAllDisconnected(ABlockBase* StartBlock, TArray<ABlockBase*>& OutDisconnectedBlocks, TArray<ABlockBase*>& DFSCheckedBlocks, int32 DebugCount);
	TArray<ABlockBase*> GetSurroundBlocks(ABlockBase* TargetBlock);

	void CollapseBlocks(const TArray<ABlockBase*>& CollapsingBlocks);
	void SnapBlocks(const TArray<TWeakObjectPtr<ABlockBase>>& FallFinishedBlockList);

	UPROPERTY(Transient)
	TWeakObjectPtr<APaperTileMapActor> TileMapActor;

	UPROPERTY(Transient)
	TArray<ABlockBase*> BlockList;

	UPROPERTY(Transient)
	TArray<FFallingBlockGroupData> FallingBlockGroupList;

	TMap<FVector2D, TWeakObjectPtr<ABlockBase>> BlockActorMap;
};