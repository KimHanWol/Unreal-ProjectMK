#include "ProjectMK/Core/Subsystem/ForestrySubsystem.h"

#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "PaperSprite.h"
#include "PaperSpriteActor.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"
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
	TreeBlockKeys.Reset();
	for (FName Key : { FName(TEXT("TreeTrunk")), FName(TEXT("TreeLeaves")) }) { if (const auto* Row = UDataManager::Get(this)->GetBlockDataTableRow(Key)) { TreeBlockKeys.Add(Row->TileIndex, Key); } }
	for (int32 Y = 0; Y < 3; ++Y) { for (int32 X = 0; X < 5; ++X)
	{
		const FName Key(*FString::Printf(TEXT("TreeLeaves_%d_%d"), X, Y));
		if (const auto* Row = UDataManager::Get(this)->GetBlockDataTableRow(Key)) { TreeBlockKeys.Add(Row->TileIndex, Key); }
	} }
	for (int32 Offset : Settings->Forestry.Columns)
	{
		if (Offset < Settings->Forestry.YardStartColumn + 3 || Offset > Settings->Forestry.YardEndColumn - 9 || Offset >= Settings->MineableLevelGeneration.WidthInBlocks - 3) { continue; }
		FManagedTree& Tree = Trees.AddDefaulted_GetRef();
		Tree.Column = Settings->MineableLevelGeneration.StartColumn + Offset;
		TryGrowTree(Tree);
	}
	GenerateYardScenery();
	GetWorld()->GetTimerManager().SetTimer(GrowthTimer, this, &UForestrySubsystem::OnGrowthTimer, 1.f, true);
}

void UForestrySubsystem::ClearTrees()
{
	GetWorld()->GetTimerManager().ClearTimer(GrowthTimer);
	for (const auto& Tree : Trees) { for (const auto& Block : Tree.Pieces) { if (Block.IsValid()) { Block->SetItemDropEnabled(false); Block->Destroy(); } } }
	Trees.Reset();
	for (const auto& Actor : YardScenery) { if (Actor.IsValid()) { Actor->Destroy(); } }
	YardScenery.Reset();
}

void UForestrySubsystem::RestoreTrees(const TArray<FTreeSaveData>& Saved)
{
	if (Saved.IsEmpty()) { return; }
	ClearTrees();
	ULevelManagerSubsystem* Level = GetWorld()->GetSubsystem<ULevelManagerSubsystem>();
	const auto* Settings = UDataManager::Get(this)->GetGameSettingDataAsset();
	for (const auto& Record : Saved)
	{
		if (Settings->Forestry.bEnabled == false || Settings->MineableLevelGeneration.WidthInBlocks < 112) { continue; }
		int32 Column = Record.Column;
		const int32 PreviousOffset = Column - Settings->MineableLevelGeneration.StartColumn;
		if (Settings->Forestry.Columns.Contains(PreviousOffset) == false)
		{
			// Version 1 saves used road plots 14/22/30. Preserve their partial harvest on the new left-hand plots.
			const TArray<int32> LegacyColumns = {14,22,30};
			const int32 Index = LegacyColumns.IndexOfByKey(PreviousOffset);
			if (Settings->Forestry.Columns.IsValidIndex(Index) == false) { continue; }
			Column = Settings->MineableLevelGeneration.StartColumn + Settings->Forestry.Columns[Index];
		}
		FManagedTree& Tree = Trees.AddDefaulted_GetRef(); Tree.Column = Column; Tree.RegrowAt = Record.RegrowAt;
		const int32 GroundRow = Level->GetSurfaceRow(Tree.Column);
		int32 SavedGroundRow = Record.GroundRow;
		if (SavedGroundRow == INDEX_NONE)
		{
			// V1 saves have absolute piece locations but no root row; recover the previous authored surface once.
			int32 Raise = 0;
			for (const auto& Rise : Settings->MineableLevelGeneration.SurfaceRises)
			{
				const int32 Offset = Tree.Column - Settings->MineableLevelGeneration.StartColumn - Rise.StartColumnOffset;
				const int32 Span = FMath::Max(8, Rise.WidthInBlocks);
				if (Offset < 0 || Offset >= Span) { continue; }
				Raise = FMath::Max(Raise, FMath::Min(FMath::Min(FMath::Clamp(Rise.HeightInBlocks, 1, 3), Settings->MineableLevelGeneration.SurfaceReliefHeight), 1 + FMath::Min(Offset, Span - 1 - Offset) / 3));
			}
			SavedGroundRow = Settings->SurfaceBlockPositionY - Raise;
		}
		const FVector Shift((Column-Record.Column)*16.f, 0.f, (SavedGroundRow - GroundRow) * 16.f);
		for (const auto& Piece : Record.Pieces)
		{
			const FVector Location = Piece.Location + Shift;
			FName Key = Piece.BlockKey;
			if (Key == TEXT("TreeLeaves"))
			{
				const FVector2D Cell = UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(Location);
				const int32 X = FMath::RoundToInt(Cell.X) - Tree.Column + 2;
				const int32 Y = 5 - (Level->GetSurfaceRow(Tree.Column) - FMath::RoundToInt(Cell.Y));
				if (X >= 0 && X < 5 && Y >= 0 && Y < 3) { Key = FName(*FString::Printf(TEXT("TreeLeaves_%d_%d"), X, Y)); }
			}
			const auto* Row = UDataManager::Get(this)->GetBlockDataTableRow(Key);
			if (Row == nullptr || TreeBlockKeys.FindRef(Row->TileIndex) != Key) { continue; }
			if (ABlockBase* Cover = Level->GetBlockAtWorldLocation(Location))
			{
				if (Cover->IsMineable() == false) { continue; }
				Cover->SetItemDropEnabled(false); Cover->Destroy();
			}
			if (Level->TryPlaceBlock(Key, Location)) { Tree.Pieces.Add(Level->GetBlockAtWorldLocation(Location)); }
		}
	}
	if (Trees.IsEmpty() && Settings->Forestry.bEnabled && Settings->MineableLevelGeneration.WidthInBlocks >= 112) { GenerateTrees(); return; }
	GenerateYardScenery();
	GetWorld()->GetTimerManager().SetTimer(GrowthTimer, this, &UForestrySubsystem::OnGrowthTimer, 1.f, true);
}

