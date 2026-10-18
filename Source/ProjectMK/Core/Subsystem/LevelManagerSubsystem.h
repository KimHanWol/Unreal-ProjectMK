#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"
#include "LevelManagerSubsystem.generated.h"

class APaperTileMapActor;
class UAbilitySystemComponent;
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

	UPROPERTY()
	float FallingVelocity = 0.f;

	TMap<TWeakObjectPtr<ABlockBase>, FVector> BlockOriginPositionMap;
};

USTRUCT(BlueprintType)
struct FFallingBlockGroupData
{
	GENERATED_BODY()

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
	float CalculateFallingBlockImpactDamage(UObject* WorldContextObject, float FallingVelocity) const;
	bool TryDestroyFallingBlockOnImpact(UObject* WorldContextObject, ABlockBase* FallingBlock, float FallingVelocity) const;
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

	void LoadTileMap();
	void GenerateTileActors();

	UFUNCTION(BlueprintCallable, Category = "Level Generation")
	void RegenerateMineableBlocks(int32 GenerationSeed = -1);
	void ResetMineableLevel(int32 GenerationSeed = -1);

	void SetBlockDebugNumbersEnabled(bool bEnabled);
	int32 GetTileSize();
	const TArray<TObjectPtr<ABlockBase>>& GetBlocks() const;
	ABlockBase* GetBlockAtPosition(const FVector2D& BlockPosition) const;
	/** 월드 위치가 속한 타일 칸의 블록. 캐릭터처럼 칸 중심에 있지 않은 위치에 쓴다 */
	ABlockBase* GetBlockAtWorldLocation(const FVector& WorldLocation) const;
	/** WorldLocation 이 속한 칸 바로 아래 칸의 블록 (캐릭터가 밟고 있는 블록) */
	ABlockBase* GetBlockBelowWorldLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Level Generation")
	int32 GetLastGenerationSeed() const { return LastGenerationSeed; }

	bool IsBlockDebugNumbersEnabled() const { return bBlockDebugNumbersEnabled; }

	void AddPendingBlockBreak(ABlockBase* BreakingBlock);
	bool TryPlaceBlock(FName BlockRowName, const FVector& WorldLocation);
	int32 ApplyExplosion(const FVector& WorldLocation, int32 Radius, UAbilitySystemComponent* InstigatorASC);
	bool IsBlockCellEmpty(const FVector& WorldLocation) const;

private:
	friend struct FFallingBlockGroupData;

	void CaptureOriginalTileTemplates(UDataManager* DataManager, UPaperTileMapComponent* TileMapComponent);
	void ClearGeneratedBlocks();
	void ClearWorldItems();
	void GenerateMineableLayout(
		FRandomStream& RandomStream,
		const FMineableLevelGenerationSettings& GenerationSettings,
		int32 RockTileIndex,
		int32 SandTileIndex,
		TMap<FIntPoint, int32>& OutGeneratedMineableTiles,
		TMap<FIntPoint, TPair<FName, int32>>& OutRewardTiles,
		TMap<FIntPoint, float>& OutPreDamageTiles) const;
	ABlockBase* SpawnTileActor(
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
	bool ApplyFallingImpactToSettledBlocks(ABlockBase* HitBlock, float ImpactDamage);
	void ApplyPendingBlockSounds();
	ESFXType CalculateBlockCountSFXType(int32 BlockCount, ESFXType SingleSFXType, ESFXType MediumSFXType, ESFXType LargeSFXType) const;
	ABlockBase* FindUnsettledBlockAtPosition(const FVector2D& BlockPosition, const ABlockBase* IgnoredBlock) const;
	bool TryGetTileCenterWorldLocation(const FVector& WorldLocation, FIntPoint& OutTileCoordinate, FVector& OutTileCenterWorldLocation) const;

	UPROPERTY(Transient)
	TWeakObjectPtr<APaperTileMapActor> TileMapActor;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ABlockBase>> BlockList;

	UPROPERTY(Transient)
	TArray<FFallingBlockGroupData> FallingBlockGroupList;

	TArray<TArray<TWeakObjectPtr<ABlockBase>>> PendingCollapseBlockLists;

	TMap<FVector2D, TWeakObjectPtr<ABlockBase>> BlockActorMap;
	TMap<FIntPoint, int32> OriginalFixedTileTemplate;
	TMap<FIntPoint, int32> OriginalMineableTileTemplate;

	int32 LastGenerationSeed = INDEX_NONE;
	int32 ShopTileIndex = INDEX_NONE;
	int32 PendingMinedBlockCount = 0;
	int32 PendingImpactBrokenBlockCount = 0;
	int32 PendingLandedBlockCount = 0;
	bool bOriginalTileTemplatesCaptured = false;
	bool bIsApplyingFallingImpact = false;
	bool bBlockDebugNumbersEnabled = false;
	bool bHasLoggedFallingStopDiagnostic = false;
	bool bHasLoggedDestroyedPendingBlockDiagnostic = false;
	bool bHasLoggedSnapDiagnostic = false;
};
