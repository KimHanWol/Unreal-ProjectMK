// LINK

#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"

#include "EngineUtils.h"
#include "PaperTileLayer.h"
#include "PaperTileMap.h"
#include "PaperTileMapActor.h"
#include "PaperTileMapComponent.h"
#include "PaperTileSet.h"
#include "Components/BoxComponent.h"
#include "Engine/OverlapResult.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"
#include "ProjectMK/Helper/Utils/DamageableUtil.h"

namespace
{
	int32 GetCharacterBlockColumn(const AActor* CharacterActor)
	{
		if (::IsValid(CharacterActor) == false)
		{
			return INDEX_NONE;
		}

		return FMath::RoundToInt(CharacterActor->GetActorLocation().X / BLOCK_SIZE);
	}

	bool DetectFallingBlockObstacle(
		ABlockBase* BottomBlock,
		const TArray<TWeakObjectPtr<ABlockBase>>& FallingBlocks,
		const FVector& TargetLocation,
		TArray<TWeakObjectPtr<AActor>>& OutHitActors)
	{
		if (::IsValid(BottomBlock) == false)
		{
			return false;
		}

		UBoxComponent* BoxComponent = BottomBlock->FindComponentByClass<UBoxComponent>();
		if (::IsValid(BoxComponent) == false)
		{
			return false;
		}

		UWorld* World = BottomBlock->GetWorld();
		if (::IsValid(World) == false)
		{
			return false;
		}

		const int32 BottomBlockPositionX = UMKBlueprintFunctionLibrary::GetBlockPosition(BottomBlock).X;
		const FVector BoxExtent = BoxComponent->GetScaledBoxExtent();

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FallingBlockSweep), false);
		QueryParams.AddIgnoredActor(BottomBlock);
		for (const TWeakObjectPtr<ABlockBase>& FallingBlock : FallingBlocks)
		{
			if (FallingBlock.IsValid())
			{
				QueryParams.AddIgnoredActor(FallingBlock.Get());
			}
		}

		FCollisionObjectQueryParams CharacterOverlapQueryParams;
		CharacterOverlapQueryParams.AddObjectTypesToQuery(ECC_Pawn);

		TArray<FOverlapResult> OverlapResults;
		if (World->OverlapMultiByObjectType(
			OverlapResults,
			BottomBlock->GetActorLocation(),
			FQuat::Identity,
			CharacterOverlapQueryParams,
			FCollisionShape::MakeBox(BoxExtent),
			QueryParams))
		{
			for (const FOverlapResult& OverlapResult : OverlapResults)
			{
				AActor* OverlappedActor = OverlapResult.GetActor();
				AMKCharacter* OverlappedCharacter = Cast<AMKCharacter>(OverlappedActor);
				if (::IsValid(OverlappedCharacter) &&
					GetCharacterBlockColumn(OverlappedCharacter) == BottomBlockPositionX)
				{
					OutHitActors.AddUnique(OverlappedActor);
					return true;
				}
			}
		}

		FCollisionObjectQueryParams SweepObjectQueryParams;
		SweepObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		SweepObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		SweepObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

		TArray<FHitResult> SweepHitResults;
		const bool bHasSweepHit = World->SweepMultiByObjectType(
			SweepHitResults,
			BottomBlock->GetActorLocation(),
			TargetLocation,
			FQuat::Identity,
			SweepObjectQueryParams,
			FCollisionShape::MakeBox(BoxExtent),
			QueryParams);

		if (bHasSweepHit == false)
		{
			return false;
		}

		for (const FHitResult& SweepHitResult : SweepHitResults)
		{
			AActor* HitActor = SweepHitResult.GetActor();
			if (::IsValid(HitActor) == false)
			{
				continue;
			}

			const bool bIsSameHeightBlock =
				Cast<ABlockBase>(HitActor) &&
				FMath::IsNearlyEqual(HitActor->GetActorLocation().Z, BottomBlock->GetActorLocation().Z);
			if (bIsSameHeightBlock)
			{
				continue;
			}

			ABlockBase* HitBlock = Cast<ABlockBase>(HitActor);
			if (::IsValid(HitBlock) &&
				UMKBlueprintFunctionLibrary::GetBlockPosition(HitBlock).X == BottomBlockPositionX)
			{
				OutHitActors.AddUnique(HitActor);
				continue;
			}

			AMKCharacter* HitCharacter = Cast<AMKCharacter>(HitActor);
			if (::IsValid(HitCharacter) &&
				GetCharacterBlockColumn(HitCharacter) == BottomBlockPositionX)
			{
				OutHitActors.AddUnique(HitActor);
			}
		}

		return OutHitActors.IsEmpty() == false;
	}
}