TArray<FTreeSaveData> UForestrySubsystem::GetSaveData() const
{
	TArray<FTreeSaveData> Saved;
	for (const auto& Tree : Trees)
	{
		FTreeSaveData& Record = Saved.AddDefaulted_GetRef(); Record.Column = Tree.Column; Record.GroundRow = GetWorld()->GetSubsystem<ULevelManagerSubsystem>()->GetSurfaceRow(Tree.Column); Record.RegrowAt = Tree.RegrowAt;
		for (const auto& Block : Tree.Pieces)
		{
			if (Block.IsValid() == false || Block->IsBreaking()) { continue; }
			const FName Key = TreeBlockKeys.FindRef(Block->GetTileIndex());
			if (Key.IsNone()) { continue; }
			FTreePieceSaveData& Piece = Record.Pieces.AddDefaulted_GetRef(); Piece.BlockKey = Key; Piece.Location = Block->GetActorLocation();
		}
	}
	return Saved;
}

const TArray<FManagedTree>& UForestrySubsystem::GetTrees() const { return Trees; }

void UForestrySubsystem::GenerateYardScenery()
{
	const auto* Settings = UDataManager::Get(this)->GetGameSettingDataAsset();
	const auto* Style = UDataManager::Get(this)->GetVisualStyleDataAsset();
	if (Style == nullptr || Trees.IsEmpty()) { return; }
	ULevelManagerSubsystem* Level = GetWorld()->GetSubsystem<ULevelManagerSubsystem>();
	auto Place = [&](FName Key, float Offset, float Layer, float BaseOffset)
	{
		UPaperSprite* Sprite = Key == TEXT("LanternPost") ? Style->MineDecorations.FindRef(Key) : Style->TownSprites.FindRef(Key);
		if (Sprite == nullptr) { return static_cast<APaperSpriteActor*>(nullptr); }
		const float Column = Settings->MineableLevelGeneration.StartColumn + Offset;
		const float Floor = -Level->GetSurfaceRow(FMath::FloorToInt(Column)) * 16.f;
		const FVector Location((Column+0.5f)*16.f,Layer,Floor+BaseOffset+(Key == TEXT("TreeStatus") ? 2.5f : Sprite->GetSourceSize().Y/Sprite->GetPixelsPerUnrealUnit()*0.5f));
		APaperSpriteActor* Actor = GetWorld()->SpawnActor<APaperSpriteActor>(APaperSpriteActor::StaticClass(),Location,FRotator::ZeroRotator);
		if (Actor == nullptr) { return Actor; }
		Actor->Tags.Add(TEXT("LumberyardScenery")); Actor->Tags.Add(Key); Actor->SetActorEnableCollision(false); Actor->SetActorTickEnabled(false);
		auto* Render = Actor->GetRenderComponent(); Render->SetMobility(EComponentMobility::Movable); Render->SetSprite(Sprite); Render->SetCollisionEnabled(ECollisionEnabled::NoCollision); Render->SetComponentTickEnabled(false);

		YardScenery.Add(Actor); return Actor;
	};
	Place(TEXT("LumberyardEnclosure"),Settings->Forestry.YardStartColumn+13.f,-10.f,-18.f);
	Place(TEXT("Lumberyard"),Settings->Forestry.YardEndColumn-7.f,-52.f,0.f);
	Place(TEXT("LanternPost"),Settings->Forestry.YardStartColumn+1.f,-52.f,0.f);
	Place(TEXT("LanternPost"),Settings->Forestry.YardEndColumn-1.f,-52.f,0.f);
	for (int32 Index = 0; Index < Trees.Num(); ++Index)
	{
		auto& Tree = Trees[Index];
		const float LampPixels[] = {145.f,383.f,622.f};
		const float LampColumn = Settings->Forestry.YardStartColumn + 13.f + (LampPixels[FMath::Min(Index,2)]-384.f)/32.f;
		APaperSpriteActor* Lamp = Place(TEXT("TreeStatus"),LampColumn,-8.f,12.f);
		if (Lamp != nullptr) { Tree.StatusLamp = Lamp->GetRenderComponent(); UpdateTreeStatus(Tree); }
	}
}

