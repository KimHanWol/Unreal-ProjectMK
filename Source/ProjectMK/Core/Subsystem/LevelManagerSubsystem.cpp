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
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Block.h"
#include "ProjectMK/Core/Manager/DataManager.h"
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

	enum class EFallingPatternType : uint8
	{
		TopHammer,
		SideOverhang,
		KeystoneWings,
		TriggerCascade,
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
	};

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
		int32 RockTileIndex)
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
			if (MineableTileTemplate.Contains(TileCoordinate) == false || InOutReservedCoordinates.Contains(TileCoordinate))
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
			InOutGeneratedMineableTiles.Add(TileCoordinate, MineableTileTemplate.FindChecked(TileCoordinate));
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
			const FIntPoint TileCoordinate = CoordinatesToCheck.Pop(false);
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

	FallingVelocity += 980.f * DeltaTime;
	const FVector MoveVector(0.f, 0.f, -FallingVelocity * DeltaTime);
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

		FVector RemainingMoveVector = MoveVector;
		bool bShouldStopFalling = false;
		const int32 MaximumCollisionIterationCount = VerticalBlockData.FallingBlocks.Num() + 1;
		int32 CollisionIterationCount = 0;
		while (RemainingMoveVector.Z < -KINDA_SMALL_NUMBER && bShouldStopFalling == false)
		{
			if (CollisionIterationCount >= MaximumCollisionIterationCount)
			{
				bShouldStopFalling = true;
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

			if (::IsValid(HitBlock))
			{
				if (TryDestroyFallingBlockOnImpact(BottomBlock.Get(), NearestFallingBlock.Get()))
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
						bShouldStopFalling = true;
					}
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
					bShouldStopFalling = true;
				}
			}

			AMKCharacter* HitCharacter = Cast<AMKCharacter>(HitActor);
			if (::IsValid(HitCharacter))
			{
				bShouldStopFalling = true;

				const int32 FallingDamage = CalculateCharacterFallingDamage(BottomBlock.Get(), VerticalBlockData);
				if (FallingDamage > 0)
				{
					FGameplayAbilityUtils::ApplyDamage(
						HitCharacter->GetAbilitySystemComponent(),
						nullptr,
						static_cast<float>(FallingDamage));
				}
			}

			++CollisionIterationCount;
		}

		if (bShouldStopFalling)
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

float FFallingBlockGroupData::CalculateFallingBlockImpactDamage(UObject* WorldContextObject) const
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
	ABlockBase* FallingBlock) const
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

	const float ImpactDamage = CalculateFallingBlockImpactDamage(WorldContextObject);
	if (ImpactDamage <= 0.f)
	{
		return false;
	}

	FGameplayAbilityUtils::ApplyDamageToDurability(FallingAbilitySystemComponent, nullptr, ImpactDamage);

	const float RemainingDurability = BlockAttributeSet->GetDurability();
	return RemainingDurability <= 0.f;
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
	ProcessPendingCollapseBlocks();

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

	if (bForce)
	{
		FixedTileTemplate.Reset();
		MineableTileTemplate.Reset();
		bTileTemplatesCaptured = false;
	}
}

void ULevelManagerSubsystem::GenerateTileActors()
{
	RegenerateMineableBlocks();
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

	CaptureTileTemplates(DataManager, TileMapComp);

	const FBlockDataTableRow* RockBlockData = DataManager->GetBlockDataTableRow(FName(TEXT("Rock")));
	if (RockBlockData == nullptr)
	{
		ensureMsgf(false, TEXT("Can't generate mineable tiles: BlockDataTable row 'Rock' was not found."));
		return;
	}

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

	TMap<FIntPoint, int32> GeneratedMineableTiles;
	GenerateMineableLayout(RandomStream, GenerationSettings, RockBlockData->TileIndex, GeneratedMineableTiles);

	ClearGeneratedBlocks();

	TArray<FIntPoint> FixedTileCoordinates;
	FixedTileTemplate.GenerateKeyArray(FixedTileCoordinates);
	SortTileCoordinates(FixedTileCoordinates);
	for (const FIntPoint& TileCoordinate : FixedTileCoordinates)
	{
		SpawnTileActor(TileCoordinate, FixedTileTemplate[TileCoordinate], TileMapComp, BlockClass);
	}

	TArray<FIntPoint> GeneratedTileCoordinates;
	GeneratedMineableTiles.GenerateKeyArray(GeneratedTileCoordinates);
	SortTileCoordinates(GeneratedTileCoordinates);
	for (const FIntPoint& TileCoordinate : GeneratedTileCoordinates)
	{
		SpawnTileActor(TileCoordinate, GeneratedMineableTiles[TileCoordinate], TileMapComp, BlockClass);
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

ABlockBase* ULevelManagerSubsystem::GetBlockAtPosition(const FVector2D& BlockPosition) const
{
	const TWeakObjectPtr<ABlockBase>* BlockPtr = BlockActorMap.Find(BlockPosition);
	return BlockPtr != nullptr ? BlockPtr->Get() : nullptr;
}

void ULevelManagerSubsystem::CaptureTileTemplates(UDataManager* DataManager, UPaperTileMapComponent* TileMapComponent)
{
	if (bTileTemplatesCaptured || ::IsValid(DataManager) == false || ::IsValid(TileMapComponent) == false)
	{
		return;
	}

	UPaperTileMap* TileMap = TileMapComponent->TileMap;
	if (::IsValid(TileMap) == false)
	{
		return;
	}

	FixedTileTemplate.Reset();
	MineableTileTemplate.Reset();

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
				MineableTileTemplate.Add(TileCoordinate, TileIndex);
			}
			else
			{
				FixedTileTemplate.Add(TileCoordinate, TileIndex);
			}
		}
	}

	bTileTemplatesCaptured = true;
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