void FFallingBlockGroupData::FinishFallingBlock(int32 FallFinishedBlockPositionX)
{
    FallingBlockMap.Remove(FallFinishedBlockPositionX);
}

void FFallingBlockGroupData::Tick_ShakeBlocks(float DeltaTime)
{
    if (IsShaking() == false)
    {
        return;
    }

    ShakeAccumulatedTime += DeltaTime;
    ShakeTime += DeltaTime;

    while (ShakeAccumulatedTime >= ShakeInterval)
    {
        ShakeAccumulatedTime -= ShakeInterval;
    }

    TArray<FFallingVerticalBlocksData> VerticalBlocksDataList;
    FallingBlockMap.GenerateValueArray(VerticalBlocksDataList);

    FVector MoveVector = FVector(FMath::FRandRange(-2.f, 2.f), 0, 0);
    for (const auto& VerticalBlocksData : VerticalBlocksDataList)
    {
        for (const auto& FallingBlock : VerticalBlocksData.FallingBlocks)
        {
            if (FallingBlock.IsValid() == false)
            {
                return;
            }

            const FVector* OriginPositionPtr = VerticalBlocksData.BlockOriginPositionMap.Find(FallingBlock);
            FVector NewActorLocation = (*OriginPositionPtr);
            if (IsShaking())
            {
                NewActorLocation += MoveVector;
            }
            FallingBlock->SetActorLocation(NewActorLocation);
        }
    }
}

void FFallingBlockGroupData::Tick_FallBlocks(float DeltaTime)
{
    if (IsFallingFinished())
    {
        return;
    }

	TArray<FFallingVerticalBlocksData> VerticalBlockDataList;
    FallingBlockMap.GenerateValueArray(VerticalBlockDataList);
	for (const auto& VerticalBlockData : VerticalBlockDataList)
	{
        TWeakObjectPtr<ABlockBase> BottomBlock = VerticalBlockData.BottomBlock;
		if (BottomBlock.IsValid() == false)
		{
			continue;
		}

		const FVector& BlockLocation = BottomBlock->GetActorLocation();
		FVector TargetLocation = BlockLocation + FVector(0.f, 0.f, -8.f);

		TArray<TWeakObjectPtr<AActor>> HitActors;
		bool bIsOnBottom = false;
		if (DetectFallingBlockObstacle(BottomBlock.Get(), VerticalBlockData.FallingBlocks, TargetLocation, HitActors))
		{
			for (const auto& HitActorPtr : HitActors)
			{
				AActor* HitActor = HitActorPtr.Get();
				if (Cast<ABlockBase>(HitActor))
				{
					bIsOnBottom = true;
				}

                AMKCharacter* HitCharacter = Cast<AMKCharacter>(HitActor);
				if (::IsValid(HitCharacter))
				{
					bIsOnBottom = true;
                    const int32 FallingDamage = GetFallingDamage(BottomBlock.Get(), VerticalBlockData);
                    if (FallingDamage > 0.f)
                    {
                        FDamageableUtil::ApplyDamage(HitCharacter->GetAbilitySystemComponent(), nullptr, static_cast<float>(FallingDamage));
                    }
				}
			}
		}

        if (bIsOnBottom)
        {
            FFallingVerticalBlocksData* FallFinishedVerticalBlocksPtr = FallingBlockMap.Find(UMKBlueprintFunctionLibrary::GetBlockPosition(BottomBlock.Get()).X);
            if (FallFinishedVerticalBlocksPtr)
            {
                FallFinishedVerticalBlocksPtr->bFallFinished = true;
            }
        }
	}

	FallingVelocity += 9.8f * 0.1f;
	//FallingVelocity = FMath::Max(FallingSpeed, MaxFallSpeed);

	FVector MoveVector = FVector::Zero();
	MoveVector.Z -= FallingVelocity * 0.1f;

    TArray<FFallingVerticalBlocksData> FallingVerticalBlockDataList;
    FallingBlockMap.GenerateValueArray(FallingVerticalBlockDataList);
    for (const auto& FallingVerticalBlockData : FallingVerticalBlockDataList)
    {
        if (FallingVerticalBlockData.bFallFinished)
        {
            continue;
        }

        for (const auto& FallingBlock : FallingVerticalBlockData.FallingBlocks)
        {
            if (FallingBlock.IsValid() == false)
            {
                return;
            }

            FallingBlock->SetActorLocation(FallingBlock->GetActorLocation() + MoveVector);
        }

        FFallingVerticalBlocksData* MutableVerticalBlockData = FallingBlockMap.Find(UMKBlueprintFunctionLibrary::GetBlockPosition(FallingVerticalBlockData.BottomBlock.Get()).X);
        if (MutableVerticalBlockData)
        {
            MutableVerticalBlockData->FallenDistance += FMath::Abs(MoveVector.Z);
        }
    }
}

