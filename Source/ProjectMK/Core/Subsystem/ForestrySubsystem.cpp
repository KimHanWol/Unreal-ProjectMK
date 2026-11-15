#include "ProjectMK/Core/Subsystem/ForestrySubsystem.h"

#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"

void UForestrySubsystem::Deinitialize()
{
	GetWorld()->GetTimerManager().ClearTimer(GrowthTimer);
	Trees.Reset();
	Super::Deinitialize();
}

void UForestrySubsystem::GenerateTrees()
{
	ClearTrees();
	const auto* Settings = UDataManager::Get(this)->GetGameSettingDataAsset();
	if (Settings->Forestry.bEnabled == false || Settings->MineableLevelGeneration.WidthInBlocks < 112) { return; }
	for (int32 Offset : Settings->Forestry.Columns)
	{
		if (Offset < 3 || Offset >= Settings->MineableLevelGeneration.WidthInBlocks - 3) { continue; }
		FManagedTree& Tree = Trees.AddDefaulted_GetRef();
		Tree.Column = Settings->MineableLevelGeneration.StartColumn + Offset;
		TryGrowTree(Tree);
	}
	GetWorld()->GetTimerManager().SetTimer(GrowthTimer, this, &UForestrySubsystem::OnGrowthTimer, 1.f, true);
}

void UForestrySubsystem::ClearTrees()
{
	GetWorld()->GetTimerManager().ClearTimer(GrowthTimer);
	for (const auto& Tree : Trees) { for (const auto& Block : Tree.Pieces) { if (Block.IsValid()) { Block->SetItemDropEnabled(false); Block->Destroy(); } } }
	Trees.Reset();
}

void UForestrySubsystem::RestoreTrees(const TArray<FTreeSaveData>& Saved)
{
	if (Saved.IsEmpty()) { return; }
	ClearTrees();
	ULevelManagerSubsystem* Level = GetWorld()->GetSubsystem<ULevelManagerSubsystem>();
	const auto* Settings = UDataManager::Get(this)->GetGameSettingDataAsset();
	for (const auto& Record : Saved)
	{
		if (Record.Column < Settings->MineableLevelGeneration.StartColumn || Record.Column >= Settings->MineableLevelGeneration.StartColumn + Settings->MineableLevelGeneration.WidthInBlocks) { continue; }
		FManagedTree& Tree = Trees.AddDefaulted_GetRef(); Tree.Column = Record.Column; Tree.RegrowAt = Record.RegrowAt;
		for (const auto& Piece : Record.Pieces)
		{
			if (Piece.BlockKey != TEXT("TreeTrunk") && Piece.BlockKey != TEXT("TreeLeaves")) { continue; }
			if (ABlockBase* Cover = Level->GetBlockAtWorldLocation(Piece.Location))
			{
				if (Cover->IsMineable() == false) { continue; }
				Cover->SetItemDropEnabled(false); Cover->Destroy();
			}
			if (Level->TryPlaceBlock(Piece.BlockKey, Piece.Location)) { Tree.Pieces.Add(Level->GetBlockAtWorldLocation(Piece.Location)); }
		}
	}
	GetWorld()->GetTimerManager().SetTimer(GrowthTimer, this, &UForestrySubsystem::OnGrowthTimer, 1.f, true);
}

TArray<FTreeSaveData> UForestrySubsystem::GetSaveData() const
{
	TArray<FTreeSaveData> Saved;
	const auto* Trunk = UDataManager::Get(const_cast<UForestrySubsystem*>(this))->GetBlockDataTableRow(TEXT("TreeTrunk"));
	for (const auto& Tree : Trees)
	{
		FTreeSaveData& Record = Saved.AddDefaulted_GetRef(); Record.Column = Tree.Column; Record.RegrowAt = Tree.RegrowAt;
		for (const auto& Block : Tree.Pieces)
		{
			if (Block.IsValid() == false || Block->IsBreaking()) { continue; }
			FTreePieceSaveData& Piece = Record.Pieces.AddDefaulted_GetRef(); Piece.BlockKey = Trunk != nullptr && Block->GetTileIndex() == Trunk->TileIndex ? TEXT("TreeTrunk") : TEXT("TreeLeaves"); Piece.Location = Block->GetActorLocation();
		}
	}
	return Saved;
}

const TArray<FManagedTree>& UForestrySubsystem::GetTrees() const { return Trees; }

bool UForestrySubsystem::TryGrowTree(FManagedTree& Tree)
{
	ULevelManagerSubsystem* Level = GetWorld()->GetSubsystem<ULevelManagerSubsystem>();
	const int32 GroundRow = Level->GetSurfaceRow(Tree.Column);
	ABlockBase* Ground = Level->GetBlockAtPosition(FVector2D(Tree.Column, GroundRow));
	if (Ground == nullptr || Ground->IsBreaking() || Ground->IsFalling()) { return false; }
	TArray<TPair<FIntPoint, FName>> Shape;
	for (int32 H = 1; H <= 3; ++H) { Shape.Add({FIntPoint(Tree.Column, GroundRow - H), TEXT("TreeTrunk")}); }
	for (int32 H = 3; H <= 5; ++H)
	{
		for (int32 X = -2; X <= 2; ++X)
		{
			if ((X == 0 && H == 3) || (H == 5 && FMath::Abs(X) == 2)) { continue; }
			Shape.Add({ FIntPoint(Tree.Column + X, GroundRow - H), TEXT("TreeLeaves") });
		}
	}
	for (const auto& Part : Shape)
	{
		if (Level->GetBlockAtPosition(FVector2D(Part.Key)) != nullptr) { return false; }
		const FVector Center((Part.Key.X + 0.5f) * 16.f, -20.f, -(Part.Key.Y + 0.5f) * 16.f);
		if (GetWorld()->OverlapAnyTestByObjectType(Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(FVector(7.2f)))) { return false; }
	}
	for (const auto& Part : Shape)
	{
		const FVector Center((Part.Key.X + 0.5f) * 16.f, -20.f, -(Part.Key.Y + 0.5f) * 16.f);
		if (Level->TryPlaceBlock(Part.Value, Center) == false)
		{
			for (const auto& Block : Tree.Pieces) { if (Block.IsValid()) { Block->SetItemDropEnabled(false); Block->Destroy(); } }
			Tree.Pieces.Reset(); return false;
		}
		Tree.Pieces.Add(Level->GetBlockAtPosition(FVector2D(Part.Key)));
	}
	Tree.RegrowAt = 0; return true;
}

void UForestrySubsystem::OnGrowthTimer()
{
	bool bChanged = false;
	const int64 Now = FDateTime::UtcNow().ToUnixTimestamp();
	for (auto& Tree : Trees)
	{
		Tree.Pieces.RemoveAll([](const auto& Block) { return Block.IsValid() == false; });
		if (Tree.Pieces.IsEmpty() == false) { continue; }
		if (Tree.RegrowAt == 0) { Tree.RegrowAt = Now + UDataManager::Get(this)->GetGameSettingDataAsset()->Forestry.RegrowSeconds; bChanged = true; }
		else if (Now >= Tree.RegrowAt) { bChanged |= TryGrowTree(Tree); }
	}
	if (bChanged) { if (auto* Player = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0))) { Player->TrySaveGame(); } }
}