void ULevelManagerSubsystem::GenerateMineableLayout(
	FRandomStream& RandomStream,
	const FMineableLevelGenerationSettings& GenerationSettings,
	int32 RockTileIndex,
	TMap<FIntPoint, int32>& OutGeneratedMineableTiles) const
{
	OutGeneratedMineableTiles = MineableTileTemplate;
	if (OutGeneratedMineableTiles.IsEmpty())
	{
		return;
	}

	TArray<FIntPoint> TemplateCoordinates;
	MineableTileTemplate.GenerateKeyArray(TemplateCoordinates);
	SortTileCoordinates(TemplateCoordinates);

	TSet<FIntPoint> ReservedCoordinates;
	TSet<FIntPoint> RandomRockCoordinates;
	TSet<FIntPoint> EmptyCoordinates;

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
					if (MineableTileTemplate.Contains(TileCoordinate) == false || ReservedCoordinates.Contains(TileCoordinate))
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

	int32 CreatedFallingStructureCount = 0;
	bool bCreatedSingleSupportTopHammer = false;
	for (int32 Attempt = 0;
		Attempt < MaxPlacementAttempts && CreatedFallingStructureCount < GenerationSettings.FallingStructureCount;
		++Attempt)
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

		const int32 StructureWidth = RandomStream.RandRange(MinStructureWidth, MaxStructureWidth);
		const int32 StructureHeight = RandomStream.RandRange(MinStructureHeight, MaxStructureHeight);
		const int32 FallingDistance = RandomStream.RandRange(MinFallingDistance, MaxFallingDistance);
		const int32 TargetThickness = RandomStream.RandRange(MinTargetThickness, MaxTargetThickness);
		const EFallingPatternType PatternType = static_cast<EFallingPatternType>(
			Attempt % static_cast<int32>(EFallingPatternType::Count));
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
		default:
			continue;
		}

		if (ApplyFallingPatternStamp(
			PatternStamp,
			MineableTileTemplate,
			OutGeneratedMineableTiles,
			ReservedCoordinates,
			EmptyCoordinates,
			RockTileIndex))
		{
			++CreatedFallingStructureCount;
			if (PatternType == EFallingPatternType::TopHammer)
			{
				bCreatedSingleSupportTopHammer = true;
			}
		}
	}

	const int32 TargetEmptyBlockCount = FMath::RoundToInt(
		MineableTileTemplate.Num() * FMath::Clamp(GenerationSettings.EmptyBlockRatio, 0.f, 1.f));
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
		MineableTileTemplate.Num() * FMath::Clamp(GenerationSettings.RockBlockRatio, 0.f, 1.f));
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
			FixedTileTemplate,
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
}

void ULevelManagerSubsystem::SpawnTileActor(
	const FIntPoint& TileCoordinate,
	int32 TileIndex,
	UPaperTileMapComponent* TileMapComponent,
	TSubclassOf<AActor> BlockClass)
{
	if (::IsValid(TileMapComponent) == false || ::IsValid(TileMapComponent->TileMap) == false)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (::IsValid(World) == false || BlockClass.Get() == nullptr)
	{
		return;
	}

	const FVector LocalPosition = TileMapComponent->GetTileCenterPosition(TileCoordinate.X, TileCoordinate.Y, 0);
	const FVector WorldPosition = TileMapComponent->GetComponentTransform().TransformPosition(LocalPosition);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABlockBase* SpawnedBlock = World->SpawnActor<ABlockBase>(BlockClass, WorldPosition, FRotator::ZeroRotator, SpawnParameters);
	if (::IsValid(SpawnedBlock) == false)
	{
		return;
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

	if (StartBlock->IsMineable() == false)
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
			if (OccupyingBlock == nullptr || OccupyingBlock->IsValid() == false)
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