int32 FFallingBlockGroupData::GetFallingDamage(UObject* WorldContextObject, const FFallingVerticalBlocksData& VerticalBlockData) const
{
    UDataManager* DataManager = UDataManager::Get(WorldContextObject);
    if (::IsValid(DataManager) == false)
    {
        return 0;
    }

    const UGameSettingDataAsset* GameSettingDataAsset = DataManager->GetGameSettingDataAsset();
    if (::IsValid(GameSettingDataAsset) == false)
    {
        return 0;
    }

    const float RawDamage = (BlockCount * GameSettingDataAsset->FallingBlockDamagePerBlock * VerticalBlockData.FallenDistance) / 100.f;
    if (RawDamage > 0.f && RawDamage < 1.f)
    {
        return 1;
    }

    return FMath::FloorToInt(RawDamage);
}

void ULevelManagerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    LoadTileMap();
    GenerateTileActors();
}

void ULevelManagerSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (FallingBlockGroupList.IsEmpty())
    {
        return;
    }

    for (int32 Index = FallingBlockGroupList.Num() - 1; Index >= 0; --Index)
    {
        FFallingBlockGroupData& FallingBlockGroupData = FallingBlockGroupList[Index];


        TArray<FFallingVerticalBlocksData> VerticalBlockList;
        FallingBlockGroupData.FallingBlockMap.GenerateValueArray(VerticalBlockList);
        for (auto It = FallingBlockGroupData.FallingBlockMap.CreateIterator(); It; ++It)
        {
            FFallingVerticalBlocksData VerticalBlockData = It.Value();
            if (VerticalBlockData.bFallFinished)
            {
                SnapBlocks(VerticalBlockData.FallingBlocks);
                It.RemoveCurrent();
                continue;
            }
        }

        if (FallingBlockGroupData.IsFallingFinished())
        {
            FallingBlockGroupList.RemoveAt(Index);
            return;
        }

        if (FallingBlockGroupData.IsShaking())
        {
            FallingBlockGroupData.Tick_ShakeBlocks(DeltaTime);
        }
        else
        {
            FallingBlockGroupData.Tick_FallBlocks(DeltaTime);
        }
    }
}

TStatId ULevelManagerSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UMyWorldSubsystem, STATGROUP_Tickables);
}

