// LINK

#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"

#include "EngineUtils.h"
#include "PaperTileLayer.h"
#include "PaperTileMap.h"
#include "PaperTileMapActor.h"
#include "PaperTileMapComponent.h"
#include "PaperTileSet.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Core/Manager/DataManager.h"

void ULevelManagerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    LoadTileMap();
    GenerateTileActors();
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

                TileMapIndices.Add(FVector2D(X, Y), TileIndex);
            }
        }
    }
}

void ULevelManagerSubsystem::GenerateTileActors()
{
    if (TileMapIndices.IsEmpty())
    {
        return;
    }

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

            ABlockBase* SpawnedActor = GetWorld()->SpawnActor<ABlockBase>(ABlockBase::StaticClass(), WorldPos, FRotator::ZeroRotator, SpawnParams);

            FBlockData BlockData;
            BlockData.TileSetIndex = TileInfo.GetTileIndex();
            BlockData.TileSize = FIntPoint(TileMap->TileWidth, TileMap->TileHeight);

            const FVector& LocalPos = TileMapComp->GetTileCenterPosition(X, Y, 0);
            BlockData.WorldLocation = TileMapComp->GetComponentTransform().TransformPosition(LocalPos);

            SpawnedActor->InitializeBlock(BlockData);
        }
    }

    OnGenerateFinished();
}

void ULevelManagerSubsystem::OnGenerateFinished()
{
    if (TileMapActor.IsValid() == false)
    {
        return;
    }

    TileMapActor->SetActorHiddenInGame(true);
}
