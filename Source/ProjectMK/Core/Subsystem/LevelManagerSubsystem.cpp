// LINK

#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "PaperTileLayer.h"
#include "PaperTileMap.h"
#include "PaperTileMapActor.h"
#include "PaperTileMapComponent.h"
#include "PaperTileSet.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/ShopActor.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Block.h"
#include "ProjectMK/Core/MKSaveGame.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Manager/SoundManager.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/Data/DataAsset/BlueprintDataAsset.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"
#include "ProjectMK/Helper/Utils/BlockUtils.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"
#include "ProjectMK/System/GlobalConstants.h"

namespace
{
	bool FindNearestFallingBlockHit(
		ABlockBase* FallingBlock,
		const TArray<TWeakObjectPtr<ABlockBase>>& IgnoredFallingBlocks,
		const FVector& TargetLocation,
		FHitResult& InOutNearestHit,
		TWeakObjectPtr<ABlockBase>& InOutNearestFallingBlock)
	{
		if (::IsValid(FallingBlock) == false)
		{
			return false;
		}

		UBoxComponent* BoxComponent = FallingBlock->FindComponentByClass<UBoxComponent>();
		if (::IsValid(BoxComponent) == false)
		{
			return false;
		}

		UWorld* World = FallingBlock->GetWorld();
		if (::IsValid(World) == false)
		{
			return false;
		}

		const FVector BlockLocation = FallingBlock->GetActorLocation();
		const FVector BoxExtent = BoxComponent->GetScaledBoxExtent();
		const int32 FallingBlockPositionX = UMKBlueprintFunctionLibrary::GetBlockPosition(FallingBlock).X;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FallingBlockSweep), false);
		for (const TWeakObjectPtr<ABlockBase>& IgnoredBlock : IgnoredFallingBlocks)
		{
			if (IgnoredBlock.IsValid())
			{
				QueryParams.AddIgnoredActor(IgnoredBlock.Get());
			}
		}

		FCollisionObjectQueryParams SweepObjectQueryParams;
		SweepObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
		SweepObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		SweepObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

		TArray<FHitResult> SweepHitResults;
		const bool bHasSweepHit = World->SweepMultiByObjectType(
			SweepHitResults,
			BlockLocation,
			TargetLocation,
			FQuat::Identity,
			SweepObjectQueryParams,
			FCollisionShape::MakeBox(BoxExtent),
			QueryParams);

		if (bHasSweepHit == false)
		{
			return ::IsValid(InOutNearestHit.GetActor());
		}

		for (const FHitResult& SweepHitResult : SweepHitResults)
		{
			AActor* HitActor = SweepHitResult.GetActor();
			if (::IsValid(HitActor) == false)
			{
				continue;
			}

			ABlockBase* HitBlock = Cast<ABlockBase>(HitActor);
			const bool bIsBlockInFallingColumn =
				::IsValid(HitBlock) &&
				UMKBlueprintFunctionLibrary::GetBlockPosition(HitBlock).X == FallingBlockPositionX;
			AMKCharacter* HitCharacter = Cast<AMKCharacter>(HitActor);
			const bool bIsCharacterCapsule =
				::IsValid(HitCharacter) &&
				SweepHitResult.GetComponent() == HitCharacter->GetCapsuleComponent();
			if (bIsBlockInFallingColumn == false && bIsCharacterCapsule == false)
			{
				continue;
			}

			if (::IsValid(InOutNearestHit.GetActor()) == false || SweepHitResult.Time < InOutNearestHit.Time)
			{
				InOutNearestHit = SweepHitResult;
				InOutNearestFallingBlock = FallingBlock;
			}
		}

		return ::IsValid(InOutNearestHit.GetActor());
	}

	bool DetectFallingBlockObstacle(
		ABlockBase* BottomBlock,
		const TArray<TWeakObjectPtr<ABlockBase>>& VerticalFallingBlocks,
		const TArray<TWeakObjectPtr<ABlockBase>>& GroupFallingBlocks,
		const FVector& TargetLocation,
		FHitResult& OutNearestHit,
		TWeakObjectPtr<ABlockBase>& OutNearestFallingBlock)
	{
		if (::IsValid(BottomBlock) == false)
		{
			return false;
		}

		if (::IsValid(BottomBlock->FindComponentByClass<UBoxComponent>()) == false)
		{
			return false;
		}

		if (::IsValid(BottomBlock->GetWorld()) == false)
		{
			return false;
		}

		FindNearestFallingBlockHit(
			BottomBlock,
			GroupFallingBlocks,
			TargetLocation,
			OutNearestHit,
			OutNearestFallingBlock);

		const FVector BottomToTargetOffset = TargetLocation - BottomBlock->GetActorLocation();
		for (const TWeakObjectPtr<ABlockBase>& FallingBlock : VerticalFallingBlocks)
		{
			if (FallingBlock.IsValid() == false || FallingBlock.Get() == BottomBlock)
			{
				continue;
			}

			const FVector BlockTargetLocation = FallingBlock->GetActorLocation() + BottomToTargetOffset;
			FindNearestFallingBlockHit(
				FallingBlock.Get(),
				GroupFallingBlocks,
				BlockTargetLocation,
				OutNearestHit,
				OutNearestFallingBlock);
		}

		return ::IsValid(OutNearestHit.GetActor());
	}

	constexpr int32 RockClusterMaxSize = 3;
	constexpr int32 PlacementAttemptLimit = 512;
	constexpr int32 MineableBlocksPerAnchor = 24;
	constexpr int32 FallingStructureSupportCountMax = 4;
	constexpr int32 FallingStructureTargetThicknessMin = 2;
	constexpr int32 FallingStructureTargetThicknessMax = 4;

	const FIntPoint CardinalDirections[] =
	{
		FIntPoint(-1, 0),
		FIntPoint(1, 0),
		FIntPoint(0, -1),
		FIntPoint(0, 1)
	};

	// 구조물별 의도는 AGENT/Design/FallingStructures.md 참고
	enum class EFallingPatternType : uint8
	{
		TopHammer,
		SideOverhang,
		KeystoneWings,
		TriggerCascade,
		Stalactites,
		OreVault,
		HarvestSlab,
		BaitTrap,
		Chandelier,
		Hourglass,
		CascadeLedges,
		LoadBridge,
		Breakthrough,
		RockSpine,
		RockUmbrella,
		RockAnvil,
		RockFunnel,
		Count
	};

	struct FFallingPatternStamp
	{
		TSet<FIntPoint> BodyCells;
		TSet<FIntPoint> SupportCells;
		TSet<FIntPoint> RockAnchorCells;
		TSet<FIntPoint> RequiredEmptyCells;
		TSet<FIntPoint> TargetCells;
		TSet<FIntPoint> ReservedCells;
		/** 보상 광물을 심을 칸. Body / Support / Target 중 하나여야 한다 */
		TSet<FIntPoint> RewardCells;
		/** 템플릿 대신 Sand 블록으로 채울 Body 칸 */
		TSet<FIntPoint> SandCells;
		/** 연쇄 붕괴를 설계하기 위해 생성 시 미리 깎아 둘 내구도 (최대 내구도 대비 비율) */
		TMap<FIntPoint, float> PreDamageRatios;
	};

	void AddPreDamageCells(FFallingPatternStamp& PatternStamp, const TSet<FIntPoint>& Cells, float MinRatio, float MaxRatio, FRandomStream& RandomStream)
	{
		TArray<FIntPoint> SortedCells = Cells.Array();
		SortedCells.Sort([](const FIntPoint& Left, const FIntPoint& Right)
		{
			return Left.Y == Right.Y ? Left.X < Right.X : Left.Y < Right.Y;
		});

		for (const FIntPoint& TileCoordinate : SortedCells)
		{
			PatternStamp.PreDamageRatios.Add(TileCoordinate, RandomStream.FRandRange(MinRatio, MaxRatio));
		}
	}

	void AddRectangleCells(TSet<FIntPoint>& OutCells, int32 MinX, int32 MinY, int32 MaxX, int32 MaxY)
	{
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				OutCells.Add(FIntPoint(X, Y));
			}
		}
	}

	bool ApplyFallingPatternStamp(
		const FFallingPatternStamp& PatternStamp,
		const TMap<FIntPoint, int32>& MineableTileTemplate,
		TMap<FIntPoint, int32>& InOutGeneratedMineableTiles,
		TSet<FIntPoint>& InOutReservedCoordinates,
		TSet<FIntPoint>& InOutEmptyCoordinates,
		TSet<FIntPoint>& InOutRewardCoordinates,
		TMap<FIntPoint, float>& InOutPreDamageRatios,
		int32 RockTileIndex,
		int32 SandTileIndex,
		int32 MinimumPlacementY)
	{
		TSet<FIntPoint> RequiredCells = PatternStamp.ReservedCells;
		for (const FIntPoint& TileCoordinate : PatternStamp.BodyCells)
		{
			RequiredCells.Add(TileCoordinate);
		}
		for (const FIntPoint& TileCoordinate : PatternStamp.SupportCells)
		{
			RequiredCells.Add(TileCoordinate);
		}
		for (const FIntPoint& TileCoordinate : PatternStamp.RockAnchorCells)
		{
			RequiredCells.Add(TileCoordinate);
		}
		for (const FIntPoint& TileCoordinate : PatternStamp.RequiredEmptyCells)
		{
			RequiredCells.Add(TileCoordinate);
		}
		for (const FIntPoint& TileCoordinate : PatternStamp.TargetCells)
		{
			RequiredCells.Add(TileCoordinate);
		}

		for (const FIntPoint& TileCoordinate : RequiredCells)
		{
			if (TileCoordinate.Y < MinimumPlacementY ||
				MineableTileTemplate.Contains(TileCoordinate) == false ||
				InOutReservedCoordinates.Contains(TileCoordinate))
			{
				return false;
			}
		}

		TSet<FIntPoint> OccupiedCells;
		for (const FIntPoint& TileCoordinate : PatternStamp.BodyCells)
		{
			OccupiedCells.Add(TileCoordinate);
		}
		for (const FIntPoint& TileCoordinate : PatternStamp.SupportCells)
		{
			OccupiedCells.Add(TileCoordinate);
		}
		for (const FIntPoint& TileCoordinate : PatternStamp.RockAnchorCells)
		{
			if (OccupiedCells.Contains(TileCoordinate))
			{
				return false;
			}
			OccupiedCells.Add(TileCoordinate);
		}
		for (const FIntPoint& TileCoordinate : PatternStamp.TargetCells)
		{
			if (PatternStamp.RockAnchorCells.Contains(TileCoordinate))
			{
				return false;
			}
			OccupiedCells.Add(TileCoordinate);
		}

		TSet<FIntPoint> EmptyCells;
		for (const FIntPoint& TileCoordinate : PatternStamp.RequiredEmptyCells)
		{
			if (OccupiedCells.Contains(TileCoordinate) == false)
			{
				EmptyCells.Add(TileCoordinate);
			}
		}

		for (const FIntPoint& TileCoordinate : EmptyCells)
		{
			InOutGeneratedMineableTiles.Remove(TileCoordinate);
			InOutEmptyCoordinates.Add(TileCoordinate);
		}
		for (const FIntPoint& TileCoordinate : PatternStamp.RockAnchorCells)
		{
			InOutGeneratedMineableTiles.Add(TileCoordinate, RockTileIndex);
			InOutEmptyCoordinates.Remove(TileCoordinate);
		}

		for (const FIntPoint& TileCoordinate : PatternStamp.BodyCells)
		{
			const bool bUseSandTile = SandTileIndex != INDEX_NONE && PatternStamp.SandCells.Contains(TileCoordinate);
			InOutGeneratedMineableTiles.Add(TileCoordinate, bUseSandTile ? SandTileIndex : MineableTileTemplate.FindChecked(TileCoordinate));
			InOutEmptyCoordinates.Remove(TileCoordinate);
		}
		for (const FIntPoint& TileCoordinate : PatternStamp.SupportCells)
		{
			InOutGeneratedMineableTiles.Add(TileCoordinate, MineableTileTemplate.FindChecked(TileCoordinate));
			InOutEmptyCoordinates.Remove(TileCoordinate);
		}
		for (const FIntPoint& TileCoordinate : PatternStamp.TargetCells)
		{
			InOutGeneratedMineableTiles.Add(TileCoordinate, MineableTileTemplate.FindChecked(TileCoordinate));
			InOutEmptyCoordinates.Remove(TileCoordinate);
		}

		for (const FIntPoint& TileCoordinate : RequiredCells)
		{
			InOutReservedCoordinates.Add(TileCoordinate);
		}

		for (const FIntPoint& TileCoordinate : PatternStamp.RewardCells)
		{
			if (OccupiedCells.Contains(TileCoordinate) && PatternStamp.RockAnchorCells.Contains(TileCoordinate) == false)
			{
				InOutRewardCoordinates.Add(TileCoordinate);
			}
		}

		for (const TPair<FIntPoint, float>& PreDamagePair : PatternStamp.PreDamageRatios)
		{
			if (OccupiedCells.Contains(PreDamagePair.Key) && PatternStamp.RockAnchorCells.Contains(PreDamagePair.Key) == false)
			{
				InOutPreDamageRatios.Add(PreDamagePair.Key, FMath::Clamp(PreDamagePair.Value, 0.f, 0.95f));
			}
		}
		return true;
	}

	FFallingPatternStamp BuildTopHammerStamp(
		const FIntPoint& CenterCoordinate,
		int32 StructureWidth,
		int32 StructureHeight,
		int32 FallingDistance,
		int32 TargetThickness,
		int32 SupportCount,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 MinX = CenterCoordinate.X - StructureWidth / 2;
		const int32 MinY = CenterCoordinate.Y;
		const int32 MaxX = MinX + StructureWidth - 1;
		const int32 MaxY = MinY + StructureHeight - 1;
		const int32 ShaftBottomY = MaxY + FallingDistance;
		const int32 TargetBottomY = ShaftBottomY + TargetThickness;
		AddRectangleCells(PatternStamp.ReservedCells, MinX - 1, MinY - 2, MaxX + 1, TargetBottomY + 1);

		for (int32 X = MinX; X <= MaxX; ++X)
		{
			const int32 ToothHeight = RandomStream.RandRange(FMath::Min(2, StructureHeight), StructureHeight);
			for (int32 Y = MinY; Y < MinY + ToothHeight; ++Y)
			{
				PatternStamp.BodyCells.Add(FIntPoint(X, Y));
			}
		}

		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				const FIntPoint TileCoordinate(X, Y);
				if (PatternStamp.BodyCells.Contains(TileCoordinate) == false)
				{
					PatternStamp.RequiredEmptyCells.Add(TileCoordinate);
				}
			}
		}

		for (int32 SupportIndex = 0; SupportIndex < SupportCount; ++SupportIndex)
		{
			const int32 SupportX = MinX + ((SupportIndex + 1) * StructureWidth) / (SupportCount + 1);
			PatternStamp.SupportCells.Add(FIntPoint(SupportX, MinY - 1));
			PatternStamp.RockAnchorCells.Add(FIntPoint(SupportX, MinY - 2));
		}

		for (int32 X = MinX - 1; X <= MaxX + 1; ++X)
		{
			const FIntPoint TopBoundaryCoordinate(X, MinY - 1);
			if (PatternStamp.SupportCells.Contains(TopBoundaryCoordinate) == false)
			{
				PatternStamp.RequiredEmptyCells.Add(TopBoundaryCoordinate);
			}
		}
		AddRectangleCells(PatternStamp.RequiredEmptyCells, MinX - 1, MinY, MinX - 1, ShaftBottomY);
		AddRectangleCells(PatternStamp.RequiredEmptyCells, MaxX + 1, MinY, MaxX + 1, ShaftBottomY);
		AddRectangleCells(PatternStamp.RequiredEmptyCells, MinX, MaxY + 1, MaxX, ShaftBottomY);
		AddRectangleCells(PatternStamp.TargetCells, MinX, ShaftBottomY + 1, MaxX, TargetBottomY);
		AddRectangleCells(PatternStamp.RockAnchorCells, MinX, TargetBottomY + 1, MaxX, TargetBottomY + 1);
		return PatternStamp;
	}

	FFallingPatternStamp BuildSideOverhangStamp(
		const FIntPoint& CenterCoordinate,
		int32 StructureWidth,
		int32 StructureHeight,
		int32 FallingDistance,
		int32 TargetThickness,
		bool bAttachFromLeft,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 MinX = CenterCoordinate.X - StructureWidth / 2;
		const int32 MinY = CenterCoordinate.Y;
		const int32 MaxX = MinX + StructureWidth - 1;
		const int32 MaxY = MinY + StructureHeight - 1;
		const int32 ShaftBottomY = MaxY + FallingDistance;
		const int32 TargetBottomY = ShaftBottomY + TargetThickness;
		const int32 AttachedBoundaryX = bAttachFromLeft ? MinX - 1 : MaxX + 1;
		const int32 AnchorX = bAttachFromLeft ? MinX - 2 : MaxX + 2;
		const int32 OppositeBoundaryX = bAttachFromLeft ? MaxX + 1 : MinX - 1;
		AddRectangleCells(
			PatternStamp.ReservedCells,
			bAttachFromLeft ? MinX - 2 : MinX - 1,
			MinY - 1,
			bAttachFromLeft ? MaxX + 1 : MaxX + 2,
			TargetBottomY + 1);

		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			const int32 RowIndex = Y - MinY;
			const int32 MaximumIndent = FMath::Max(0, StructureWidth / 2 - 1);
			const int32 FarEdgeIndent = FMath::Min(MaximumIndent, RandomStream.RandRange(0, RowIndex + 1));
			const int32 RowMinX = bAttachFromLeft ? MinX : MinX + FarEdgeIndent;
			const int32 RowMaxX = bAttachFromLeft ? MaxX - FarEdgeIndent : MaxX;
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				const FIntPoint TileCoordinate(X, Y);
				if (X >= RowMinX && X <= RowMaxX)
				{
					PatternStamp.BodyCells.Add(TileCoordinate);
				}
				else
				{
					PatternStamp.RequiredEmptyCells.Add(TileCoordinate);
				}
			}
		}

		const int32 SupportCount = StructureHeight >= 5 ? 2 : 1;
		for (int32 SupportIndex = 0; SupportIndex < SupportCount; ++SupportIndex)
		{
			const int32 SupportY = MinY + ((SupportIndex + 1) * StructureHeight) / (SupportCount + 1);
			PatternStamp.SupportCells.Add(FIntPoint(AttachedBoundaryX, SupportY));
			PatternStamp.RockAnchorCells.Add(FIntPoint(AnchorX, SupportY));
		}

		AddRectangleCells(PatternStamp.RequiredEmptyCells, MinX, MinY - 1, MaxX, MinY - 1);
		AddRectangleCells(PatternStamp.RequiredEmptyCells, OppositeBoundaryX, MinY, OppositeBoundaryX, ShaftBottomY);
		for (int32 Y = MinY; Y <= ShaftBottomY; ++Y)
		{
			const FIntPoint BoundaryCoordinate(AttachedBoundaryX, Y);
			if (PatternStamp.SupportCells.Contains(BoundaryCoordinate) == false)
			{
				PatternStamp.RequiredEmptyCells.Add(BoundaryCoordinate);
			}
		}
		AddRectangleCells(PatternStamp.RequiredEmptyCells, MinX, MaxY + 1, MaxX, ShaftBottomY);
		AddRectangleCells(PatternStamp.TargetCells, MinX, ShaftBottomY + 1, MaxX, TargetBottomY);
		AddRectangleCells(PatternStamp.RockAnchorCells, MinX, TargetBottomY + 1, MaxX, TargetBottomY + 1);
		return PatternStamp;
	}

	FFallingPatternStamp BuildKeystoneWingsStamp(
		const FIntPoint& CenterCoordinate,
		int32 StructureWidth,
		int32 StructureHeight,
		int32 FallingDistance,
		int32 TargetThickness,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 MinX = CenterCoordinate.X - StructureWidth / 2;
		const int32 MinY = CenterCoordinate.Y;
		const int32 MaxX = MinX + StructureWidth - 1;
		const int32 MaxY = MinY + StructureHeight - 1;
		const int32 CenterX = (MinX + MaxX) / 2;
		const int32 ShaftBottomY = MaxY + FallingDistance;
		const int32 TargetBottomY = ShaftBottomY + TargetThickness;
		AddRectangleCells(PatternStamp.ReservedCells, MinX - 1, MinY - 1, MaxX + 1, TargetBottomY + 1);

		const int32 LeftWingWidth = CenterX - MinX;
		const int32 RightWingWidth = MaxX - CenterX;
		const int32 LeftWingHeight = RandomStream.RandRange(FMath::Min(2, StructureHeight), StructureHeight);
		const int32 RightWingHeight = RandomStream.RandRange(FMath::Min(2, StructureHeight), StructureHeight);
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			if (X == CenterX)
			{
				continue;
			}

			const bool bIsLeftWing = X < CenterX;
			const int32 WingWidth = bIsLeftWing ? LeftWingWidth : RightWingWidth;
			const int32 WingHeight = bIsLeftWing ? LeftWingHeight : RightWingHeight;
			const int32 DistanceFromCenter = FMath::Abs(X - CenterX);
			const int32 HeightFromSlope = 1 + FMath::DivideAndRoundUp(
				FMath::Max(0, WingHeight - 1) * DistanceFromCenter,
				FMath::Max(1, WingWidth));
			const int32 ColumnHeight = FMath::Clamp(
				HeightFromSlope + RandomStream.RandRange(-1, 1),
				1,
				WingHeight);
			for (int32 Y = MinY; Y < MinY + ColumnHeight; ++Y)
			{
				PatternStamp.BodyCells.Add(FIntPoint(X, Y));
			}
		}

		PatternStamp.SupportCells.Add(FIntPoint(CenterX, MinY));
		PatternStamp.RockAnchorCells.Add(FIntPoint(CenterX, MinY - 1));
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				const FIntPoint TileCoordinate(X, Y);
				if (PatternStamp.BodyCells.Contains(TileCoordinate) == false &&
					PatternStamp.SupportCells.Contains(TileCoordinate) == false)
				{
					PatternStamp.RequiredEmptyCells.Add(TileCoordinate);
				}
			}
		}
		for (int32 X = MinX - 1; X <= MaxX + 1; ++X)
		{
			const FIntPoint TopBoundaryCoordinate(X, MinY - 1);
			if (PatternStamp.RockAnchorCells.Contains(TopBoundaryCoordinate) == false)
			{
				PatternStamp.RequiredEmptyCells.Add(TopBoundaryCoordinate);
			}
		}
		AddRectangleCells(PatternStamp.RequiredEmptyCells, MinX - 1, MinY, MinX - 1, ShaftBottomY);
		AddRectangleCells(PatternStamp.RequiredEmptyCells, MaxX + 1, MinY, MaxX + 1, ShaftBottomY);
		AddRectangleCells(PatternStamp.RequiredEmptyCells, MinX, MaxY + 1, MaxX, ShaftBottomY);
		AddRectangleCells(PatternStamp.TargetCells, MinX, ShaftBottomY + 1, MaxX, TargetBottomY);
		AddRectangleCells(PatternStamp.RockAnchorCells, MinX, TargetBottomY + 1, MaxX, TargetBottomY + 1);
		return PatternStamp;
	}

	FFallingPatternStamp BuildTriggerCascadeStamp(
		const FIntPoint& CenterCoordinate,
		int32 StructureWidth,
		int32 StructureHeight,
		int32 FallingDistance,
		int32 TargetThickness,
		bool bTriggerFromLeft,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp = BuildSideOverhangStamp(
			CenterCoordinate,
			StructureWidth,
			StructureHeight,
			FallingDistance,
			TargetThickness,
			bTriggerFromLeft,
			RandomStream);
		PatternStamp.SupportCells.Reset();
		PatternStamp.RockAnchorCells.Reset();

		const int32 MinX = CenterCoordinate.X - StructureWidth / 2;
		const int32 MinY = CenterCoordinate.Y;
		const int32 MaxX = MinX + StructureWidth - 1;
		const int32 TriggerX = bTriggerFromLeft ? MinX - 1 : MaxX + 1;
		const int32 TriggerAnchorX = bTriggerFromLeft ? MinX - 2 : MaxX + 2;
		const int32 LowerSupportY = MinY + StructureHeight / 2;
		const int32 MaxY = MinY + StructureHeight - 1;
		const int32 ShaftBottomY = MaxY + FallingDistance;
		const int32 TargetBottomY = ShaftBottomY + TargetThickness;
		const int32 StrikerHeight = FMath::Clamp(StructureHeight / 2, 2, 4);
		const int32 StrikerDropDistance = FMath::Max(4, FallingDistance / 2);
		const int32 StrikerMaxY = LowerSupportY - StrikerDropDistance - 1;
		const int32 StrikerMinY = StrikerMaxY - StrikerHeight + 1;

		for (int32 Y = MinY; Y <= ShaftBottomY; ++Y)
		{
			PatternStamp.RequiredEmptyCells.Add(FIntPoint(TriggerX, Y));
		}
		const FIntPoint LowerSupportCoordinate(TriggerX, LowerSupportY);
		PatternStamp.RequiredEmptyCells.Remove(LowerSupportCoordinate);
		PatternStamp.SupportCells.Add(LowerSupportCoordinate);
		for (int32 Y = StrikerMaxY + 1; Y <= MaxY + 1; ++Y)
		{
			PatternStamp.RockAnchorCells.Add(FIntPoint(TriggerAnchorX, Y));
		}
		PatternStamp.RockAnchorCells.Add(FIntPoint(TriggerX, MaxY + 1));
		PatternStamp.RockAnchorCells.Add(FIntPoint(TriggerX, TargetBottomY + 1));
		AddRectangleCells(PatternStamp.RockAnchorCells, MinX, TargetBottomY + 1, MaxX, TargetBottomY + 1);
		for (int32 Y = StrikerMinY; Y <= StrikerMaxY; ++Y)
		{
			PatternStamp.BodyCells.Add(FIntPoint(TriggerX, Y));
			PatternStamp.RequiredEmptyCells.Add(FIntPoint(TriggerX - 1, Y));
			PatternStamp.RequiredEmptyCells.Add(FIntPoint(TriggerX + 1, Y));
		}

		const FIntPoint StrikerSupportCoordinate(TriggerX, StrikerMinY - 1);
		const FIntPoint StrikerAnchorCoordinate(TriggerX, StrikerMinY - 2);
		PatternStamp.SupportCells.Add(StrikerSupportCoordinate);
		PatternStamp.RockAnchorCells.Add(StrikerAnchorCoordinate);
		PatternStamp.RequiredEmptyCells.Add(FIntPoint(TriggerX - 1, StrikerMinY - 1));
		PatternStamp.RequiredEmptyCells.Add(FIntPoint(TriggerX + 1, StrikerMinY - 1));
		for (int32 Y = StrikerMaxY + 1; Y < LowerSupportY; ++Y)
		{
			PatternStamp.RequiredEmptyCells.Add(FIntPoint(TriggerX, Y));
		}

		const int32 ReserveMinX = FMath::Min(TriggerAnchorX, TriggerX - 1);
		const int32 ReserveMaxX = FMath::Max(TriggerAnchorX, TriggerX + 1);
		AddRectangleCells(PatternStamp.ReservedCells, ReserveMinX, StrikerMinY - 2, ReserveMaxX, LowerSupportY);
		return PatternStamp;
	}

	/**
	 * Body / Support 를 둘러싼 한 칸을 비워 템플릿 지형과 떨어뜨리고, 아래로 낙하 공간과 바닥을 만든다.
	 * 바닥이 Rock 이면 떨어진 블록이 충돌로 부서지기 쉽고, 채굴 블록(Target)이면 충격이 아래로 전달되며 쌓인다.
	 * 지지용 Rock 앵커와 보상 칸은 호출 전에 채워 둔다.
	 */
	void FinalizeEnclosedStamp(FFallingPatternStamp& PatternStamp, int32 FallingDistance, int32 TargetThickness, bool bRockFloor)
	{
		TSet<FIntPoint> StructureCells = PatternStamp.BodyCells;
		StructureCells.Append(PatternStamp.SupportCells);
		if (StructureCells.IsEmpty())
		{
			return;
		}

		FIntPoint MinCoordinate(MAX_int32, MAX_int32);
		FIntPoint MaxCoordinate(MIN_int32, MIN_int32);
		for (const FIntPoint& TileCoordinate : StructureCells)
		{
			MinCoordinate = MinCoordinate.ComponentMin(TileCoordinate);
			MaxCoordinate = MaxCoordinate.ComponentMax(TileCoordinate);
		}

		const int32 ShaftBottomY = MaxCoordinate.Y + FMath::Max(0, FallingDistance);
		int32 FloorY = ShaftBottomY + 1;
		if (bRockFloor == false && TargetThickness > 0)
		{
			AddRectangleCells(PatternStamp.TargetCells, MinCoordinate.X, ShaftBottomY + 1, MaxCoordinate.X, ShaftBottomY + TargetThickness);
			FloorY = ShaftBottomY + TargetThickness + 1;
		}
		AddRectangleCells(PatternStamp.RockAnchorCells, MinCoordinate.X, FloorY, MaxCoordinate.X, FloorY);

		TSet<FIntPoint> EnclosureCells;
		AddRectangleCells(EnclosureCells, MinCoordinate.X - 1, MinCoordinate.Y - 1, MaxCoordinate.X + 1, MaxCoordinate.Y + 1);
		AddRectangleCells(EnclosureCells, MinCoordinate.X, MaxCoordinate.Y + 1, MaxCoordinate.X, ShaftBottomY);
		for (const FIntPoint& TileCoordinate : EnclosureCells)
		{
			if (StructureCells.Contains(TileCoordinate) == false &&
				PatternStamp.RockAnchorCells.Contains(TileCoordinate) == false &&
				PatternStamp.TargetCells.Contains(TileCoordinate) == false)
			{
				PatternStamp.RequiredEmptyCells.Add(TileCoordinate);
			}
		}

		AddRectangleCells(PatternStamp.ReservedCells, MinCoordinate.X - 1, MinCoordinate.Y - 1, MaxCoordinate.X + 1, FloorY);
	}

	void AddRandomRewardCells(FFallingPatternStamp& PatternStamp, const TSet<FIntPoint>& CandidateCells, float RewardChance, FRandomStream& RandomStream)
	{
		TArray<FIntPoint> SortedCandidates = CandidateCells.Array();
		SortedCandidates.Sort([](const FIntPoint& Left, const FIntPoint& Right)
		{
			return Left.Y == Right.Y ? Left.X < Right.X : Left.Y < Right.Y;
		});

		bool bAddedReward = false;
		for (const FIntPoint& TileCoordinate : SortedCandidates)
		{
			if (RandomStream.FRand() < RewardChance)
			{
				PatternStamp.RewardCells.Add(TileCoordinate);
				bAddedReward = true;
			}
		}

		if (bAddedReward == false && SortedCandidates.IsEmpty() == false)
		{
			PatternStamp.RewardCells.Add(SortedCandidates[RandomStream.RandRange(0, SortedCandidates.Num() - 1)]);
		}
	}

	/** 종유석: 천장 Rock 에 한 칸 너비 기둥이 매달려 있다. 윗부분을 끊으면 떨어져 부서지고, 아래에서 캐면 머리 위로 떨어진다. */
	FFallingPatternStamp BuildStalactitesStamp(
		const FIntPoint& CenterCoordinate,
		int32 FieldWidth,
		int32 MaxLength,
		int32 FallingDistance,
		int32 TargetThickness,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 MinX = CenterCoordinate.X - FieldWidth / 2;
		const int32 MaxX = MinX + FieldWidth - 1;
		const int32 CeilingY = CenterCoordinate.Y;

		TSet<FIntPoint> TipCells;
		for (int32 X = MinX; X <= MaxX; X += 2)
		{
			const int32 Length = RandomStream.RandRange(2, FMath::Max(2, MaxLength));
			PatternStamp.SupportCells.Add(FIntPoint(X, CeilingY + 1));
			for (int32 Y = CeilingY + 2; Y <= CeilingY + Length; ++Y)
			{
				PatternStamp.BodyCells.Add(FIntPoint(X, Y));
			}
			TipCells.Add(FIntPoint(X, CeilingY + Length));
		}

		AddRectangleCells(PatternStamp.RockAnchorCells, MinX - 1, CeilingY, MaxX + 1, CeilingY);
		AddRandomRewardCells(PatternStamp, TipCells, 0.35f, RandomStream);
		FinalizeEnclosedStamp(PatternStamp, FallingDistance, TargetThickness, false);

		// 바닥 윗줄에 금을 내어, 떨어진 종유석이 바닥을 뚫고 박힌다.
		int32 FloorTopY = MAX_int32;
		for (const FIntPoint& TargetCell : PatternStamp.TargetCells)
		{
			FloorTopY = FMath::Min(FloorTopY, TargetCell.Y);
		}
		TSet<FIntPoint> FloorTopCells;
		for (const FIntPoint& TargetCell : PatternStamp.TargetCells)
		{
			if (TargetCell.Y == FloorTopY)
			{
				FloorTopCells.Add(TargetCell);
			}
		}
		AddPreDamageCells(PatternStamp, FloorTopCells, 0.4f, 0.6f, RandomStream);
		return PatternStamp;
	}

	/** 금 간 바닥: 매달린 바위 아래 금 간 얇은 바닥이 있고, 그 아래는 깊은 구덩이다. 바위를 떨어뜨리면 바닥을 뚫고 끝까지 떨어져 부서진다. */
	FFallingPatternStamp BuildBreakthroughStamp(
		const FIntPoint& CenterCoordinate,
		int32 BoulderWidth,
		int32 GapHeight,
		int32 PitDepth,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 CenterX = CenterCoordinate.X;
		const int32 TopY = CenterCoordinate.Y;
		const int32 BoulderMinX = CenterX - BoulderWidth / 2;
		const int32 BoulderMaxX = BoulderMinX + BoulderWidth - 1;
		constexpr int32 BoulderHeight = 2;

		PatternStamp.RockAnchorCells.Add(FIntPoint(CenterX, TopY - 1));
		PatternStamp.SupportCells.Add(FIntPoint(CenterX, TopY));
		TSet<FIntPoint> BoulderCells;
		AddRectangleCells(BoulderCells, BoulderMinX, TopY + 1, BoulderMaxX, TopY + BoulderHeight);
		PatternStamp.BodyCells.Append(BoulderCells);

		const int32 FloorY = TopY + BoulderHeight + GapHeight + 1;
		const int32 FloorMinX = BoulderMinX - 1;
		const int32 FloorMaxX = BoulderMaxX + 1;
		TSet<FIntPoint> FloorCells;
		AddRectangleCells(FloorCells, FloorMinX, FloorY, FloorMaxX, FloorY);
		PatternStamp.BodyCells.Append(FloorCells);
		PatternStamp.RockAnchorCells.Add(FIntPoint(FloorMinX - 1, FloorY));
		PatternStamp.RockAnchorCells.Add(FIntPoint(FloorMaxX + 1, FloorY));

		AddPreDamageCells(PatternStamp, FloorCells, 0.6f, 0.8f, RandomStream);
		AddRandomRewardCells(PatternStamp, BoulderCells, 0.5f, RandomStream);
		FinalizeEnclosedStamp(PatternStamp, PitDepth, 0, true);
		return PatternStamp;
	}

	/** 보물 매달기: 광물이 몰린 작은 덩어리를 지지대 하나로 매달고 아래는 깊은 낙하 공간과 Rock 바닥이다. 끊으면 부서지며 광물이 쏟아진다. */
	FFallingPatternStamp BuildOreVaultStamp(
		const FIntPoint& CenterCoordinate,
		int32 VaultWidth,
		int32 VaultHeight,
		int32 FallingDistance,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 MinX = CenterCoordinate.X - VaultWidth / 2;
		const int32 MaxX = MinX + VaultWidth - 1;
		const int32 SupportX = RandomStream.RandRange(MinX, MaxX);
		const int32 TopY = CenterCoordinate.Y;

		PatternStamp.RockAnchorCells.Add(FIntPoint(SupportX, TopY - 1));
		PatternStamp.SupportCells.Add(FIntPoint(SupportX, TopY));
		AddRectangleCells(PatternStamp.BodyCells, MinX, TopY + 1, MaxX, TopY + VaultHeight);
		AddRandomRewardCells(PatternStamp, PatternStamp.BodyCells, 0.5f, RandomStream);
		FinalizeEnclosedStamp(PatternStamp, FallingDistance, 0, true);
		return PatternStamp;
	}

	/** 수확 판: 넓은 판을 양쪽 옆벽 지지대로 고정했다. 양쪽을 모두 끊으면 깊이 떨어져 한 번에 부서진다. */
	FFallingPatternStamp BuildHarvestSlabStamp(
		const FIntPoint& CenterCoordinate,
		int32 SlabWidth,
		int32 SlabHeight,
		int32 FallingDistance,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 MinX = CenterCoordinate.X - SlabWidth / 2;
		const int32 MaxX = MinX + SlabWidth - 1;
		const int32 MinY = CenterCoordinate.Y;
		const int32 MaxY = MinY + SlabHeight - 1;
		const int32 LeftSupportY = RandomStream.RandRange(MinY, MaxY);
		const int32 RightSupportY = RandomStream.RandRange(MinY, MaxY);

		AddRectangleCells(PatternStamp.BodyCells, MinX, MinY, MaxX, MaxY);
		PatternStamp.SupportCells.Add(FIntPoint(MinX - 1, LeftSupportY));
		PatternStamp.RockAnchorCells.Add(FIntPoint(MinX - 2, LeftSupportY));
		PatternStamp.SupportCells.Add(FIntPoint(MaxX + 1, RightSupportY));
		PatternStamp.RockAnchorCells.Add(FIntPoint(MaxX + 2, RightSupportY));
		AddRandomRewardCells(PatternStamp, PatternStamp.BodyCells, 0.12f, RandomStream);
		FinalizeEnclosedStamp(PatternStamp, FallingDistance, 0, true);
		return PatternStamp;
	}

	/** 미끼 오두막: 광물이 박힌 두 기둥이 천장 판을 받친다. 기둥의 광물을 다 캐면 천장이 방 안으로 무너진다. */
	FFallingPatternStamp BuildBaitTrapStamp(
		const FIntPoint& CenterCoordinate,
		int32 RoomWidth,
		int32 RoomHeight,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 RoomMinX = CenterCoordinate.X - RoomWidth / 2;
		const int32 RoomMaxX = RoomMinX + RoomWidth - 1;
		const int32 RoomMinY = CenterCoordinate.Y;
		const int32 RoomMaxY = RoomMinY + RoomHeight - 1;
		constexpr int32 CeilingThickness = 2;

		TSet<FIntPoint> LeftPillarCells;
		TSet<FIntPoint> RightPillarCells;
		AddRectangleCells(LeftPillarCells, RoomMinX - 1, RoomMinY, RoomMinX - 1, RoomMaxY);
		AddRectangleCells(RightPillarCells, RoomMaxX + 1, RoomMinY, RoomMaxX + 1, RoomMaxY);
		PatternStamp.SupportCells.Append(LeftPillarCells);
		PatternStamp.SupportCells.Append(RightPillarCells);
		AddRectangleCells(PatternStamp.BodyCells, RoomMinX - 1, RoomMinY - CeilingThickness, RoomMaxX + 1, RoomMinY - 1);

		AddRandomRewardCells(PatternStamp, LeftPillarCells, 0.5f, RandomStream);
		AddRandomRewardCells(PatternStamp, RightPillarCells, 0.5f, RandomStream);
		FinalizeEnclosedStamp(PatternStamp, 0, 0, true);
		return PatternStamp;
	}

	/** 샹들리에: 목과 층이 번갈아 매달려 있다. 어느 목을 끊느냐에 따라 떨어지는 양이 달라진다. */
	FFallingPatternStamp BuildChandelierStamp(
		const FIntPoint& CenterCoordinate,
		int32 TopTierWidth,
		int32 FallingDistance,
		int32 TargetThickness,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 CenterX = CenterCoordinate.X;
		const int32 TierCount = RandomStream.RandRange(2, 3);
		int32 CursorY = CenterCoordinate.Y;

		PatternStamp.RockAnchorCells.Add(FIntPoint(CenterX, CursorY));
		PatternStamp.SupportCells.Add(FIntPoint(CenterX, CursorY + 1));
		CursorY += 2;

		TSet<FIntPoint> LastTierCells;
		int32 TierHalfWidth = FMath::Max(1, TopTierWidth / 2);
		for (int32 TierIndex = 0; TierIndex < TierCount; ++TierIndex)
		{
			const int32 TierHeight = RandomStream.RandRange(1, 2);
			LastTierCells.Reset();
			AddRectangleCells(LastTierCells, CenterX - TierHalfWidth, CursorY, CenterX + TierHalfWidth, CursorY + TierHeight - 1);
			PatternStamp.BodyCells.Append(LastTierCells);
			CursorY += TierHeight;

			if (TierIndex < TierCount - 1)
			{
				const int32 NeckHeight = RandomStream.RandRange(1, 2);
				AddRectangleCells(PatternStamp.BodyCells, CenterX, CursorY, CenterX, CursorY + NeckHeight - 1);
				CursorY += NeckHeight;
				TierHalfWidth = FMath::Max(0, TierHalfWidth - 1);
			}
		}

		AddRandomRewardCells(PatternStamp, LastTierCells, 0.6f, RandomStream);
		FinalizeEnclosedStamp(PatternStamp, FallingDistance, TargetThickness, RandomStream.FRand() < 0.5f);
		return PatternStamp;
	}

	/** 모래시계: 무거운 Sand 저장소가 가느다란 목 하나로 Rock 받침에 얹혀 있다. 목을 캐는 순간 저장소가 덮친다. */
	FFallingPatternStamp BuildHourglassStamp(
		const FIntPoint& CenterCoordinate,
		int32 ReservoirRowCount,
		int32 NeckHeight)
	{
		FFallingPatternStamp PatternStamp;
		const int32 CenterX = CenterCoordinate.X;
		const int32 TopY = CenterCoordinate.Y;
		const int32 TopHalfWidth = ReservoirRowCount;

		for (int32 RowIndex = 0; RowIndex < ReservoirRowCount; ++RowIndex)
		{
			const int32 HalfWidth = FMath::Max(1, TopHalfWidth - RowIndex);
			AddRectangleCells(PatternStamp.BodyCells, CenterX - HalfWidth, TopY + RowIndex, CenterX + HalfWidth, TopY + RowIndex);
		}
		PatternStamp.SandCells = PatternStamp.BodyCells;

		const int32 NeckTopY = TopY + ReservoirRowCount;
		AddRectangleCells(PatternStamp.SupportCells, CenterX, NeckTopY, CenterX, NeckTopY + NeckHeight - 1);
		FinalizeEnclosedStamp(PatternStamp, 0, 0, true);
		return PatternStamp;
	}

	/** 계단 선반: 대각선으로 겹쳐 내려가는 선반. 위 선반이 아래 선반에 쌓이고, 쌓인 것을 다시 떨어뜨리는 단계적 붕괴를 만든다. */
	FFallingPatternStamp BuildCascadeLedgesStamp(
		const FIntPoint& CenterCoordinate,
		int32 LedgeCount,
		int32 LedgeWidth,
		int32 LedgeGap,
		int32 FallingDistance,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 Direction = RandomStream.RandRange(0, 1) == 0 ? 1 : -1;
		const int32 BaseX = CenterCoordinate.X - (Direction * LedgeCount) / 2;

		TSet<FIntPoint> LowestLedgeCells;
		int32 PreviousLedgeMinX = INDEX_NONE;
		for (int32 LedgeIndex = 0; LedgeIndex < LedgeCount; ++LedgeIndex)
		{
			const int32 LedgeMinX = BaseX + (LedgeIndex * Direction);
			const int32 LedgeMaxX = LedgeMinX + LedgeWidth - 1;
			const int32 LedgeY = CenterCoordinate.Y + (LedgeIndex * (LedgeGap + 1));
			LowestLedgeCells.Reset();
			AddRectangleCells(LowestLedgeCells, LedgeMinX, LedgeY, LedgeMaxX, LedgeY);
			PatternStamp.BodyCells.Append(LowestLedgeCells);

			// 위 선반이 떨어져 닿는 칸에 금을 내어, 쌓이는 대신 뚫고 내려가는 연쇄가 일어나게 한다.
			if (PreviousLedgeMinX != INDEX_NONE)
			{
				TSet<FIntPoint> LandingCells;
				for (const FIntPoint& LedgeCell : LowestLedgeCells)
				{
					if (LedgeCell.X >= PreviousLedgeMinX && LedgeCell.X <= PreviousLedgeMinX + LedgeWidth - 1)
					{
						LandingCells.Add(LedgeCell);
					}
				}
				AddPreDamageCells(PatternStamp, LandingCells, 0.5f, 0.75f, RandomStream);
			}
			PreviousLedgeMinX = LedgeMinX;

			// 내려가는 방향의 반대쪽 벽에 붙인다.
			const int32 SupportX = Direction > 0 ? LedgeMinX - 1 : LedgeMaxX + 1;
			const int32 AnchorX = Direction > 0 ? LedgeMinX - 2 : LedgeMaxX + 2;
			PatternStamp.SupportCells.Add(FIntPoint(SupportX, LedgeY));
			PatternStamp.RockAnchorCells.Add(FIntPoint(AnchorX, LedgeY));
		}

		AddRandomRewardCells(PatternStamp, LowestLedgeCells, 0.4f, RandomStream);
		FinalizeEnclosedStamp(PatternStamp, FallingDistance, 0, true);
		return PatternStamp;
	}

	/** 하중 다리: 동굴을 가로지르는 양끝 고정 다리 위에 흙더미가 얹혀 있다. 두 군데를 끊어야 가운데 구간이 떨어진다. */
	FFallingPatternStamp BuildLoadBridgeStamp(
		const FIntPoint& CenterCoordinate,
		int32 SpanWidth,
		int32 CavernHeight,
		int32 TargetThickness,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 MinX = CenterCoordinate.X - SpanWidth / 2;
		const int32 MaxX = MinX + SpanWidth - 1;
		const int32 BeamY = CenterCoordinate.Y;
		const int32 LoadHalfWidth = RandomStream.RandRange(1, 2);
		const int32 LoadHeight = RandomStream.RandRange(1, 2);
		const int32 LoadCenterX = RandomStream.RandRange(MinX + LoadHalfWidth + 1, MaxX - LoadHalfWidth - 1);

		AddRectangleCells(PatternStamp.BodyCells, MinX, BeamY, MaxX, BeamY);
		PatternStamp.RockAnchorCells.Add(FIntPoint(MinX - 1, BeamY));
		PatternStamp.RockAnchorCells.Add(FIntPoint(MaxX + 1, BeamY));

		TSet<FIntPoint> LoadCells;
		AddRectangleCells(LoadCells, LoadCenterX - LoadHalfWidth, BeamY - LoadHeight, LoadCenterX + LoadHalfWidth, BeamY - 1);
		PatternStamp.BodyCells.Append(LoadCells);
		AddRandomRewardCells(PatternStamp, LoadCells, 0.5f, RandomStream);

		// 흙더미 양옆 다리 칸에 금을 내어, 어디를 끊으면 가운데가 떨어지는지 눈으로 읽을 수 있게 한다.
		TSet<FIntPoint> CutPointCells;
		CutPointCells.Add(FIntPoint(LoadCenterX - LoadHalfWidth - 1, BeamY));
		CutPointCells.Add(FIntPoint(LoadCenterX + LoadHalfWidth + 1, BeamY));
		AddPreDamageCells(PatternStamp, CutPointCells, 0.6f, 0.8f, RandomStream);
		FinalizeEnclosedStamp(PatternStamp, CavernHeight, TargetThickness, false);
		return PatternStamp;
	}

	/**
	 * 영역 안에서 Body / Support / Rock / Target 으로 지정되지 않은 칸과 바깥 한 칸 둘레를 모두 비우고 예약한다.
	 * 배치를 직접 설계하는 Rock 구조물이 쓴다. 호출 전에 모든 칸을 지정해 둔다.
	 */
	void EncloseStampArea(FFallingPatternStamp& PatternStamp, int32 MinX, int32 MinY, int32 MaxX, int32 MaxY)
	{
		TSet<FIntPoint> AreaCells;
		AddRectangleCells(AreaCells, MinX - 1, MinY - 1, MaxX + 1, MaxY + 1);
		for (const FIntPoint& TileCoordinate : AreaCells)
		{
			if (PatternStamp.BodyCells.Contains(TileCoordinate) == false &&
				PatternStamp.SupportCells.Contains(TileCoordinate) == false &&
				PatternStamp.RockAnchorCells.Contains(TileCoordinate) == false &&
				PatternStamp.TargetCells.Contains(TileCoordinate) == false)
			{
				PatternStamp.RequiredEmptyCells.Add(TileCoordinate);
			}
		}
		PatternStamp.ReservedCells.Append(AreaCells);
	}

	/** 돌 척추: 세로 Rock 기둥에 흙 가지가 번갈아 붙어 있다. 가지 뿌리를 끊으면 떨어져 아래 가지에 부딪힌다. 가지 끝의 보상은 끊기 전에 캐야 한다. */
	FFallingPatternStamp BuildRockSpineStamp(
		const FIntPoint& CenterCoordinate,
		int32 SpineHeight,
		int32 MaxBranchLength,
		int32 FallingDistance,
		int32 TargetThickness,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 SpineX = CenterCoordinate.X;
		const int32 TopY = CenterCoordinate.Y;
		AddRectangleCells(PatternStamp.RockAnchorCells, SpineX, TopY, SpineX, TopY + SpineHeight - 1);

		TSet<FIntPoint> TipCells;
		bool bBranchToRight = RandomStream.RandRange(0, 1) == 0;
		for (int32 BranchY = TopY + 1; BranchY <= TopY + SpineHeight - 2; BranchY += RandomStream.RandRange(2, 3))
		{
			const int32 Direction = bBranchToRight ? 1 : -1;
			const int32 BranchLength = RandomStream.RandRange(2, FMath::Max(2, MaxBranchLength));
			PatternStamp.SupportCells.Add(FIntPoint(SpineX + Direction, BranchY));
			for (int32 Offset = 2; Offset <= BranchLength; ++Offset)
			{
				PatternStamp.BodyCells.Add(FIntPoint(SpineX + (Direction * Offset), BranchY));
			}
			TipCells.Add(FIntPoint(SpineX + (Direction * BranchLength), BranchY));

			if (RandomStream.FRand() < 0.7f)
			{
				bBranchToRight = !bBranchToRight;
			}
		}

		AddRandomRewardCells(PatternStamp, TipCells, 0.5f, RandomStream);
		FinalizeEnclosedStamp(PatternStamp, FallingDistance, TargetThickness, RandomStream.FRand() < 0.5f);
		return PatternStamp;
	}

	/** 돌 우산: 매달린 흙덩이 아래에 흙 마개가 박힌 Rock 지붕이 있고, 그 아래 Rock 으로 둘러싼 보상 방이 있다. 마개를 먼저 뚫으면 흙덩이가 방으로 떨어져 보상이 깨진다. */
	FFallingPatternStamp BuildRockUmbrellaStamp(
		const FIntPoint& CenterCoordinate,
		int32 HalfWidth,
		int32 GapHeight,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 CenterX = CenterCoordinate.X;
		const int32 TopY = CenterCoordinate.Y;
		const int32 MinX = CenterX - HalfWidth;
		const int32 MaxX = CenterX + HalfWidth;

		PatternStamp.RockAnchorCells.Add(FIntPoint(CenterX, TopY));
		PatternStamp.SupportCells.Add(FIntPoint(CenterX, TopY + 1));
		AddRectangleCells(PatternStamp.BodyCells, MinX, TopY + 2, MaxX, TopY + 3);

		const int32 RoofY = TopY + 4 + GapHeight;
		AddRectangleCells(PatternStamp.RockAnchorCells, MinX - 1, RoofY, MaxX + 1, RoofY);
		const FIntPoint PlugCoordinate(CenterX, RoofY);
		PatternStamp.RockAnchorCells.Remove(PlugCoordinate);
		PatternStamp.BodyCells.Add(PlugCoordinate);
		// 짧은 낙하 충격은 버티도록 마개를 무거운 Sand 로 채운다.
		PatternStamp.SandCells.Add(PlugCoordinate);

		const int32 RoomFloorY = RoofY + 3;
		AddRectangleCells(PatternStamp.RockAnchorCells, MinX - 1, RoofY + 1, MinX - 1, RoomFloorY + 1);
		AddRectangleCells(PatternStamp.RockAnchorCells, MaxX + 1, RoofY + 1, MaxX + 1, RoomFloorY + 1);
		AddRectangleCells(PatternStamp.RockAnchorCells, MinX, RoomFloorY + 1, MaxX, RoomFloorY + 1);

		TSet<FIntPoint> RoomFloorCells;
		AddRectangleCells(RoomFloorCells, MinX, RoomFloorY, MaxX, RoomFloorY);
		PatternStamp.TargetCells.Append(RoomFloorCells);
		AddRandomRewardCells(PatternStamp, RoomFloorCells, 0.6f, RandomStream);

		EncloseStampArea(PatternStamp, MinX - 1, TopY + 1, MaxX + 1, RoomFloorY + 1);
		return PatternStamp;
	}

	/** 돌 모루: 두 보물 덩어리가 나란히 매달려 있다. 한쪽은 깊은 Rock 바닥(떨어뜨리면 깨짐), 한쪽은 얕은 흙 쿠션(떨어뜨려도 멀쩡함) 위다. */
	FFallingPatternStamp BuildRockAnvilStamp(
		const FIntPoint& CenterCoordinate,
		int32 VaultWidth,
		int32 AnvilDropHeight,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 TopY = CenterCoordinate.Y;
		const int32 MassTopY = TopY + 2;
		const int32 MassBottomY = TopY + 3;
		const bool bAnvilOnLeft = RandomStream.RandRange(0, 1) == 0;
		const int32 LeftMinX = CenterCoordinate.X - VaultWidth - 1;
		const int32 LeftMaxX = LeftMinX + VaultWidth - 1;
		const int32 RightMinX = CenterCoordinate.X + 2;
		const int32 RightMaxX = RightMinX + VaultWidth - 1;
		const int32 AnvilMinX = bAnvilOnLeft ? LeftMinX : RightMinX;
		const int32 AnvilMaxX = bAnvilOnLeft ? LeftMaxX : RightMaxX;
		const int32 CushionMinX = bAnvilOnLeft ? RightMinX : LeftMinX;
		const int32 CushionMaxX = bAnvilOnLeft ? RightMaxX : LeftMaxX;

		TSet<FIntPoint> MassCells;
		for (const int32 MassMinX : { LeftMinX, RightMinX })
		{
			const int32 SupportX = MassMinX + (VaultWidth / 2);
			PatternStamp.RockAnchorCells.Add(FIntPoint(SupportX, TopY));
			PatternStamp.SupportCells.Add(FIntPoint(SupportX, TopY + 1));
			AddRectangleCells(MassCells, MassMinX, MassTopY, MassMinX + VaultWidth - 1, MassBottomY);
		}
		PatternStamp.BodyCells.Append(MassCells);
		AddRandomRewardCells(PatternStamp, MassCells, 0.5f, RandomStream);

		const int32 AnvilY = MassBottomY + AnvilDropHeight + 1;
		AddRectangleCells(PatternStamp.RockAnchorCells, AnvilMinX, AnvilY, AnvilMaxX, AnvilY);

		// 쿠션은 1칸 아래에 두 줄 두께로 둔다. 짧게 떨어져 부서지지 않고 쌓인다.
		const int32 CushionTopY = MassBottomY + 2;
		AddRectangleCells(PatternStamp.TargetCells, CushionMinX, CushionTopY, CushionMaxX, CushionTopY + 1);
		AddRectangleCells(PatternStamp.RockAnchorCells, CushionMinX, CushionTopY + 2, CushionMaxX, CushionTopY + 2);

		EncloseStampArea(PatternStamp, LeftMinX, TopY + 1, RightMaxX, FMath::Max(AnvilY, CushionTopY + 2));
		return PatternStamp;
	}

	/** 돌 깔때기: V 자로 좁아지는 Rock 계단 위에 흙덩이가 매달려 있다. 바깥 줄은 계단에 차례로 부딪혀 깨지고, 가운데만 통과해 금 간 바닥을 뚫는다. */
	FFallingPatternStamp BuildRockFunnelStamp(
		const FIntPoint& CenterCoordinate,
		int32 HalfWidth,
		int32 PitDepth,
		FRandomStream& RandomStream)
	{
		FFallingPatternStamp PatternStamp;
		const int32 CenterX = CenterCoordinate.X;
		const int32 TopY = CenterCoordinate.Y;
		const int32 MinX = CenterX - HalfWidth;
		const int32 MaxX = CenterX + HalfWidth;

		PatternStamp.RockAnchorCells.Add(FIntPoint(CenterX, TopY));
		PatternStamp.SupportCells.Add(FIntPoint(CenterX, TopY + 1));
		TSet<FIntPoint> MassCells;
		AddRectangleCells(MassCells, MinX, TopY + 2, MaxX, TopY + 3);
		PatternStamp.BodyCells.Append(MassCells);
		AddRandomRewardCells(PatternStamp, MassCells, 0.2f, RandomStream);

		const int32 FunnelTopY = TopY + 6;
		int32 LeftWallX = MinX - 1;
		int32 RightWallX = MaxX + 1;
		int32 FunnelY = FunnelTopY;
		while (RightWallX - LeftWallX - 1 > 1)
		{
			PatternStamp.RockAnchorCells.Add(FIntPoint(LeftWallX, FunnelY));
			PatternStamp.RockAnchorCells.Add(FIntPoint(RightWallX, FunnelY));
			++LeftWallX;
			--RightWallX;
			++FunnelY;
		}

		// 반복이 끝났을 때 LeftWallX / RightWallX 는 마지막으로 놓인 벽 바로 안쪽 칸이다.
		const int32 OpeningMinX = LeftWallX;
		const int32 OpeningMaxX = RightWallX;
		const int32 FloorY = FunnelY + 2;
		TSet<FIntPoint> FloorCells;
		AddRectangleCells(FloorCells, OpeningMinX - 1, FloorY, OpeningMaxX + 1, FloorY);
		PatternStamp.BodyCells.Append(FloorCells);
		PatternStamp.RockAnchorCells.Add(FIntPoint(OpeningMinX - 2, FloorY));
		PatternStamp.RockAnchorCells.Add(FIntPoint(OpeningMaxX + 2, FloorY));
		AddPreDamageCells(PatternStamp, FloorCells, 0.6f, 0.8f, RandomStream);

		const int32 PitBottomY = FloorY + PitDepth + 1;
		AddRectangleCells(PatternStamp.RockAnchorCells, OpeningMinX - 1, PitBottomY, OpeningMaxX + 1, PitBottomY);

		EncloseStampArea(PatternStamp, MinX - 1, TopY + 1, MaxX + 1, PitBottomY);
		return PatternStamp;
	}

	void SortTileCoordinates(TArray<FIntPoint>& TileCoordinates)
	{
		TileCoordinates.Sort([](const FIntPoint& Left, const FIntPoint& Right)
		{
			return Left.Y == Right.Y ? Left.X < Right.X : Left.Y < Right.Y;
		});
	}

	void CollectMineableComponent(
		const FIntPoint& StartCoordinate,
		const TMap<FIntPoint, int32>& GeneratedMineableTiles,
		int32 RockTileIndex,
		TSet<FIntPoint>& CheckedCoordinates,
		TArray<FIntPoint>& OutComponent)
	{
		TArray<FIntPoint> CoordinatesToCheck;
		CoordinatesToCheck.Add(StartCoordinate);

		while (CoordinatesToCheck.IsEmpty() == false)
		{
			const FIntPoint TileCoordinate = CoordinatesToCheck.Pop(EAllowShrinking::No);
			if (CheckedCoordinates.Contains(TileCoordinate))
			{
				continue;
			}

			const int32* TileIndex = GeneratedMineableTiles.Find(TileCoordinate);
			if (TileIndex == nullptr || *TileIndex == RockTileIndex)
			{
				continue;
			}

			CheckedCoordinates.Add(TileCoordinate);
			OutComponent.Add(TileCoordinate);

			for (const FIntPoint& Direction : CardinalDirections)
			{
				CoordinatesToCheck.Add(TileCoordinate + Direction);
			}
		}
	}

	int32 CountAdjacentAnchors(
		const TArray<FIntPoint>& MineableComponent,
		const TMap<FIntPoint, int32>& GeneratedMineableTiles,
		const TMap<FIntPoint, int32>& FixedTileTemplate,
		int32 RockTileIndex)
	{
		TSet<FIntPoint> AdjacentAnchors;
		for (const FIntPoint& TileCoordinate : MineableComponent)
		{
			for (const FIntPoint& Direction : CardinalDirections)
			{
				const FIntPoint AdjacentCoordinate = TileCoordinate + Direction;
				if (FixedTileTemplate.Contains(AdjacentCoordinate))
				{
					AdjacentAnchors.Add(AdjacentCoordinate);
					continue;
				}

				const int32* AdjacentTileIndex = GeneratedMineableTiles.Find(AdjacentCoordinate);
				if (AdjacentTileIndex != nullptr && *AdjacentTileIndex == RockTileIndex)
				{
					AdjacentAnchors.Add(AdjacentCoordinate);
				}
			}
		}

		return AdjacentAnchors.Num();
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

	if (ShakeTime <= 0.f)
	{
		const TArray<ABlockBase*> ShakingBlocks = GetFallingBlocks();
		if (ShakingBlocks.IsEmpty() == false)
		{
			USoundManager::TryPlaySFX(ShakingBlocks[0], ESFXType::Block_Shake);
		}
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

	TArray<TWeakObjectPtr<ABlockBase>> GroupFallingBlocks;
	for (const TPair<int32, FFallingVerticalBlocksData>& FallingBlockPair : FallingBlockMap)
	{
		for (const TWeakObjectPtr<ABlockBase>& FallingBlock : FallingBlockPair.Value.FallingBlocks)
		{
			if (FallingBlock.IsValid())
			{
				GroupFallingBlocks.AddUnique(FallingBlock);
			}
		}
	}

	for (TPair<int32, FFallingVerticalBlocksData>& FallingBlockPair : FallingBlockMap)
	{
		FFallingVerticalBlocksData& VerticalBlockData = FallingBlockPair.Value;
		TWeakObjectPtr<ABlockBase> BottomBlock = VerticalBlockData.BottomBlock;
		if (BottomBlock.IsValid() == false)
		{
			VerticalBlockData.bFallFinished = true;
			continue;
		}

		VerticalBlockData.FallingVelocity += 980.f * DeltaTime;
		const FVector MoveVector(0.f, 0.f, -VerticalBlockData.FallingVelocity * DeltaTime);
		FVector RemainingMoveVector = MoveVector;
		bool bShouldFinishFalling = false;
		bool bShouldPauseFalling = false;
		// 낙하 블록 파괴와 부딪힌 블록 파괴가 한 프레임에 여러 번 일어날 수 있다.
		const int32 MaximumCollisionIterationCount = (VerticalBlockData.FallingBlocks.Num() * 2) + 4;
		int32 CollisionIterationCount = 0;
		while (RemainingMoveVector.Z < -KINDA_SMALL_NUMBER &&
			bShouldFinishFalling == false &&
			bShouldPauseFalling == false)
		{
			if (CollisionIterationCount >= MaximumCollisionIterationCount)
			{
				bShouldFinishFalling = true;
				break;
			}

			const FVector TargetLocation = BottomBlock->GetActorLocation() + RemainingMoveVector;
			FHitResult NearestHit;
			TWeakObjectPtr<ABlockBase> NearestFallingBlock;
			if (DetectFallingBlockObstacle(
				BottomBlock.Get(),
				VerticalBlockData.FallingBlocks,
				GroupFallingBlocks,
				TargetLocation,
				NearestHit,
				NearestFallingBlock) == false)
			{
				for (const TWeakObjectPtr<ABlockBase>& FallingBlock : VerticalBlockData.FallingBlocks)
				{
					if (FallingBlock.IsValid())
					{
						FallingBlock->SetActorLocation(FallingBlock->GetActorLocation() + RemainingMoveVector);
					}
				}
				VerticalBlockData.FallenDistance += FMath::Max(0.f, -RemainingMoveVector.Z);
				RemainingMoveVector = FVector::ZeroVector;
				break;
			}

			AActor* HitActor = NearestHit.GetActor();
			ABlockBase* HitBlock = Cast<ABlockBase>(HitActor);
			const float RemainingMoveBeforeContactZ = RemainingMoveVector.Z;
			FVector ContactMoveVector = RemainingMoveVector * FMath::Clamp(NearestHit.Time, 0.f, 1.f);
			if (::IsValid(HitBlock) && NearestFallingBlock.IsValid())
			{
				UBoxComponent* FallingBoxComponent = NearestFallingBlock->FindComponentByClass<UBoxComponent>();
				UBoxComponent* HitBoxComponent = HitBlock->FindComponentByClass<UBoxComponent>();
				if (::IsValid(FallingBoxComponent) && ::IsValid(HitBoxComponent))
				{
					const float FallingBlockBottom =
						FallingBoxComponent->GetComponentLocation().Z - FallingBoxComponent->GetScaledBoxExtent().Z;
					const float HitBlockTop =
						HitBoxComponent->GetComponentLocation().Z + HitBoxComponent->GetScaledBoxExtent().Z;
					const float ExactContactMoveZ = HitBlockTop - FallingBlockBottom;
					ContactMoveVector.Z = NearestHit.bStartPenetrating
						? ExactContactMoveZ
						: FMath::Clamp(ExactContactMoveZ, RemainingMoveVector.Z, 0.f);
				}
			}

			for (const TWeakObjectPtr<ABlockBase>& FallingBlock : VerticalBlockData.FallingBlocks)
			{
				if (FallingBlock.IsValid())
				{
					FallingBlock->SetActorLocation(FallingBlock->GetActorLocation() + ContactMoveVector);
				}
			}
			VerticalBlockData.FallenDistance += FMath::Max(0.f, -ContactMoveVector.Z);
			RemainingMoveVector.Z -= ContactMoveVector.Z;

			if (::IsValid(HitBlock) && HitBlock->IsCushioned())
			{
				// 완충 젤 위로 떨어진 블록은 충격 없이 착지해 드롭을 유지한다.
				bShouldFinishFalling = true;
			}
			else if (::IsValid(HitBlock))
			{
				bool bHitBlockBroken = false;
				if (UWorld* World = BottomBlock->GetWorld())
				{
					if (ULevelManagerSubsystem* LevelManagerSubsystem = World->GetSubsystem<ULevelManagerSubsystem>())
					{
						bHitBlockBroken = LevelManagerSubsystem->ApplyFallingImpactToSettledBlocks(
							HitBlock,
							CalculateFallingBlockImpactDamage(BottomBlock.Get(), VerticalBlockData.FallingVelocity));
					}
				}

				if (TryDestroyFallingBlockOnImpact(
					BottomBlock.Get(),
					NearestFallingBlock.Get(),
					VerticalBlockData.FallingVelocity))
				{
					VerticalBlockData.FallingBlocks.Remove(NearestFallingBlock);
					VerticalBlockData.BlockOriginPositionMap.Remove(NearestFallingBlock);
					GroupFallingBlocks.Remove(NearestFallingBlock);

					BottomBlock = nullptr;
					for (const TWeakObjectPtr<ABlockBase>& FallingBlock : VerticalBlockData.FallingBlocks)
					{
						if (FallingBlock.IsValid() &&
							(BottomBlock.IsValid() == false || FallingBlock->GetActorLocation().Z < BottomBlock->GetActorLocation().Z))
						{
							BottomBlock = FallingBlock;
						}
					}
					VerticalBlockData.BottomBlock = BottomBlock;

					if (BottomBlock.IsValid() == false)
					{
						bShouldFinishFalling = true;
					}
				}
				else if (bHitBlockBroken)
				{
					// 부딪힌 블록이 부서지면서 콜리전이 꺼졌으므로 남은 이동량만큼 계속 떨어진다.
				}
				else
				{
					if (UWorld* World = BottomBlock->GetWorld())
					{
						if (ULevelManagerSubsystem* LevelManagerSubsystem = World->GetSubsystem<ULevelManagerSubsystem>())
						{
							LevelManagerSubsystem->LogFallingStopDiagnostic(
								NearestFallingBlock.Get(),
								HitBlock,
								NearestHit,
								ContactMoveVector.Z,
								RemainingMoveBeforeContactZ,
								RemainingMoveVector.Z);
						}
					}
					bShouldFinishFalling = true;
				}
			}

			AMKCharacter* HitCharacter = Cast<AMKCharacter>(HitActor);
			if (::IsValid(HitCharacter))
			{
				const int32 FallingDamage = CalculateCharacterFallingDamage(BottomBlock.Get(), VerticalBlockData);
				if (FallingDamage > 0)
				{
					FGameplayAbilityUtils::ApplyDamage(
						HitCharacter->GetAbilitySystemComponent(),
						nullptr,
						static_cast<float>(FallingDamage));
				}

				VerticalBlockData.FallingVelocity = 0.f;
				VerticalBlockData.FallenDistance = 0.f;
				bShouldPauseFalling = true;
			}

			++CollisionIterationCount;
		}

		if (bShouldFinishFalling)
		{
			VerticalBlockData.bFallFinished = true;
		}
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

int32 FFallingBlockGroupData::CalculateCharacterFallingDamage(
	UObject* WorldContextObject,
	const FFallingVerticalBlocksData& VerticalBlockData) const
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

	const float RawDamage =
		VerticalBlockData.FallingBlocks.Num() *
		GameSettingDataAsset->FallingBlockDamagePerBlock *
		(VerticalBlockData.FallenDistance / 100.f);
	if (RawDamage > 0.f && RawDamage < 1.f)
	{
		return 1;
	}

	return FMath::FloorToInt(RawDamage);
}

float FFallingBlockGroupData::CalculateFallingBlockImpactDamage(UObject* WorldContextObject, float FallingVelocity) const
{
	UDataManager* DataManager = UDataManager::Get(WorldContextObject);
	if (::IsValid(DataManager) == false)
	{
		return 0.f;
	}

	const UGameSettingDataAsset* GameSettings = DataManager->GetGameSettingDataAsset();
	if (::IsValid(GameSettings) == false)
	{
		return 0.f;
	}

	const float DamageSpeed = FMath::Max(0.f, FallingVelocity - GameSettings->MinimumFallingBlockImpactSpeed);
	if (DamageSpeed <= 0.f)
	{
		return 0.f;
	}

	const float RawImpactDamage = DamageSpeed * GameSettings->FallingBlockImpactDamagePerSpeed;
	return FMath::Min(RawImpactDamage, GameSettings->MaximumFallingBlockImpactPower);
}

bool FFallingBlockGroupData::TryDestroyFallingBlockOnImpact(
	UObject* WorldContextObject,
	ABlockBase* FallingBlock,
	float FallingVelocity) const
{
	if (::IsValid(FallingBlock) == false)
	{
		return false;
	}

	UAbilitySystemComponent* FallingAbilitySystemComponent = FallingBlock->GetAbilitySystemComponent();
	const UAttributeSet_Block* BlockAttributeSet = ::IsValid(FallingAbilitySystemComponent)
		? Cast<UAttributeSet_Block>(FallingAbilitySystemComponent->GetAttributeSet(UAttributeSet_Block::StaticClass()))
		: nullptr;
	if (::IsValid(BlockAttributeSet) == false)
	{
		return false;
	}

	const float CurrentDurability = BlockAttributeSet->GetDurability();
	if (CurrentDurability <= 0.f)
	{
		return true;
	}

	const float ImpactDamage = CalculateFallingBlockImpactDamage(WorldContextObject, FallingVelocity);
	if (ImpactDamage <= 0.f)
	{
		return false;
	}

	// 붕괴 충격으로 부서진 블록은 아이템을 드롭하지 않는다. 버텨서 착지한 블록은 드롭을 유지한다.
	FallingBlock->SetItemDropEnabled(false);
	FGameplayAbilityUtils::ApplyDamageToDurability(FallingAbilitySystemComponent, nullptr, ImpactDamage);

	const float RemainingDurability = BlockAttributeSet->GetDurability();
	if (RemainingDurability > 0.f)
	{
		FallingBlock->SetItemDropEnabled(true);
	}
	return RemainingDurability <= 0.f;
}

void ULevelManagerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	LoadTileMap();
	if (TileMapActor.IsValid())
	{
		CaptureOriginalTileTemplates(UDataManager::Get(this), TileMapActor->GetRenderComponent());
	}
	GenerateTileActors();
}

void ULevelManagerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ProcessPendingCollapseBlocks();
	ApplyPendingBlockSounds();

	if (FallingBlockGroupList.IsEmpty())
	{
		return;
	}

	for (int32 Index = FallingBlockGroupList.Num() - 1; Index >= 0; --Index)
	{
		FFallingBlockGroupData& FallingBlockGroupData = FallingBlockGroupList[Index];

		for (auto It = FallingBlockGroupData.FallingBlockMap.CreateIterator(); It; ++It)
		{
			FFallingVerticalBlocksData VerticalBlockData = It.Value();
			if (VerticalBlockData.bFallFinished)
			{
				for (const TWeakObjectPtr<ABlockBase>& LandedBlock : VerticalBlockData.FallingBlocks)
				{
					PendingLandedBlockCount += LandedBlock.IsValid() ? 1 : 0;
				}

				SnapBlocks(VerticalBlockData.FallingBlocks);
				It.RemoveCurrent();
				continue;
			}
		}

		if (FallingBlockGroupData.IsFallingFinished())
		{
			FallingBlockGroupList.RemoveAt(Index);
			continue;
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

void ULevelManagerSubsystem::LoadTileMap()
{
	if (TileMapActor.IsValid() == false)
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
}

void ULevelManagerSubsystem::GenerateTileActors()
{
	const USaveGameSubsystem* SaveGameSubsystem = USaveGameSubsystem::Get(this);
	const UMKSaveGame* SaveGame = ::IsValid(SaveGameSubsystem) ? SaveGameSubsystem->GetSaveGame() : nullptr;
	RegenerateMineableBlocks(::IsValid(SaveGame) ? SaveGame->GenerationSeed : INDEX_NONE);
}

void ULevelManagerSubsystem::RegenerateMineableBlocks(int32 GenerationSeed)
{
	if (TileMapActor.IsValid() == false)
	{
		LoadTileMap();
		if (TileMapActor.IsValid() == false)
		{
			return;
		}
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

	CaptureOriginalTileTemplates(DataManager, TileMapComp);

	const FBlockDataTableRow* RockBlockData = DataManager->GetBlockDataTableRow(FName(TEXT("Rock")));
	if (RockBlockData == nullptr)
	{
		ensureMsgf(false, TEXT("Can't generate mineable tiles: BlockDataTable row 'Rock' was not found."));
		return;
	}

	const FBlockDataTableRow* ShopBlockData = DataManager->GetBlockDataTableRow(FName(TEXT("Shop")));
	ShopTileIndex = ShopBlockData != nullptr ? ShopBlockData->TileIndex : INDEX_NONE;

	TSubclassOf<AActor> BlockClass = DataManager->GetBlueprintClass(EBlueprintClassType::BlockBase);
	if (BlockClass.Get() == nullptr)
	{
		return;
	}

	const UGameSettingDataAsset* GameSettings = DataManager->GetGameSettingDataAsset();
	const FMineableLevelGenerationSettings GenerationSettings = GameSettings != nullptr
		? GameSettings->MineableLevelGeneration
		: FMineableLevelGenerationSettings();

	LastGenerationSeed = GenerationSeed == INDEX_NONE ? FMath::Rand() : GenerationSeed;
	FRandomStream RandomStream(LastGenerationSeed);

	const FBlockDataTableRow* SandBlockData = DataManager->GetBlockDataTableRow(FName(TEXT("Sand")));
	const int32 SandTileIndex = SandBlockData != nullptr ? SandBlockData->TileIndex : INDEX_NONE;

	TMap<FIntPoint, int32> GeneratedMineableTiles;
	TMap<FIntPoint, TPair<FName, int32>> RewardTiles;
	TMap<FIntPoint, float> PreDamageTiles;
	GenerateMineableLayout(
		RandomStream,
		GenerationSettings,
		RockBlockData->TileIndex,
		SandTileIndex,
		GeneratedMineableTiles,
		RewardTiles,
		PreDamageTiles);

	ClearGeneratedBlocks();

	TArray<FIntPoint> FixedTileCoordinates;
	OriginalFixedTileTemplate.GenerateKeyArray(FixedTileCoordinates);
	SortTileCoordinates(FixedTileCoordinates);
	for (const FIntPoint& TileCoordinate : FixedTileCoordinates)
	{
		SpawnTileActor(TileCoordinate, OriginalFixedTileTemplate[TileCoordinate], TileMapComp, BlockClass);
	}

	TArray<FIntPoint> GeneratedTileCoordinates;
	GeneratedMineableTiles.GenerateKeyArray(GeneratedTileCoordinates);
	SortTileCoordinates(GeneratedTileCoordinates);
	for (const FIntPoint& TileCoordinate : GeneratedTileCoordinates)
	{
		const int32 TileIndex = GeneratedMineableTiles[TileCoordinate];
		ABlockBase* SpawnedBlock = SpawnTileActor(TileCoordinate, TileIndex, TileMapComp, BlockClass);
		if (::IsValid(SpawnedBlock) == false || TileIndex == RockBlockData->TileIndex)
		{
			continue;
		}

		if (const TPair<FName, int32>* RewardTile = RewardTiles.Find(TileCoordinate))
		{
			SpawnedBlock->TryApplyStructureReward(RewardTile->Key, RewardTile->Value);
		}

		const float* PreDamageRatio = PreDamageTiles.Find(TileCoordinate);
		const FBlockDataTableRow* SpawnedBlockData = PreDamageRatio != nullptr ? DataManager->GetBlockDataTableRow(TileIndex) : nullptr;
		if (SpawnedBlockData != nullptr && SpawnedBlockData->BlockDurability > 0.f)
		{
			FGameplayAbilityUtils::ApplyDamageToDurability(
				SpawnedBlock->GetAbilitySystemComponent(),
				nullptr,
				SpawnedBlockData->BlockDurability * (*PreDamageRatio));
		}
	}

	OnGenerateFinished();
}

void ULevelManagerSubsystem::ResetMineableLevel(int32 GenerationSeed)
{
	ClearWorldItems();
	RegenerateMineableBlocks(GenerationSeed);
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

const TArray<TObjectPtr<ABlockBase>>& ULevelManagerSubsystem::GetBlocks() const
{
	return BlockList;
}

ABlockBase* ULevelManagerSubsystem::GetBlockAtPosition(const FVector2D& BlockPosition) const
{
	const TWeakObjectPtr<ABlockBase>* BlockPtr = BlockActorMap.Find(BlockPosition);
	return BlockPtr != nullptr ? BlockPtr->Get() : nullptr;
}

ABlockBase* ULevelManagerSubsystem::GetBlockAtWorldLocation(const FVector& WorldLocation) const
{
	FIntPoint TileCoordinate;
	FVector TileCenterWorldLocation;
	if (TryGetTileCenterWorldLocation(WorldLocation, TileCoordinate, TileCenterWorldLocation) == false)
	{
		return nullptr;
	}

	return GetBlockAtPosition(UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(TileCenterWorldLocation));
}

ABlockBase* ULevelManagerSubsystem::GetBlockBelowWorldLocation(const FVector& WorldLocation) const
{
	FIntPoint TileCoordinate;
	FVector TileCenterWorldLocation;
	if (TryGetTileCenterWorldLocation(WorldLocation, TileCoordinate, TileCenterWorldLocation) == false)
	{
		return nullptr;
	}

	const float TileHeight = TileMapActor->GetRenderComponent()->TileMap->TileHeight;
	return GetBlockAtWorldLocation(TileCenterWorldLocation - FVector(0.f, 0.f, TileHeight));
}

void ULevelManagerSubsystem::CaptureOriginalTileTemplates(UDataManager* DataManager, UPaperTileMapComponent* TileMapComponent)
{
	if (bOriginalTileTemplatesCaptured || ::IsValid(DataManager) == false || ::IsValid(TileMapComponent) == false)
	{
		return;
	}

	UPaperTileMap* TileMap = TileMapComponent->TileMap;
	if (::IsValid(TileMap) == false)
	{
		return;
	}

	OriginalFixedTileTemplate.Reset();
	OriginalMineableTileTemplate.Reset();

	for (int32 Y = 0; Y < TileMap->MapHeight; ++Y)
	{
		for (int32 X = 0; X < TileMap->MapWidth; ++X)
		{
			const FPaperTileInfo TileInfo = TileMapComponent->GetTile(X, Y, 0);
			if (TileInfo.TileSet == nullptr)
			{
				continue;
			}

			const int32 TileIndex = TileInfo.PackedTileIndex & 0xFFFF;
			const FIntPoint TileCoordinate(X, Y);
			const FBlockDataTableRow* BlockData = DataManager->GetBlockDataTableRow(TileIndex);
			if (BlockData != nullptr && BlockData->bIsMineable)
			{
				OriginalMineableTileTemplate.Add(TileCoordinate, TileIndex);
			}
			else
			{
				OriginalFixedTileTemplate.Add(TileCoordinate, TileIndex);
			}
		}
	}

	bOriginalTileTemplatesCaptured = true;
}

void ULevelManagerSubsystem::ClearGeneratedBlocks()
{
	for (ABlockBase* Block : BlockList)
	{
		if (::IsValid(Block))
		{
			Block->Destroy();
		}
	}

	BlockList.Reset();
	BlockActorMap.Reset();
	FallingBlockGroupList.Reset();
	PendingCollapseBlockLists.Reset();
	bHasLoggedFallingStopDiagnostic = false;
	bHasLoggedDestroyedPendingBlockDiagnostic = false;
	bHasLoggedSnapDiagnostic = false;
}

void ULevelManagerSubsystem::ClearWorldItems()
{
	UWorld* World = GetWorld();
	if (::IsValid(World) == false)
	{
		return;
	}

	for (TActorIterator<AItemBase> ItemIterator(World); ItemIterator; ++ItemIterator)
	{
		AItemBase* Item = *ItemIterator;
		if (::IsValid(Item))
		{
			Item->Destroy();
		}
	}
}

void ULevelManagerSubsystem::GenerateMineableLayout(
	FRandomStream& RandomStream,
	const FMineableLevelGenerationSettings& GenerationSettings,
	int32 RockTileIndex,
	int32 SandTileIndex,
	TMap<FIntPoint, int32>& OutGeneratedMineableTiles,
	TMap<FIntPoint, TPair<FName, int32>>& OutRewardTiles,
	TMap<FIntPoint, float>& OutPreDamageTiles) const
{
	OutGeneratedMineableTiles = OriginalMineableTileTemplate;
	OutRewardTiles.Reset();
	OutPreDamageTiles.Reset();
	if (OutGeneratedMineableTiles.IsEmpty())
	{
		return;
	}

	TArray<FIntPoint> TemplateCoordinates;
	OriginalMineableTileTemplate.GenerateKeyArray(TemplateCoordinates);
	SortTileCoordinates(TemplateCoordinates);

	TSet<FIntPoint> ReservedCoordinates;
	TSet<FIntPoint> RandomRockCoordinates;
	TSet<FIntPoint> EmptyCoordinates;
	TSet<FIntPoint> RewardCoordinates;

	const int32 MaxPlacementAttempts = PlacementAttemptLimit;
	if (RandomStream.FRand() <= FMath::Clamp(GenerationSettings.MonsterRoomGenerationChance, 0.f, 1.f))
	{
		for (int32 Attempt = 0; Attempt < MaxPlacementAttempts; ++Attempt)
		{
			const FIntPoint CenterCoordinate = TemplateCoordinates[RandomStream.RandRange(0, TemplateCoordinates.Num() - 1)];
			const int32 MinRoomWidth = FMath::Max(1, GenerationSettings.MonsterRoomWidthRange.Min);
			const int32 MaxRoomWidth = FMath::Max(MinRoomWidth, GenerationSettings.MonsterRoomWidthRange.Max);
			const int32 RoomWidth = RandomStream.RandRange(MinRoomWidth, MaxRoomWidth);
			const int32 RoomHeight = FMath::Max(1, GenerationSettings.MonsterRoomHeight);
			const int32 MinX = CenterCoordinate.X - RoomWidth / 2;
			const int32 MinY = CenterCoordinate.Y - RoomHeight / 2;
			const int32 MaxX = MinX + RoomWidth - 1;
			const int32 MaxY = MinY + RoomHeight - 1;

			bool bCanCreateRoom = true;
			for (int32 Y = MinY; bCanCreateRoom && Y <= MaxY + 1; ++Y)
			{
				for (int32 X = MinX; X <= MaxX; ++X)
				{
					const FIntPoint TileCoordinate(X, Y);
					if (OriginalMineableTileTemplate.Contains(TileCoordinate) == false || ReservedCoordinates.Contains(TileCoordinate))
					{
						bCanCreateRoom = false;
						break;
					}
				}
			}

			if (bCanCreateRoom == false)
			{
				continue;
			}

			for (int32 Y = MinY; Y <= MaxY; ++Y)
			{
				for (int32 X = MinX; X <= MaxX; ++X)
				{
					const FIntPoint TileCoordinate(X, Y);
					OutGeneratedMineableTiles.Remove(TileCoordinate);
					EmptyCoordinates.Add(TileCoordinate);
					ReservedCoordinates.Add(TileCoordinate);
				}
			}

			for (int32 X = MinX; X <= MaxX; ++X)
			{
				const FIntPoint FloorCoordinate(X, MaxY + 1);
				OutGeneratedMineableTiles.Add(FloorCoordinate, RockTileIndex);
				ReservedCoordinates.Add(FloorCoordinate);
			}
			break;
		}
	}

	// 다양성을 우선한다. 모든 종류를 한 번씩 시도한 뒤, 다음 회차는 적게 배치된 종류부터 시도한다.
	// 한 종류가 실패하면 크기와 낙하 거리를 점점 줄여 작은 버전이라도 들어가게 한다.
	int32 CreatedFallingStructureCount = 0;
	bool bCreatedSingleSupportTopHammer = false;
	const int32 PlacementRetryCount = FMath::Max(1, GenerationSettings.FallingStructurePlacementRetryCount);
	// TemplateCoordinates 는 Y 오름차순이라 첫 칸이 채굴 영역 맨 윗줄(지표면)이다.
	const int32 MinimumStructureY = TemplateCoordinates[0].Y + FMath::Max(0, GenerationSettings.FallingStructureSurfaceMargin);
	TArray<EFallingPatternType> PatternBag;
	TArray<int32> PlacedPatternCounts;
	PlacedPatternCounts.Init(0, static_cast<int32>(EFallingPatternType::Count));
	// 가장 작은 크기로도 자리를 못 찾은 종류는 이후 회차에서 건너뛰어, 남은 시도를 들어갈 수 있는 종류에 쓴다.
	TArray<bool> ExhaustedPatterns;
	ExhaustedPatterns.Init(false, static_cast<int32>(EFallingPatternType::Count));
	const int32 MaxStructureAttempts = PlacementRetryCount * static_cast<int32>(EFallingPatternType::Count) * 8;
	const int32 TargetFallingStructureCount = FMath::Min(
		FMath::Max(0, GenerationSettings.FallingStructureCount),
		FMath::RoundToInt(static_cast<float>(TemplateCoordinates.Num()) * FMath::Max(0.f, GenerationSettings.FallingStructuresPerThousandTiles) / 1000.f));
	for (int32 Attempt = 0;
		Attempt < MaxStructureAttempts && CreatedFallingStructureCount < TargetFallingStructureCount;)
	{
		if (PatternBag.IsEmpty())
		{
			for (int32 PatternIndex = 0; PatternIndex < static_cast<int32>(EFallingPatternType::Count); ++PatternIndex)
			{
				if (ExhaustedPatterns[PatternIndex] == false)
				{
					PatternBag.Add(static_cast<EFallingPatternType>(PatternIndex));
				}
			}
			if (PatternBag.IsEmpty())
			{
				break;
			}
			for (int32 PatternIndex = PatternBag.Num() - 1; PatternIndex > 0; --PatternIndex)
			{
				PatternBag.Swap(PatternIndex, RandomStream.RandRange(0, PatternIndex));
			}

			// Pop 은 뒤에서 꺼내므로, 적게 배치된 종류가 뒤로 가도록 정렬한다.
			PatternBag.StableSort([&PlacedPatternCounts](const EFallingPatternType Left, const EFallingPatternType Right)
			{
				return PlacedPatternCounts[static_cast<int32>(Left)] > PlacedPatternCounts[static_cast<int32>(Right)];
			});
		}

		const EFallingPatternType PatternType = PatternBag.Pop(EAllowShrinking::No);
		bool bPlacedPattern = false;
		for (int32 RetryIndex = 0; RetryIndex < PlacementRetryCount; ++RetryIndex, ++Attempt)
		{
			const FIntPoint CenterCoordinate = TemplateCoordinates[RandomStream.RandRange(0, TemplateCoordinates.Num() - 1)];
			const int32 MinStructureWidth = FMath::Max(4, GenerationSettings.FallingStructureWidthRange.Min);
			const int32 MaxStructureWidth = FMath::Max(MinStructureWidth, GenerationSettings.FallingStructureWidthRange.Max);
			const int32 MinStructureHeight = FMath::Max(2, GenerationSettings.FallingStructureHeightRange.Min);
			const int32 MaxStructureHeight = FMath::Max(MinStructureHeight, GenerationSettings.FallingStructureHeightRange.Max);
			const int32 MinFallingDistance = FMath::Max(1, GenerationSettings.FallingDistanceInTilesRange.Min);
			const int32 MaxFallingDistance = FMath::Max(MinFallingDistance, GenerationSettings.FallingDistanceInTilesRange.Max);
			const int32 MinTargetThickness = FallingStructureTargetThicknessMin;
			const int32 MaxTargetThickness = FallingStructureTargetThicknessMax;

			// 재시도할수록 1.0 에서 0.4 까지 크기를 줄인다.
			const float SizeScale = PlacementRetryCount > 1
				? 1.f - (0.6f * static_cast<float>(RetryIndex) / static_cast<float>(PlacementRetryCount - 1))
				: 1.f;
			auto ScaleSize = [SizeScale](int32 Size, int32 MinimumSize)
			{
				return FMath::Max(MinimumSize, FMath::RoundToInt(static_cast<float>(Size) * SizeScale));
			};

			const int32 StructureWidth = ScaleSize(RandomStream.RandRange(MinStructureWidth, MaxStructureWidth), 4);
			const int32 StructureHeight = ScaleSize(RandomStream.RandRange(MinStructureHeight, MaxStructureHeight), 2);
			const int32 FallingDistance = ScaleSize(RandomStream.RandRange(MinFallingDistance, MaxFallingDistance), 2);
			const int32 TargetThickness = ScaleSize(RandomStream.RandRange(MinTargetThickness, MaxTargetThickness), 1);
			// 충돌로 부서지는 연출이 필요한 구조물은 Dirt 기준 부서지는 높이보다 충분히 떨어지게 한다.
			const int32 ShatterFallingDistance = FMath::Max(6, FallingDistance);
			FFallingPatternStamp PatternStamp;
			switch (PatternType)
			{
			case EFallingPatternType::TopHammer:
			{
				const int32 MaximumSupportCount = FMath::Clamp(
					FallingStructureSupportCountMax,
					1,
					StructureWidth);
				const int32 SupportCount = bCreatedSingleSupportTopHammer == false || MaximumSupportCount == 1
					? 1
					: RandomStream.RandRange(2, MaximumSupportCount);
				PatternStamp = BuildTopHammerStamp(
					CenterCoordinate,
					StructureWidth,
					StructureHeight,
					FallingDistance,
					TargetThickness,
					SupportCount,
					RandomStream);
				break;
			}
			case EFallingPatternType::SideOverhang:
				PatternStamp = BuildSideOverhangStamp(
					CenterCoordinate,
					StructureWidth,
					StructureHeight,
					FallingDistance,
					TargetThickness,
					RandomStream.RandRange(0, 1) == 0,
					RandomStream);
				break;
			case EFallingPatternType::KeystoneWings:
				PatternStamp = BuildKeystoneWingsStamp(
					CenterCoordinate,
					FMath::Max(7, StructureWidth),
					StructureHeight,
					FallingDistance,
					TargetThickness,
					RandomStream);
				break;
			case EFallingPatternType::TriggerCascade:
				PatternStamp = BuildTriggerCascadeStamp(
					CenterCoordinate,
					FMath::Max(6, StructureWidth),
					FMath::Max(4, StructureHeight),
					FMath::Max(5, FallingDistance),
					TargetThickness,
					RandomStream.RandRange(0, 1) == 0,
					RandomStream);
				break;
			case EFallingPatternType::Stalactites:
				PatternStamp = BuildStalactitesStamp(
					CenterCoordinate,
					FMath::Clamp(StructureWidth, 3, 11),
					FMath::Clamp(StructureHeight, 2, 5),
					FallingDistance,
					TargetThickness,
					RandomStream);
				break;
			case EFallingPatternType::OreVault:
				PatternStamp = BuildOreVaultStamp(
					CenterCoordinate,
					RandomStream.RandRange(3, 4),
					RandomStream.RandRange(2, 3),
					ShatterFallingDistance,
					RandomStream);
				break;
			case EFallingPatternType::HarvestSlab:
				PatternStamp = BuildHarvestSlabStamp(
					CenterCoordinate,
					FMath::Clamp(StructureWidth, 5, 14),
					RandomStream.RandRange(2, 3),
					ShatterFallingDistance,
					RandomStream);
				break;
			case EFallingPatternType::BaitTrap:
				PatternStamp = BuildBaitTrapStamp(
					CenterCoordinate,
					RandomStream.RandRange(3, 5),
					RandomStream.RandRange(2, 3),
					RandomStream);
				break;
			case EFallingPatternType::Chandelier:
				PatternStamp = BuildChandelierStamp(
					CenterCoordinate,
					ScaleSize(RandomStream.RandRange(5, 7), 3),
					FallingDistance,
					TargetThickness,
					RandomStream);
				break;
			case EFallingPatternType::Hourglass:
				PatternStamp = BuildHourglassStamp(
					CenterCoordinate,
					RandomStream.RandRange(3, 4),
					RandomStream.RandRange(2, 3));
				break;
			case EFallingPatternType::CascadeLedges:
				PatternStamp = BuildCascadeLedgesStamp(
					CenterCoordinate,
					SizeScale < 0.7f ? 2 : RandomStream.RandRange(3, 4),
					RandomStream.RandRange(3, 4),
					RandomStream.RandRange(2, 3),
					ShatterFallingDistance,
					RandomStream);
				break;
			case EFallingPatternType::LoadBridge:
				PatternStamp = BuildLoadBridgeStamp(
					CenterCoordinate,
					FMath::Clamp(StructureWidth, 7, 13),
					FMath::Clamp(FallingDistance, 3, 6),
					TargetThickness,
					RandomStream);
				break;
			case EFallingPatternType::Breakthrough:
				PatternStamp = BuildBreakthroughStamp(
					CenterCoordinate,
					RandomStream.RandRange(2, 3),
					RandomStream.RandRange(3, 4),
					ShatterFallingDistance,
					RandomStream);
				break;
			case EFallingPatternType::RockSpine:
				PatternStamp = BuildRockSpineStamp(
					CenterCoordinate,
					ScaleSize(RandomStream.RandRange(7, 11), 5),
					ScaleSize(RandomStream.RandRange(3, 4), 2),
					FallingDistance,
					TargetThickness,
					RandomStream);
				break;
			case EFallingPatternType::RockUmbrella:
				PatternStamp = BuildRockUmbrellaStamp(
					CenterCoordinate,
					ScaleSize(RandomStream.RandRange(1, 2), 1),
					RandomStream.RandRange(3, 4),
					RandomStream);
				break;
			case EFallingPatternType::RockAnvil:
				PatternStamp = BuildRockAnvilStamp(
					CenterCoordinate,
					ScaleSize(RandomStream.RandRange(2, 3), 2),
					ScaleSize(RandomStream.RandRange(5, 7), 4),
					RandomStream);
				break;
			case EFallingPatternType::RockFunnel:
				PatternStamp = BuildRockFunnelStamp(
					CenterCoordinate,
					ScaleSize(RandomStream.RandRange(2, 4), 2),
					ScaleSize(RandomStream.RandRange(4, 6), 3),
					RandomStream);
				break;
			default:
				continue;
			}

			if (ApplyFallingPatternStamp(
				PatternStamp,
				OriginalMineableTileTemplate,
				OutGeneratedMineableTiles,
				ReservedCoordinates,
				EmptyCoordinates,
				RewardCoordinates,
				OutPreDamageTiles,
				RockTileIndex,
				SandTileIndex,
				MinimumStructureY))
			{
				++CreatedFallingStructureCount;
				++PlacedPatternCounts[static_cast<int32>(PatternType)];
				if (PatternType == EFallingPatternType::TopHammer)
				{
					bCreatedSingleSupportTopHammer = true;
				}
				bPlacedPattern = true;
				break;
			}
		}

		if (bPlacedPattern == false)
		{
			ExhaustedPatterns[static_cast<int32>(PatternType)] = true;
		}
	}

	FString PlacedPatternSummary;
	for (int32 PatternIndex = 0; PatternIndex < PlacedPatternCounts.Num(); ++PatternIndex)
	{
		PlacedPatternSummary += FString::Printf(TEXT("%d "), PlacedPatternCounts[PatternIndex]);
	}
	UE_LOG(LogTemp, Log, TEXT("[LevelGen] Falling structures %d/%d, per pattern: %s"),
		CreatedFallingStructureCount,
		TargetFallingStructureCount,
		*PlacedPatternSummary);

	const int32 TargetEmptyBlockCount = FMath::RoundToInt(
		OriginalMineableTileTemplate.Num() * FMath::Clamp(GenerationSettings.EmptyBlockRatio, 0.f, 1.f));
	for (int32 Attempt = 0;
		Attempt < MaxPlacementAttempts && EmptyCoordinates.Num() < TargetEmptyBlockCount;
		++Attempt)
	{
		const FIntPoint CenterCoordinate = TemplateCoordinates[RandomStream.RandRange(0, TemplateCoordinates.Num() - 1)];
		const int32 MinPocketWidth = FMath::Max(1, GenerationSettings.EmptyPocketWidthRange.Min);
		const int32 MaxPocketWidth = FMath::Max(MinPocketWidth, GenerationSettings.EmptyPocketWidthRange.Max);
		const int32 MinPocketHeight = FMath::Max(1, GenerationSettings.EmptyPocketHeightRange.Min);
		const int32 MaxPocketHeight = FMath::Max(MinPocketHeight, GenerationSettings.EmptyPocketHeightRange.Max);
		const int32 PocketWidth = RandomStream.RandRange(MinPocketWidth, MaxPocketWidth);
		const int32 PocketHeight = RandomStream.RandRange(MinPocketHeight, MaxPocketHeight);
		const int32 MinX = CenterCoordinate.X - RandomStream.RandRange(0, PocketWidth - 1);
		const int32 MinY = CenterCoordinate.Y - RandomStream.RandRange(0, PocketHeight - 1);

		for (int32 Y = MinY; Y < MinY + PocketHeight && EmptyCoordinates.Num() < TargetEmptyBlockCount; ++Y)
		{
			for (int32 X = MinX; X < MinX + PocketWidth && EmptyCoordinates.Num() < TargetEmptyBlockCount; ++X)
			{
				const FIntPoint TileCoordinate(X, Y);
				if (ReservedCoordinates.Contains(TileCoordinate) || OutGeneratedMineableTiles.Contains(TileCoordinate) == false)
				{
					continue;
				}

				const bool bIsCorner = (X == MinX || X == MinX + PocketWidth - 1) &&
					(Y == MinY || Y == MinY + PocketHeight - 1);
				if (bIsCorner && RandomStream.FRand() < 0.5f)
				{
					continue;
				}

				OutGeneratedMineableTiles.Remove(TileCoordinate);
				EmptyCoordinates.Add(TileCoordinate);
			}
		}
	}

	const int32 TargetRandomRockBlockCount = FMath::RoundToInt(
		OriginalMineableTileTemplate.Num() * FMath::Clamp(GenerationSettings.RockBlockRatio, 0.f, 1.f));
	for (int32 Attempt = 0;
		Attempt < MaxPlacementAttempts && RandomRockCoordinates.Num() < TargetRandomRockBlockCount;
		++Attempt)
	{
		FIntPoint RockCoordinate = TemplateCoordinates[RandomStream.RandRange(0, TemplateCoordinates.Num() - 1)];
		const int32 ClusterSize = RandomStream.RandRange(1, RockClusterMaxSize);
		for (int32 ClusterIndex = 0;
			ClusterIndex < ClusterSize && RandomRockCoordinates.Num() < TargetRandomRockBlockCount;
			++ClusterIndex)
		{
			if (ReservedCoordinates.Contains(RockCoordinate) == false && OutGeneratedMineableTiles.Contains(RockCoordinate))
			{
				OutGeneratedMineableTiles.Add(RockCoordinate, RockTileIndex);
				RandomRockCoordinates.Add(RockCoordinate);
			}

			RockCoordinate += CardinalDirections[RandomStream.RandRange(0, UE_ARRAY_COUNT(CardinalDirections) - 1)];
		}
	}

	TSet<FIntPoint> CheckedCoordinates;
	for (const FIntPoint& StartCoordinate : TemplateCoordinates)
	{
		if (CheckedCoordinates.Contains(StartCoordinate))
		{
			continue;
		}

		TArray<FIntPoint> MineableComponent;
		CollectMineableComponent(
			StartCoordinate,
			OutGeneratedMineableTiles,
			RockTileIndex,
			CheckedCoordinates,
			MineableComponent);
		if (MineableComponent.IsEmpty())
		{
			continue;
		}

		SortTileCoordinates(MineableComponent);
		const int32 MaxBlocksPerAnchor = MineableBlocksPerAnchor;
		bool bIsReservedStructureComponent = true;
		for (const FIntPoint& TileCoordinate : MineableComponent)
		{
			if (ReservedCoordinates.Contains(TileCoordinate) == false)
			{
				bIsReservedStructureComponent = false;
				break;
			}
		}
		const int32 RequiredAnchorCount = bIsReservedStructureComponent
			? 1
			: FMath::DivideAndRoundUp(MineableComponent.Num(), MaxBlocksPerAnchor);
		const int32 ExistingAnchorCount = CountAdjacentAnchors(
			MineableComponent,
			OutGeneratedMineableTiles,
			OriginalFixedTileTemplate,
			RockTileIndex);
		const int32 MissingAnchorCount = FMath::Max(0, RequiredAnchorCount - ExistingAnchorCount);
		TArray<FIntPoint> AnchorCandidateCoordinates;
		for (const FIntPoint& TileCoordinate : MineableComponent)
		{
			if (ReservedCoordinates.Contains(TileCoordinate) == false)
			{
				AnchorCandidateCoordinates.Add(TileCoordinate);
			}
		}
		if (AnchorCandidateCoordinates.IsEmpty())
		{
			AnchorCandidateCoordinates = MineableComponent;
		}
		for (int32 AnchorIndex = 0; AnchorIndex < MissingAnchorCount; ++AnchorIndex)
		{
			const int32 ComponentIndex = FMath::Clamp(
				((AnchorIndex + 1) * AnchorCandidateCoordinates.Num()) / (MissingAnchorCount + 1),
				0,
				AnchorCandidateCoordinates.Num() - 1);
			OutGeneratedMineableTiles.Add(AnchorCandidateCoordinates[ComponentIndex], RockTileIndex);
		}
	}

	float TotalRewardWeight = 0.f;
	for (const FBlockSpawnableItemData& RewardItemData : GenerationSettings.StructureRewardItemDataList)
	{
		TotalRewardWeight += RewardItemData.SpawnableItemKey.IsNone() ? 0.f : FMath::Max(0.f, RewardItemData.OreSpawnWeight);
	}
	if (TotalRewardWeight <= 0.f)
	{
		return;
	}

	TArray<FIntPoint> SortedRewardCoordinates = RewardCoordinates.Array();
	SortTileCoordinates(SortedRewardCoordinates);
	for (const FIntPoint& RewardCoordinate : SortedRewardCoordinates)
	{
		const int32* RewardTileIndex = OutGeneratedMineableTiles.Find(RewardCoordinate);
		if (RewardTileIndex == nullptr || *RewardTileIndex == RockTileIndex)
		{
			continue;
		}

		float RewardSelectionValue = RandomStream.FRand() * TotalRewardWeight;
		for (const FBlockSpawnableItemData& RewardItemData : GenerationSettings.StructureRewardItemDataList)
		{
			const float RewardWeight = RewardItemData.SpawnableItemKey.IsNone() ? 0.f : FMath::Max(0.f, RewardItemData.OreSpawnWeight);
			if (RewardWeight <= 0.f)
			{
				continue;
			}

			RewardSelectionValue -= RewardWeight;
			if (RewardSelectionValue <= 0.f)
			{
				const int32 MinRewardCount = FMath::Max(1, RewardItemData.MinSpawnItemCount);
				const int32 MaxRewardCount = FMath::Max(MinRewardCount, RewardItemData.MaxSpawnItemCount);
				OutRewardTiles.Add(RewardCoordinate, TPair<FName, int32>(RewardItemData.SpawnableItemKey, RandomStream.RandRange(MinRewardCount, MaxRewardCount)));
				break;
			}
		}
	}
}

ABlockBase* ULevelManagerSubsystem::SpawnTileActor(
	const FIntPoint& TileCoordinate,
	int32 TileIndex,
	UPaperTileMapComponent* TileMapComponent,
	TSubclassOf<AActor> BlockClass)
{
	if (::IsValid(TileMapComponent) == false || ::IsValid(TileMapComponent->TileMap) == false)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (::IsValid(World) == false || BlockClass.Get() == nullptr)
	{
		return nullptr;
	}

	const FVector LocalPosition = TileMapComponent->GetTileCenterPosition(TileCoordinate.X, TileCoordinate.Y, 0);
	const FVector WorldPosition = TileMapComponent->GetComponentTransform().TransformPosition(LocalPosition);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABlockBase* SpawnedBlock = World->SpawnActor<ABlockBase>(BlockClass, WorldPosition, FRotator::ZeroRotator, SpawnParameters);
	if (::IsValid(SpawnedBlock) == false)
	{
		return nullptr;
	}

	FBlockTileData BlockData;
	BlockData.TileSetIndex = TileIndex;
	BlockData.TileSize = FIntPoint(TileMapComponent->TileMap->TileWidth, TileMapComponent->TileMap->TileHeight);
	BlockData.WorldLocation = WorldPosition;
	BlockData.OnBlockDestroyedDelegate.AddUObject(this, &ULevelManagerSubsystem::OnBlockDestroyed);

	SpawnedBlock->InitializeBlock(BlockData);
	BlockList.Add(SpawnedBlock);

	const FVector2D BlockPosition = UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(WorldPosition);
	BlockActorMap.Add(BlockPosition, SpawnedBlock);

	if (TileIndex == ShopTileIndex)
	{
		FActorSpawnParameters ShopSpawnParameters;
		ShopSpawnParameters.Owner = SpawnedBlock;
		ShopSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AShopActor* SpawnedShop = World->SpawnActor<AShopActor>(AShopActor::StaticClass(), WorldPosition, FRotator::ZeroRotator, ShopSpawnParameters);
		if (::IsValid(SpawnedShop))
		{
			SpawnedShop->InitializeShop(BlockData.TileSize);
		}
	}

	return SpawnedBlock;
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

	for (auto BlockActorIterator = BlockActorMap.CreateIterator(); BlockActorIterator; ++BlockActorIterator)
	{
		if (BlockActorIterator.Value().IsValid() == false || BlockActorIterator.Value().Get() == DestroyedBlock)
		{
			BlockActorIterator.RemoveCurrent();
		}
	}

	TArray<ABlockBase*> ProcessedBlocks;
	const TArray<ABlockBase*> SurroundBlocks = FBlockUtils::GetSurroundBlocks(this, DestroyedBlock);
	for (const auto& SurroundBlock : SurroundBlocks)
	{
		if (::IsValid(SurroundBlock) == false || ProcessedBlocks.Contains(SurroundBlock))
		{
			continue;
		}

		TArray<ABlockBase*> DisconnectedBlocks;
		TArray<ABlockBase*> DFSCheckedBlocks;
		if (CheckBlockIsAllDisconnected(SurroundBlock, DisconnectedBlocks, DFSCheckedBlocks, 0))
		{
			QueueCollapseBlocks(DisconnectedBlocks);
		}

		ProcessedBlocks.Append(DisconnectedBlocks);
	}
}

bool ULevelManagerSubsystem::CheckBlockIsAllDisconnected(ABlockBase* StartBlock, TArray<ABlockBase*>& OutDisconnectedBlocks, TArray<ABlockBase*>& DFSCheckedBlocks, int32 DebugCount)
{
	if (::IsValid(StartBlock) == false)
	{
		return true;
	}

	// DFS cycles should be ignored within a single search only.
	if (DFSCheckedBlocks.Contains(StartBlock))
	{
		return true;
	}
	DFSCheckedBlocks.Add(StartBlock);

	// 채굴 불가 블록과 버팀목은 덩어리를 지탱하는 앵커다.
	if (StartBlock->IsMineable() == false || StartBlock->IsSupportAnchor())
	{
		return false;
	}

	if (bBlockDebugNumbersEnabled)
	{
		StartBlock->BP_EnableDebugState(DebugCount);
	}

	OutDisconnectedBlocks.Add(StartBlock);

	const TArray<ABlockBase*> SurroundBlocks = FBlockUtils::GetSurroundBlocks(this, StartBlock);
	for (const auto& SurroundBlock : SurroundBlocks)
	{
		if (CheckBlockIsAllDisconnected(SurroundBlock, OutDisconnectedBlocks, DFSCheckedBlocks, DebugCount + 1) == false)
		{
			return false;
		}
	}

	return true;
}

void ULevelManagerSubsystem::QueueCollapseBlocks(const TArray<ABlockBase*>& CollapsingBlocks)
{
	TArray<TWeakObjectPtr<ABlockBase>> PendingCollapsingBlocks;
	for (ABlockBase* CollapsingBlock : CollapsingBlocks)
	{
		if (::IsValid(CollapsingBlock) && CollapsingBlock->IsMineable())
		{
			PendingCollapsingBlocks.AddUnique(CollapsingBlock);
		}
	}

	if (PendingCollapsingBlocks.IsEmpty() == false)
	{
		PendingCollapseBlockLists.Add(MoveTemp(PendingCollapsingBlocks));
	}
}

void ULevelManagerSubsystem::ProcessPendingCollapseBlocks()
{
	TArray<TArray<TWeakObjectPtr<ABlockBase>>> PendingBlockLists = MoveTemp(PendingCollapseBlockLists);
	PendingCollapseBlockLists.Reset();

	for (const TArray<TWeakObjectPtr<ABlockBase>>& PendingBlockList : PendingBlockLists)
	{
		TArray<ABlockBase*> CollapsingBlocks;
		for (const TWeakObjectPtr<ABlockBase>& PendingBlock : PendingBlockList)
		{
			if (bHasLoggedDestroyedPendingBlockDiagnostic == false && PendingBlock.IsValid())
			{
				UAbilitySystemComponent* AbilitySystemComponent = PendingBlock->GetAbilitySystemComponent();
				const UAttributeSet_Block* BlockAttributeSet = ::IsValid(AbilitySystemComponent)
					? Cast<UAttributeSet_Block>(AbilitySystemComponent->GetAttributeSet(UAttributeSet_Block::StaticClass()))
					: nullptr;
				if (::IsValid(BlockAttributeSet) && BlockAttributeSet->GetDurability() <= 0.f)
				{
					bHasLoggedDestroyedPendingBlockDiagnostic = true;
					UE_LOG(
						LogTemp,
						Warning,
						TEXT("[CollapseDiag][DestroyedPending] Block=%s Location=%s Key=%s Mineable=%s Falling=%s Durability=%.2f"),
						*GetNameSafe(PendingBlock.Get()),
						*PendingBlock->GetActorLocation().ToCompactString(),
						*UMKBlueprintFunctionLibrary::GetBlockPosition(PendingBlock.Get()).ToString(),
						PendingBlock->IsMineable() ? TEXT("true") : TEXT("false"),
						IsBlockFalling(PendingBlock.Get()) ? TEXT("true") : TEXT("false"),
						BlockAttributeSet->GetDurability());
				}
			}

			if (PendingBlock.IsValid() && PendingBlock->IsMineable())
			{
				CollapsingBlocks.Add(PendingBlock.Get());
			}
		}

		if (CollapsingBlocks.IsEmpty() == false)
		{
			CollapseBlocks(CollapsingBlocks);
		}
	}
}

void ULevelManagerSubsystem::CollapseBlocks(const TArray<ABlockBase*>& CollapsingBlocks)
{
	TMap<int32, FFallingVerticalBlocksData> VerticalBlocksMap;
	int32 CollapsingBlockCount = 0;
	for (auto& CollapsingBlock : CollapsingBlocks)
	{
		if (::IsValid(CollapsingBlock) == false || CollapsingBlock->IsMineable() == false)
		{
			continue;
		}

		CollapsingBlock->SetMineableState(false);

		const FVector2D& NewBlockPosition = UMKBlueprintFunctionLibrary::GetBlockPosition(CollapsingBlock);
		BlockActorMap.Remove(NewBlockPosition);
		++CollapsingBlockCount;
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

	if (CollapsingBlockCount == 0)
	{
		return;
	}

	FFallingBlockGroupData FallingBlockGroupData;
	FallingBlockGroupData.FallingBlockMap = VerticalBlocksMap;

	FallingBlockGroupList.Add(FallingBlockGroupData);
}

void ULevelManagerSubsystem::SnapBlocks(const TArray<TWeakObjectPtr<ABlockBase>>& FallFinishedBlockList)
{
	if (TileMapActor.IsValid() == false)
	{
		if (bHasLoggedSnapDiagnostic == false)
		{
			bHasLoggedSnapDiagnostic = true;
			UE_LOG(LogTemp, Warning, TEXT("[CollapseDiag][SnapAbort] TileMapActor is invalid. BlockCount=%d"), FallFinishedBlockList.Num());
		}
		return;
	}

	UPaperTileMapComponent* TileMapComponent = TileMapActor->GetRenderComponent();
	if (::IsValid(TileMapComponent) == false || ::IsValid(TileMapComponent->TileMap) == false)
	{
		if (bHasLoggedSnapDiagnostic == false)
		{
			bHasLoggedSnapDiagnostic = true;
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[CollapseDiag][SnapAbort] TileMapComponent=%s TileMap=%s BlockCount=%d"),
				*GetNameSafe(TileMapComponent),
				*GetNameSafe(::IsValid(TileMapComponent) ? TileMapComponent->TileMap : nullptr),
				FallFinishedBlockList.Num());
		}
		return;
	}

	const UPaperTileMap* TileMap = TileMapComponent->TileMap;
	if (FMath::IsNearlyZero(TileMap->PixelsPerUnrealUnit))
	{
		if (bHasLoggedSnapDiagnostic == false)
		{
			bHasLoggedSnapDiagnostic = true;
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[CollapseDiag][SnapAbort] PixelsPerUnrealUnit=%.6f BlockCount=%d"),
				TileMap->PixelsPerUnrealUnit,
				FallFinishedBlockList.Num());
		}
		return;
	}

	const float TileWidth = TileMap->TileWidth / TileMap->PixelsPerUnrealUnit;
	const float TileHeight = TileMap->TileHeight / TileMap->PixelsPerUnrealUnit;
	const FVector LocalGridOrigin = TileMapComponent->GetTileCenterPosition(0, 0, 0);
	const FTransform& TileMapTransform = TileMapComponent->GetComponentTransform();
	const FVector WorldVerticalTileOffset = TileMapTransform.TransformVector(FVector(0.f, 0.f, TileHeight));
	const bool bCanStepUpInTileMapSpace = FMath::IsNearlyZero(WorldVerticalTileOffset.Z) == false;
	const float LocalUpwardTileStep = WorldVerticalTileOffset.Z >= 0.f ? TileHeight : -TileHeight;

	TArray<TWeakObjectPtr<ABlockBase>> SortedBlocks = FallFinishedBlockList;
	SortedBlocks.RemoveAll([](const TWeakObjectPtr<ABlockBase>& Block)
	{
		return Block.IsValid() == false;
	});
	SortedBlocks.Sort([](const TWeakObjectPtr<ABlockBase>& Left, const TWeakObjectPtr<ABlockBase>& Right)
	{
		return Left->GetActorLocation().Z < Right->GetActorLocation().Z;
	});

	for (const TWeakObjectPtr<ABlockBase>& FallFinishedBlock : SortedBlocks)
	{
		for (auto BlockActorIterator = BlockActorMap.CreateIterator(); BlockActorIterator; ++BlockActorIterator)
		{
			if (BlockActorIterator.Value().IsValid() == false || BlockActorIterator.Value() == FallFinishedBlock)
			{
				BlockActorIterator.RemoveCurrent();
			}
		}

		FVector LocalSnapLocation = TileMapTransform.InverseTransformPosition(FallFinishedBlock->GetActorLocation());
		LocalSnapLocation.X = LocalGridOrigin.X +
			FMath::RoundToFloat((LocalSnapLocation.X - LocalGridOrigin.X) / TileWidth) * TileWidth;
		LocalSnapLocation.Z = LocalGridOrigin.Z +
			FMath::RoundToFloat((LocalSnapLocation.Z - LocalGridOrigin.Z) / TileHeight) * TileHeight;

		FVector SnapLocation = TileMapTransform.TransformPosition(LocalSnapLocation);
		FVector2D BlockPosition = UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(SnapLocation);
		const FVector PreSnapLocation = FallFinishedBlock->GetActorLocation();
		const FVector2D RawBlockPosition = BlockPosition;
		ABlockBase* RawUnsettledBlock = FindUnsettledBlockAtPosition(RawBlockPosition, FallFinishedBlock.Get());
		const TWeakObjectPtr<ABlockBase>* RawMapOccupant = BlockActorMap.Find(RawBlockPosition);
		const FString RawMapOccupantName = RawMapOccupant != nullptr
			? GetNameSafe(RawMapOccupant->Get())
			: TEXT("None");
		int32 StepUpCount = 0;
		while (true)
		{
			TWeakObjectPtr<ABlockBase>* OccupyingBlock = BlockActorMap.Find(BlockPosition);
			ABlockBase* UnsettledOccupyingBlock = FindUnsettledBlockAtPosition(BlockPosition, FallFinishedBlock.Get());
			const bool bHasSettledOccupant = OccupyingBlock != nullptr && OccupyingBlock->IsValid();
			if (bHasSettledOccupant == false && ::IsValid(UnsettledOccupyingBlock) == false)
			{
				if (OccupyingBlock != nullptr)
				{
					BlockActorMap.Remove(BlockPosition);
				}
				break;
			}

			if (bCanStepUpInTileMapSpace)
			{
				LocalSnapLocation.Z += LocalUpwardTileStep;
				SnapLocation = TileMapTransform.TransformPosition(LocalSnapLocation);
			}
			else
			{
				SnapLocation.Z += TileHeight;
			}
			BlockPosition = UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(SnapLocation);
			++StepUpCount;
		}

		const bool bMovedToSnapLocation = FallFinishedBlock->SetActorLocation(SnapLocation);
		FallFinishedBlock->SetMineableState(true);

		BlockActorMap.Add(BlockPosition, TWeakObjectPtr<ABlockBase>(FallFinishedBlock));

		const FVector2D ActualBlockPosition = UMKBlueprintFunctionLibrary::GetBlockPosition(FallFinishedBlock.Get());
		if (bHasLoggedSnapDiagnostic == false &&
			(::IsValid(RawUnsettledBlock) || StepUpCount > 0 || bMovedToSnapLocation == false || ActualBlockPosition != BlockPosition))
		{
			bHasLoggedSnapDiagnostic = true;
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[CollapseDiag][Snap] Block=%s Pre=%s RawKey=%s RawMap=%s RawUnsettled=%s Final=%s FinalKey=%s ActualKey=%s StepUp=%d MoveSucceeded=%s"),
				*GetNameSafe(FallFinishedBlock.Get()),
				*PreSnapLocation.ToCompactString(),
				*RawBlockPosition.ToString(),
				*RawMapOccupantName,
				*GetNameSafe(RawUnsettledBlock),
				*FallFinishedBlock->GetActorLocation().ToCompactString(),
				*BlockPosition.ToString(),
				*ActualBlockPosition.ToString(),
				StepUpCount,
				bMovedToSnapLocation ? TEXT("true") : TEXT("false"));
		}
	}
}

void ULevelManagerSubsystem::LogFallingStopDiagnostic(
	ABlockBase* FallingBlock,
	ABlockBase* HitBlock,
	const FHitResult& HitResult,
	float ContactMoveZ,
	float RemainingMoveBeforeContactZ,
	float RemainingMoveAfterContactZ)
{
	if (bHasLoggedFallingStopDiagnostic || ::IsValid(HitBlock) == false)
	{
		return;
	}

	const bool bHitBlockPendingCollapse = IsBlockPendingCollapse(HitBlock);
	const bool bHitBlockFalling = IsBlockFalling(HitBlock);
	if (bHitBlockPendingCollapse == false && bHitBlockFalling == false)
	{
		return;
	}

	bHasLoggedFallingStopDiagnostic = true;
	const UAbilitySystemComponent* AbilitySystemComponent = HitBlock->GetAbilitySystemComponent();
	const UAttributeSet_Block* BlockAttributeSet = ::IsValid(AbilitySystemComponent)
		? Cast<UAttributeSet_Block>(AbilitySystemComponent->GetAttributeSet(UAttributeSet_Block::StaticClass()))
		: nullptr;
	const float Durability = ::IsValid(BlockAttributeSet) ? BlockAttributeSet->GetDurability() : -1.f;
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[CollapseDiag][UnsettledStop] Falling=%s FallingLocation=%s FallingKey=%s Hit=%s HitLocation=%s HitKey=%s Pending=%s FallingGroup=%s Mineable=%s Durability=%.2f Time=%.4f StartPenetrating=%s PenetrationDepth=%.3f Normal=%s ContactMoveZ=%.3f RemainingBeforeZ=%.3f RemainingAfterZ=%.3f"),
		*GetNameSafe(FallingBlock),
		::IsValid(FallingBlock) ? *FallingBlock->GetActorLocation().ToCompactString() : TEXT("Invalid"),
		::IsValid(FallingBlock) ? *UMKBlueprintFunctionLibrary::GetBlockPosition(FallingBlock).ToString() : TEXT("Invalid"),
		*GetNameSafe(HitBlock),
		*HitBlock->GetActorLocation().ToCompactString(),
		*UMKBlueprintFunctionLibrary::GetBlockPosition(HitBlock).ToString(),
		bHitBlockPendingCollapse ? TEXT("true") : TEXT("false"),
		bHitBlockFalling ? TEXT("true") : TEXT("false"),
		HitBlock->IsMineable() ? TEXT("true") : TEXT("false"),
		Durability,
		HitResult.Time,
		HitResult.bStartPenetrating ? TEXT("true") : TEXT("false"),
		HitResult.PenetrationDepth,
		*HitResult.Normal.ToCompactString(),
		ContactMoveZ,
		RemainingMoveBeforeContactZ,
		RemainingMoveAfterContactZ);
}

bool ULevelManagerSubsystem::IsBlockPendingCollapse(const ABlockBase* Block) const
{
	if (::IsValid(Block) == false)
	{
		return false;
	}

	for (const TArray<TWeakObjectPtr<ABlockBase>>& PendingBlockList : PendingCollapseBlockLists)
	{
		for (const TWeakObjectPtr<ABlockBase>& PendingBlock : PendingBlockList)
		{
			if (PendingBlock.Get() == Block)
			{
				return true;
			}
		}
	}

	return false;
}

bool ULevelManagerSubsystem::IsBlockFalling(const ABlockBase* Block) const
{
	if (::IsValid(Block) == false)
	{
		return false;
	}

	for (const FFallingBlockGroupData& FallingBlockGroup : FallingBlockGroupList)
	{
		for (const TPair<int32, FFallingVerticalBlocksData>& FallingBlockPair : FallingBlockGroup.FallingBlockMap)
		{
			for (const TWeakObjectPtr<ABlockBase>& FallingBlock : FallingBlockPair.Value.FallingBlocks)
			{
				if (FallingBlock.Get() == Block)
				{
					return true;
				}
			}
		}
	}

	return false;
}

ABlockBase* ULevelManagerSubsystem::FindUnsettledBlockAtPosition(
	const FVector2D& BlockPosition,
	const ABlockBase* IgnoredBlock) const
{
	for (const FFallingBlockGroupData& FallingBlockGroup : FallingBlockGroupList)
	{
		for (const TPair<int32, FFallingVerticalBlocksData>& FallingBlockPair : FallingBlockGroup.FallingBlockMap)
		{
			for (const TWeakObjectPtr<ABlockBase>& FallingBlock : FallingBlockPair.Value.FallingBlocks)
			{
				if (FallingBlock.IsValid() && FallingBlock.Get() != IgnoredBlock &&
					UMKBlueprintFunctionLibrary::GetBlockPosition(FallingBlock.Get()) == BlockPosition)
				{
					return FallingBlock.Get();
				}
			}
		}
	}

	for (const TArray<TWeakObjectPtr<ABlockBase>>& PendingBlockList : PendingCollapseBlockLists)
	{
		for (const TWeakObjectPtr<ABlockBase>& PendingBlock : PendingBlockList)
		{
			if (PendingBlock.IsValid() && PendingBlock.Get() != IgnoredBlock &&
				UMKBlueprintFunctionLibrary::GetBlockPosition(PendingBlock.Get()) == BlockPosition)
			{
				return PendingBlock.Get();
			}
		}
	}

	return nullptr;
}

bool ULevelManagerSubsystem::TryGetTileCenterWorldLocation(const FVector& WorldLocation, FIntPoint& OutTileCoordinate, FVector& OutTileCenterWorldLocation) const
{
	const UPaperTileMapComponent* TileMapComp = TileMapActor.IsValid() ? TileMapActor->GetRenderComponent() : nullptr;
	const UPaperTileMap* TileMap = ::IsValid(TileMapComp) ? TileMapComp->TileMap.Get() : nullptr;
	if (::IsValid(TileMap) == false)
	{
		return false;
	}

	// 블록은 타일 중심에 놓이므로 월드 좌표를 먼저 타일로 바꿔야 옆 칸 판정이 반 칸씩 어긋나지 않는다.
	const FVector LocalPosition = TileMapComp->GetComponentTransform().InverseTransformPosition(WorldLocation);
	int32 TileX = INDEX_NONE;
	int32 TileY = INDEX_NONE;
	TileMap->GetTileCoordinatesFromLocalSpacePosition(LocalPosition, TileX, TileY);
	if (TileX < 0 || TileY < 0 || TileX >= TileMap->MapWidth || TileY >= TileMap->MapHeight)
	{
		return false;
	}

	OutTileCoordinate = FIntPoint(TileX, TileY);
	OutTileCenterWorldLocation = TileMapComp->GetComponentTransform().TransformPosition(TileMapComp->GetTileCenterPosition(TileX, TileY, 0));
	return true;
}

bool ULevelManagerSubsystem::ApplyFallingImpactToSettledBlocks(ABlockBase* HitBlock, float ImpactDamage)
{
	if (::IsValid(HitBlock) == false || ImpactDamage <= 0.f)
	{
		return false;
	}

	UDataManager* DataManager = UDataManager::Get(this);
	const UGameSettingDataAsset* GameSettings = ::IsValid(DataManager) ? DataManager->GetGameSettingDataAsset() : nullptr;
	if (::IsValid(GameSettings) == false)
	{
		return false;
	}

	// 떨어지는 중이거나 이미 부서지는 블록은 IsMineable 이 false 라 여기서 걸러진다.
	TArray<TPair<TWeakObjectPtr<ABlockBase>, float>> DamagedBlocks;
	const FVector2D HitBlockPosition = UMKBlueprintFunctionLibrary::GetBlockPosition(HitBlock);
	for (int32 DepthIndex = 0; DepthIndex < GameSettings->FallingBlockImpactPropagationRatios.Num(); ++DepthIndex)
	{
		ABlockBase* TargetBlock = DepthIndex == 0
			? HitBlock
			: GetBlockAtPosition(FVector2D(HitBlockPosition.X, HitBlockPosition.Y + DepthIndex));
		if (::IsValid(TargetBlock) == false || TargetBlock->IsMineable() == false)
		{
			break;
		}

		const float PropagatedDamage = ImpactDamage * FMath::Max(0.f, GameSettings->FallingBlockImpactPropagationRatios[DepthIndex]);
		if (PropagatedDamage > 0.f)
		{
			DamagedBlocks.Emplace(TargetBlock, PropagatedDamage);
		}
	}

	if (DamagedBlocks.IsEmpty())
	{
		return false;
	}

	bIsApplyingFallingImpact = true;
	for (const TPair<TWeakObjectPtr<ABlockBase>, float>& DamagedBlock : DamagedBlocks)
	{
		if (DamagedBlock.Key.IsValid())
		{
			ABlockBase* DamagedBlockActor = DamagedBlock.Key.Get();
			DamagedBlockActor->SetItemDropEnabled(false);
			FGameplayAbilityUtils::ApplyDamageToDurability(DamagedBlockActor->GetAbilitySystemComponent(), nullptr, DamagedBlock.Value);
			if (DamagedBlockActor->IsMineable())
			{
				DamagedBlockActor->SetItemDropEnabled(true);
			}
		}
	}
	bIsApplyingFallingImpact = false;

	return HitBlock->IsMineable() == false;
}

void ULevelManagerSubsystem::AddPendingBlockBreak(ABlockBase* BreakingBlock)
{
	if (bIsApplyingFallingImpact || IsBlockFalling(BreakingBlock))
	{
		++PendingImpactBrokenBlockCount;
		return;
	}

	++PendingMinedBlockCount;
}

bool ULevelManagerSubsystem::TryPlaceBlock(FName BlockRowName, const FVector& WorldLocation)
{
	FIntPoint TileCoordinate;
	FVector CellCenter;
	if (TryGetTileCenterWorldLocation(WorldLocation, TileCoordinate, CellCenter) == false || IsBlockCellEmpty(CellCenter) == false)
	{
		return false;
	}

	UPaperTileMapComponent* TileMapComp = TileMapActor->GetRenderComponent();
	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return false;
	}

	const FBlockDataTableRow* BlockData = DataManager->GetBlockDataTableRow(BlockRowName);
	TSubclassOf<AActor> BlockClass = DataManager->GetBlueprintClass(EBlueprintClassType::BlockBase);
	if (BlockData == nullptr || BlockClass.Get() == nullptr)
	{
		return false;
	}

	// 캐릭터가 통과할 수 없는 블록은 캐릭터나 다른 폰이 들어가 있는 칸에 설치하지 않는다.
	UWorld* World = GetWorld();
	const float HalfExtent = TileMapComp->TileMap->TileWidth * 0.45f;
	if (::IsValid(World) == false)
	{
		return false;
	}

	if (BlockData->bIsPassableByCharacter == false &&
		World->OverlapAnyTestByObjectType(
			CellCenter,
			FQuat::Identity,
			FCollisionObjectQueryParams(ECC_Pawn),
			FCollisionShape::MakeBox(FVector(HalfExtent, HalfExtent, HalfExtent))))
	{
		return false;
	}

	return ::IsValid(SpawnTileActor(TileCoordinate, BlockData->TileIndex, TileMapComp, BlockClass));
}

int32 ULevelManagerSubsystem::ApplyExplosion(const FVector& WorldLocation, int32 Radius, UAbilitySystemComponent* InstigatorASC)
{
	FIntPoint TileCoordinate;
	FVector CellCenter;
	if (TryGetTileCenterWorldLocation(WorldLocation, TileCoordinate, CellCenter) == false)
	{
		return 0;
	}

	// 파괴 도중 BlockActorMap 이 바뀌므로 대상을 먼저 모은다.
	const FVector2D CenterPosition = UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(CellCenter);
	TArray<TWeakObjectPtr<ABlockBase>> TargetBlocks;
	for (int32 OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
	{
		for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
		{
			ABlockBase* TargetBlock = GetBlockAtPosition(FVector2D(CenterPosition.X + OffsetX, CenterPosition.Y + OffsetY));
			if (::IsValid(TargetBlock))
			{
				TargetBlocks.Add(TargetBlock);
			}
		}
	}

	int32 DestroyedBlockCount = 0;
	for (const TWeakObjectPtr<ABlockBase>& TargetBlock : TargetBlocks)
	{
		if (TargetBlock.IsValid() && TargetBlock->TryDestroyByExplosion())
		{
			++DestroyedBlockCount;
		}
	}

	USoundManager::TryPlaySFX(this, ESFXType::Blast_Explode);
	return DestroyedBlockCount;
}

bool ULevelManagerSubsystem::IsBlockCellEmpty(const FVector& WorldLocation) const
{
	FIntPoint TileCoordinate;
	FVector CellCenter;
	if (TryGetTileCenterWorldLocation(WorldLocation, TileCoordinate, CellCenter) == false)
	{
		return false;
	}

	const FVector2D BlockPosition = UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(CellCenter);
	return ::IsValid(GetBlockAtPosition(BlockPosition)) == false &&
		::IsValid(FindUnsettledBlockAtPosition(BlockPosition, nullptr)) == false;
}

void ULevelManagerSubsystem::ApplyPendingBlockSounds()
{
	if (PendingMinedBlockCount > 0)
	{
		USoundManager::TryPlaySFX(this, CalculateBlockCountSFXType(PendingMinedBlockCount, ESFXType::Block_Mine_Single, ESFXType::Block_Mine_Medium, ESFXType::Block_Mine_Large));
	}

	if (PendingImpactBrokenBlockCount > 0)
	{
		USoundManager::TryPlaySFX(this, CalculateBlockCountSFXType(PendingImpactBrokenBlockCount, ESFXType::Block_Break_Single, ESFXType::Block_Break_Medium, ESFXType::Block_Break_Large));
	}

	if (PendingLandedBlockCount > 0)
	{
		USoundManager::TryPlaySFX(this, CalculateBlockCountSFXType(PendingLandedBlockCount, ESFXType::Block_Fall_Single, ESFXType::Block_Fall_Medium, ESFXType::Block_Fall_Large));
	}

	PendingMinedBlockCount = 0;
	PendingImpactBrokenBlockCount = 0;
	PendingLandedBlockCount = 0;
}

ESFXType ULevelManagerSubsystem::CalculateBlockCountSFXType(int32 BlockCount, ESFXType SingleSFXType, ESFXType MediumSFXType, ESFXType LargeSFXType) const
{
	const UDataManager* DataManager = UDataManager::Get(const_cast<ULevelManagerSubsystem*>(this));
	const UGameSettingDataAsset* GameSettings = ::IsValid(DataManager) ? DataManager->GetGameSettingDataAsset() : nullptr;
	const int32 MediumMinCount = ::IsValid(GameSettings) ? GameSettings->MediumBlockSoundMinCount : 2;
	const int32 LargeMinCount = ::IsValid(GameSettings) ? GameSettings->LargeBlockSoundMinCount : 6;

	if (BlockCount >= LargeMinCount)
	{
		return LargeSFXType;
	}

	return BlockCount >= MediumMinCount ? MediumSFXType : SingleSFXType;
}