void ULevelManagerSubsystem::LoadTileMap(bool bForce /*=false*/)
{
    if (TileMapActor.IsValid() == false || bForce)
    {
        for (TActorIterator<APaperTileMapActor> It(GetWorld()); It; ++It)
        {
            APaperTileMapActor* InTileMapActor = *It;
            TileMapActor = InTileMapActor;
            break;
        }
    }

    if (TileMapActor.IsValid() == false)
    {
        ensureMsgf(false, TEXT("Can't load tile map : Can't find tile map in recent level."));
        return;
    }

    UPaperTileMapComponent* TileMapComp = TileMapActor->GetRenderComponent();
    if (::IsValid(TileMapComp) == false)
    {
        return;
    }

    UPaperTileMap* TileMap = TileMapComp->TileMap;
    if (::IsValid(TileMap) == false)
    {
        return;
    }

    int32 Width = TileMap->MapWidth;
    int32 Height = TileMap->MapHeight;
    int32 NumLayers = TileMap->TileLayers.Num();
    
    for (int32 LayerIndex = 0; LayerIndex < NumLayers; ++LayerIndex)
    {
        for (int32 Y = 0; Y < Height; ++Y)
        {
            for (int32 X = 0; X < Width; ++X)
            {
                FPaperTileInfo TileInfo = TileMapComp->GetTile(X, Y, LayerIndex);
                int32 TileIndex = TileInfo.PackedTileIndex & 0xFFFF; // 타일 인덱스, 플립 정보, 기타 설정을 압축해 표현한거라 비트 연산으로 index 추출
            }
        }
    }
}

void ULevelManagerSubsystem::GenerateTileActors()
{
    if (TileMapActor.IsValid() == false)
    {
        return;
    }

    UPaperTileMapComponent* TileMapComp = TileMapActor->GetRenderComponent();
    if (::IsValid(TileMapComp) == false)
    {
        return;
    }

    UPaperTileMap* TileMap = TileMapComp->TileMap;
    if (::IsValid(TileMap) == false)
    {
        return;
    }

    UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        return;
    }

    TSubclassOf<AActor> BlockClass = DataManager->GetBlueprintClass(EBlueprintClassType::BlockBase);

    FVector Origin = TileMapComp->GetComponentLocation();
    float TileWidth = TileMap->TileWidth / TileMap->PixelsPerUnrealUnit;
    float TileHeight = TileMap->TileHeight / TileMap->PixelsPerUnrealUnit;
    int32 MapWidth = TileMap->MapWidth;
    int32 MapHeight = TileMap->MapHeight;

    for (int32 Y = 0; Y < MapHeight; ++Y)
    {
        for (int32 X = 0; X < MapWidth; ++X)
        {
			FPaperTileInfo TileInfo = TileMapComp->GetTile(X, Y, 0);
            if (TileInfo.TileSet == nullptr)
            {
                continue;
            }

            const FVector& WorldPos = Origin + FVector(X * TileWidth, 0.f, -Y * TileHeight);

            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            ABlockBase* SpawnedBlock = GetWorld()->SpawnActor<ABlockBase>(BlockClass, WorldPos, FRotator::ZeroRotator, SpawnParams);

            FBlockTileData BlockData;
            BlockData.TileSetIndex = TileInfo.GetTileIndex();
            BlockData.TileSize = FIntPoint(TileMap->TileWidth, TileMap->TileHeight);
            BlockData.OnBlockDestroyedDelegate.AddUObject(this, &ULevelManagerSubsystem::OnBlockDestroyed);

            const FVector &LocalPos = TileMapComp->GetTileCenterPosition(X, Y, 0);
            BlockData.WorldLocation = TileMapComp->GetComponentTransform().TransformPosition(LocalPos);

            SpawnedBlock->InitializeBlock(BlockData);

            BlockActorMap.Add(FVector2D(X, Y), SpawnedBlock);
        }
    }

    OnGenerateFinished();
}

void ULevelManagerSubsystem::SetBlockDebugNumbersEnabled(bool bEnabled)
{
    bBlockDebugNumbersEnabled = bEnabled;
}

int32 ULevelManagerSubsystem::GetTileSize()
{
    if (TileMapActor.IsValid() == false)
    {
        return 0;
    }

    UPaperTileMapComponent* TileMapComp = TileMapActor->GetRenderComponent();
    if (::IsValid(TileMapComp) == false)
    {
        return 0;
    }

    UPaperTileMap* TileMap = TileMapComp->TileMap;
    if (::IsValid(TileMap) == false)
    {
        return 0;
    }

    //가로 세로 동일하다고 가정
    return TileMap->TileWidth;
}

