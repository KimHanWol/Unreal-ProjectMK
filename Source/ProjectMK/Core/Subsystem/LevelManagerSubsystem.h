#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LevelManagerSubsystem.generated.h"

class APaperTileMapActor;
class ABlockBase;
class UDataManager;
class UPaperTileMapComponent;
struct FMineableLevelGenerationSettings;

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

	UPROPERTY()
	float FallenDistance = 0.f;

	TMap<TWeakObjectPtr<ABlockBase>, FVector> BlockOriginPositionMap;
};

USTRUCT(BlueprintType)
struct FFallingBlockGroupData
{
	GENERATED_BODY()

	UPROPERTY()
	float FallingVelocity = 0.f;

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
	int32 CalculateCharacterFallingDamage(UObject* WorldContextObject, const FFallingVerticalBlocksData& VerticalBlockData) const;
	float CalculateFallingBlockImpactDamage(UObject* WorldContextObject) const;
	bool TryDestroyFallingBlockOnImpact(UObject* WorldContextObject, ABlockBase* FallingBlock) const;
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

	UFUNCTION(BlueprintCallable, Category = "Level Generation")
	void RegenerateMineableBlocks(int32 GenerationSeed = -1);

	void SetBlockDebugNumbersEnabled(bool bEnabled);
	int32 GetTileSize();
	ABlockBase* GetBlockAtPosition(const FVector2D& BlockPosition) const;

	UFUNCTION(BlueprintPure, Category = "Level Generation")
	int32 GetLastGenerationSeed() const { return LastGenerationSeed; }

	bool IsBlockDebugNumbersEnabled() const { return bBlockDebugNumbersEnabled; }

private:
	friend struct FFallingBlockGroupData;

	void CaptureTileTemplates(UDataManager* DataManager, UPaperTileMapComponent* TileMapComponent);
	void ClearGeneratedBlocks();
	void GenerateMineableLayout(
		FRandomStream& RandomStream,
		const FMineableLevelGenerationSettings& GenerationSettings,
		int32 RockTileIndex,
		TMap<FIntPoint, int32>& OutGeneratedMineableTiles) const;
	void SpawnTileActor(
		const FIntPoint& TileCoordinate,
		int32 TileIndex,
		UPaperTileMapComponent* TileMapComponent,
		TSubclassOf<AActor> BlockClass);

	void OnGenerateFinished();
	void OnBlockDestroyed(ABlockBase* DestroyedBlock);

	bool CheckBlockIsAllDisconnected(ABlockBase* StartBlock, TArray<ABlockBase*>& OutDisconnectedBlocks, TArray<ABlockBase*>& DFSCheckedBlocks, int32 DebugCount);

	void QueueCollapseBlocks(const TArray<ABlockBase*>& CollapsingBlocks);
	void ProcessPendingCollapseBlocks();
	void CollapseBlocks(const TArray<ABlockBase*>& CollapsingBlocks);
	void SnapBlocks(const TArray<TWeakObjectPtr<ABlockBase>>& FallFinishedBlockList);
	void LogFallingStopDiagnostic(
		ABlockBase* FallingBlock,
		ABlockBase* HitBlock,
		const FHitResult& HitResult,
		float ContactMoveZ,
		float RemainingMoveBeforeContactZ,
		float RemainingMoveAfterContactZ);
	bool IsBlockPendingCollapse(const ABlockBase* Block) const;
	bool IsBlockFalling(const ABlockBase* Block) const;
	ABlockBase* FindUnsettledBlockAtPosition(const FVector2D& BlockPosition, const ABlockBase* IgnoredBlock) const;

	UPROPERTY(Transient)
	TWeakObjectPtr<APaperTileMapActor> TileMapActor;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ABlockBase>> BlockList;

	UPROPERTY(Transient)
	TArray<FFallingBlockGroupData> FallingBlockGroupList;

	TArray<TArray<TWeakObjectPtr<ABlockBase>>> PendingCollapseBlockLists;

	TMap<FVector2D, TWeakObjectPtr<ABlockBase>> BlockActorMap;
	TMap<FIntPoint, int32> FixedTileTemplate;
	TMap<FIntPoint, int32> MineableTileTemplate;

	int32 LastGenerationSeed = INDEX_NONE;
	bool bTileTemplatesCaptured = false;
	bool bBlockDebugNumbersEnabled = false;
	bool bHasLoggedFallingStopDiagnostic = false;
	bool bHasLoggedDestroyedPendingBlockDiagnostic = false;
	bool bHasLoggedSnapDiagnostic = false;
};