bool UForestrySubsystem::TryGrowTree(FManagedTree& Tree)
{
	ULevelManagerSubsystem* Level = GetWorld()->GetSubsystem<ULevelManagerSubsystem>();
	const int32 GroundRow = Level->GetSurfaceRow(Tree.Column);
	ABlockBase* Ground = Level->GetBlockAtPosition(FVector2D(Tree.Column, GroundRow));
	if (Ground == nullptr || Ground->IsBreaking() || Ground->IsFalling()) { return false; }
	TArray<TPair<FIntPoint, FName>> Shape;
	for (int32 H = 1; H <= 2; ++H) { Shape.Add({FIntPoint(Tree.Column, GroundRow - H), TEXT("TreeTrunk")}); }
	for (int32 Y = 0; Y < 3; ++Y) { for (int32 X = 0; X < 5; ++X)
	{
		const FName Key(*FString::Printf(TEXT("TreeLeaves_%d_%d"), X, Y));
		const auto* Row = UDataManager::Get(this)->GetBlockDataTableRow(Key);
		if (Row != nullptr) { Shape.Add({ FIntPoint(Tree.Column + X - 2, GroundRow - 5 + Y), Key }); }
	} }
	if (Shape.Num() < 7) { return false; }
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

void UForestrySubsystem::UpdateTreeStatus(FManagedTree& Tree)
{
	if (Tree.StatusLamp.IsValid() == false) { return; }
	int32 Remaining = 0;
	for (const auto& Piece : Tree.Pieces) { Remaining += Piece.IsValid() && Piece->IsBreaking() == false; }
	const FLinearColor Color = Remaining == 0 ? FLinearColor(0.8f,0.5f,0.12f) : (Remaining >= 17 ? FLinearColor(0.2f,0.8f,0.35f) : FLinearColor(0.85f,0.18f,0.1f));
	Tree.StatusLamp->SetSpriteColor(Color);
}

void UForestrySubsystem::OnGrowthTimer()
{
	bool bChanged = false;
	const int64 Now = FDateTime::UtcNow().ToUnixTimestamp();
	for (auto& Tree : Trees)
	{
		Tree.Pieces.RemoveAll([](const auto& Block) { return Block.IsValid() == false; });
		UpdateTreeStatus(Tree);
		if (Tree.Pieces.IsEmpty() == false) { continue; }
		if (Tree.RegrowAt == 0) { Tree.RegrowAt = Now + UDataManager::Get(this)->GetGameSettingDataAsset()->Forestry.RegrowSeconds; bChanged = true; }
		else if (Now >= Tree.RegrowAt) { bChanged |= TryGrowTree(Tree); }
	}
	if (bChanged) { if (auto* Player = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0))) { Player->TrySaveGame(); } }
}