void ULevelManagerSubsystem::OnGenerateFinished()
{
    if (TileMapActor.IsValid() == false)
    {
        return;
    }

    TileMapActor->SetActorHiddenInGame(true);
}

void ULevelManagerSubsystem::OnBlockDestroyed(ABlockBase* DestroyedBlock)
{
    if (::IsValid(DestroyedBlock) == false)
    {
        return;
    }

    const FVector2D& BlockPosition = UMKBlueprintFunctionLibrary::GetBlockPosition(DestroyedBlock);
    BlockActorMap.Remove(BlockPosition);

	TArray<ABlockBase*> DFSCheckedBlocks;
    const TArray<ABlockBase*>& SurroundBlocks = GetSurroundBlocks(DestroyedBlock);
    for (const auto& SurroundBlock : SurroundBlocks)
    {
        TArray<ABlockBase*> DisconnectedBlocks;
        if (CheckBlockIsAllDisconnected(SurroundBlock, DisconnectedBlocks, DFSCheckedBlocks, 0))
        {
            CollapseBlocks(DisconnectedBlocks);
        }
    }
}

bool ULevelManagerSubsystem::CheckBlockIsAllDisconnected(ABlockBase* StartBlock, TArray<ABlockBase*>& OutDisconnectedBlocks, TArray<ABlockBase*>& DFSCheckedBlocks, int32 DebugCount)
{
    //DFS
    if (DFSCheckedBlocks.Contains(StartBlock))
    {
        return true;
    }
    DFSCheckedBlocks.Add(StartBlock);

    if (::IsValid(StartBlock) == false)
    {
        return true;
    }

    if (StartBlock->IsMineable() == false)
    {
        return false;
    }

    if (bBlockDebugNumbersEnabled)
    {
        StartBlock->BP_EnableDebugState(DebugCount);
    }

    OutDisconnectedBlocks.Add(StartBlock);

    const TArray<ABlockBase*>& SurroundBlocks = GetSurroundBlocks(StartBlock);
    for (const auto& SurroundBlock : SurroundBlocks)
    {
        if (CheckBlockIsAllDisconnected(SurroundBlock, OutDisconnectedBlocks, DFSCheckedBlocks, DebugCount + 1) == false)
        {
            return false;
        }
    }

    return true;
}

TArray<ABlockBase*> ULevelManagerSubsystem::GetSurroundBlocks(ABlockBase* TargetBlock)
{
    TArray<ABlockBase*> SurroundBlocks;
    if (::IsValid(TargetBlock) == false)
    {
        return SurroundBlocks;
    }

    const FVector2D& TargetBlockPosition = UMKBlueprintFunctionLibrary::GetBlockPosition(TargetBlock);

    TArray<FVector2D> SurroundBlockPositions;
    SurroundBlockPositions.Add(TargetBlockPosition + FVector2D(0, 1));
    SurroundBlockPositions.Add(TargetBlockPosition + FVector2D(0, -1));
    SurroundBlockPositions.Add(TargetBlockPosition + FVector2D(1, 0));
    SurroundBlockPositions.Add(TargetBlockPosition + FVector2D(-1, 0));

    for (const auto SurroundBlockPosition : SurroundBlockPositions)
    {
        TWeakObjectPtr<ABlockBase>* SurroundBlockPtr = BlockActorMap.Find(SurroundBlockPosition);
        if (SurroundBlockPtr == nullptr)
        {
            continue;
        }

        ABlockBase* SurroundBlock = (*SurroundBlockPtr).Get();
        if (::IsValid(SurroundBlock))
        {
            SurroundBlocks.Add(SurroundBlock);
        }
    }

    return SurroundBlocks;
}

void ULevelManagerSubsystem::CollapseBlocks(const TArray<ABlockBase*>& CollapsingBlocks)
{
    TMap<int32, FFallingVerticalBlocksData> VerticalBlocksMap;
    for (auto& CollapsingBlock : CollapsingBlocks)
    {
        if (::IsValid(CollapsingBlock) == false)
        {
            continue;
        }

        CollapsingBlock->SetMineableState(false);

        const FVector2D& NewBlockPosition = UMKBlueprintFunctionLibrary::GetBlockPosition(CollapsingBlock);
        FFallingVerticalBlocksData* VerticalBlocksDataPtr = VerticalBlocksMap.Find(NewBlockPosition.X);
        if (VerticalBlocksDataPtr != nullptr)
        {
            //BlockPosition 이라 반대
            if (UMKBlueprintFunctionLibrary::GetBlockPosition(VerticalBlocksDataPtr->BottomBlock.Get()).Y < NewBlockPosition.Y)
            {
                VerticalBlocksDataPtr->BottomBlock = CollapsingBlock;
            }
            VerticalBlocksDataPtr->FallingBlocks.Add(CollapsingBlock);
            VerticalBlocksDataPtr->BlockOriginPositionMap.Add(CollapsingBlock, CollapsingBlock->GetActorLocation());
        }
        else
        {
            FFallingVerticalBlocksData VerticalBlocksData;
            VerticalBlocksData.BottomBlock = CollapsingBlock;
            VerticalBlocksData.FallingBlocks.Add(CollapsingBlock);
            VerticalBlocksData.BlockOriginPositionMap.Add(CollapsingBlock, CollapsingBlock->GetActorLocation());

            VerticalBlocksMap.Add(NewBlockPosition.X, VerticalBlocksData);
        }
    }

    FFallingBlockGroupData FallingBlockGroupData;
    FallingBlockGroupData.FallingBlockMap = VerticalBlocksMap;
    FallingBlockGroupData.BlockCount = CollapsingBlocks.Num();

    FallingBlockGroupList.Add(FallingBlockGroupData);
}

void ULevelManagerSubsystem::SnapBlocks(const TArray<TWeakObjectPtr<ABlockBase>>& FallFinishedBlockList)
{
    for (auto& FallFinishedBlock : FallFinishedBlockList)
    {
        if (FallFinishedBlock.IsValid() == false)
        {
            continue;
        }

        const FVector SnapSampleLocation = FallFinishedBlock->GetActorLocation() + FVector(0.f, 0.f, -(BLOCK_SIZE * 0.5f));
        const FVector SnapLocation = UMKBlueprintFunctionLibrary::GetSnappingWorldPosition(SnapSampleLocation);
        FallFinishedBlock->SetActorLocation(SnapLocation);
        FallFinishedBlock->SetMineableState(true);

        const FVector2D& BlockPosition = UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(SnapLocation);
        BlockActorMap.Add(BlockPosition, TWeakObjectPtr<ABlockBase>(FallFinishedBlock));
    }
}

bool FFallingBlockGroupData::IsFallingFinished()
{
    TArray<FFallingVerticalBlocksData> VerticalBlockList;
    FallingBlockMap.GenerateValueArray(VerticalBlockList);
    for (const auto& VerticalBlockData : VerticalBlockList)
    {
        if (VerticalBlockData.bFallFinished == false)
        {
            return false;
        }
    }

    return true;
}

TWeakObjectPtr<ABlockBase> FFallingBlockGroupData::GetBottomBlock(int32 BlockPositionX)
{
    FFallingVerticalBlocksData* VerticalBlocksPtr = FallingBlockMap.Find(BlockPositionX);
    if (VerticalBlocksPtr == nullptr)
    {
        return nullptr;
    }

    return VerticalBlocksPtr->BottomBlock;
}

TArray<ABlockBase*> FFallingBlockGroupData::GetFallingBlocks()
{
    TArray<ABlockBase*> FallingBlockList;

    TArray<FFallingVerticalBlocksData> VerticalBlockList;
    FallingBlockMap.GenerateValueArray(VerticalBlockList);

    for (const auto& VerticalBlockData : VerticalBlockList)
    {
        if (VerticalBlockData.bFallFinished == false)
        {
            for (const auto& FallingBlock : VerticalBlockData.FallingBlocks)
            {
                FallingBlockList.Add(FallingBlock.Get());
            }
        }
    }

    return FallingBlockList;
}
