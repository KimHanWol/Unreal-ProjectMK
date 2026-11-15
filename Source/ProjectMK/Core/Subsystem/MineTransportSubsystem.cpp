#include "ProjectMK/Core/Subsystem/MineTransportSubsystem.h"

#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Actor/Shop/StorageChestActor.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Transport/RailTrackActor.h"
#include "ProjectMK/Actor/Transport/MineCartActor.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"
#include "Engine/OverlapResult.h"
#include "ProjectMK/Core/Manager/DataManager.h"

void UMineTransportSubsystem::Tick(float DeltaSeconds)
{
	if (GetWorld()->IsGameWorld() == false || bClearing) { return; }
	SupportCheckTime += DeltaSeconds;
	if (SupportCheckTime >= 0.1f) { SupportCheckTime = 0.f; UpdateUnsupportedRails(); }
	float Remaining = FMath::Min(DeltaSeconds, 0.25f);
	while (Remaining > KINDA_SMALL_NUMBER)
	{
		const float Step = FMath::Min(Remaining, 0.025f);
		for (const auto& Cart : Carts)
		{
			if (Cart.IsValid()) { const FVector Before = Cart->GetActorLocation(); Cart->UpdateCart(Step); bDirty |= FVector::DistSquared(Before, Cart->GetActorLocation()) > 0.01f; }
		}
		Remaining -= Step;
	}
}

TStatId UMineTransportSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UMineTransportSubsystem, STATGROUP_Tickables); }

void UMineTransportSubsystem::ClearNetwork()
{
	TGuardValue<bool> Guard(bClearing, true);
	for (const auto& Cart : Carts) { if (Cart.IsValid()) { Cart->Destroy(); } }
	for (const auto& Rail : Rails) { if (Rail.IsValid()) { Rail->Destroy(); } }
	Carts.Reset(); Rails.Reset(); bDirty = false;
}

bool UMineTransportSubsystem::TryPlaceRail(AMKCharacter* Player)
{
	if (Player == nullptr) { return false; }
	ULevelManagerSubsystem* Level = GetWorld()->GetSubsystem<ULevelManagerSubsystem>();
	ABlockBase* Ground = Level->GetBlockAtWorldLocation(Player->GetActorLocation());
	if (Ground == nullptr || Ground->IsClimbableSupport() == false) { Ground = Level->GetBlockBelowWorldLocation(Player->GetActorLocation()); }
	if (Ground == nullptr) { return false; }
	const FVector2D Cell = UMKBlueprintFunctionLibrary::GetBlockPosition(Ground);
	const FIntPoint CellNode(FMath::RoundToInt(Cell.X), FMath::RoundToInt(Cell.Y));
	if (Ground->IsClimbableSupport())
	{
		for (const auto& Existing : Rails)
		{
			if (Existing.IsValid() == false) { continue; }
			const auto& Edge = Existing->GetSaveData();
			for (const FIntPoint& Node : {Edge.Start,Edge.End})
			{
				if (GetNodeDegree(Node) != 1 || Node.Y != CellNode.Y) { continue; }
				const FIntPoint Other = Node == Edge.Start ? Edge.End : Edge.Start;
				if (Node.Y <= Other.Y) { continue; }
				const int32 Step = Node.X > Other.X ? -1 : 1;
				if ((Node.X == CellNode.X || Node.X == CellNode.X+1) && TryPlaceRailSegment(Node, Node + FIntPoint(Step,1))) { return true; }
			}
		}
	}
	// Each placed piece spans this ground cell. Prefer an existing free boundary endpoint.
	for (const auto& Existing : Rails)
	{
		if (Existing.IsValid() == false) { continue; }
		for (const FIntPoint& Node : { Existing->GetSaveData().Start, Existing->GetSaveData().End })
		{
			if (GetNodeDegree(Node) != 1 || FMath::Abs(Node.Y - CellNode.Y) > 1) { continue; }
			if (Node.X == CellNode.X && TryPlaceRailSegment(Node, CellNode + FIntPoint(1, 0))) { return true; }
			if (Node.X == CellNode.X + 1 && TryPlaceRailSegment(CellNode, Node)) { return true; }
		}
	}
	return TryPlaceRailSegment(CellNode, CellNode + FIntPoint(1, 0));
}

bool UMineTransportSubsystem::TryPlaceRailSegment(const FIntPoint& Start, const FIntPoint& End, FGuid Id, bool bKeepEndDisconnected)
{
	if (FMath::Abs(Start.X - End.X) != 1 || FMath::Abs(Start.Y - End.Y) > 1 || GetNodeDegree(Start) >= 2 || GetNodeDegree(End) >= 2) { return false; }
	for (const auto& Existing : Rails) { if (Existing.IsValid() && ((Existing->GetSaveData().Start == Start && Existing->GetSaveData().End == End) || (Existing->GetSaveData().Start == End && Existing->GetSaveData().End == Start))) { return false; } }
	const bool bDisconnect = bKeepEndDisconnected || AreNodesConnected(Start, End);
	if (IsNodeSupported(Start) == false || IsNodeSupported(End) == false) { return false; }
	for (const auto& Existing : Rails)
	{
		if (Existing.IsValid() == false) { continue; }
		const auto& Edge = Existing->GetSaveData();
		if (Edge.Start == Start || Edge.Start == End || Edge.End == Start || Edge.End == End) { continue; }
		FVector Hit;
		if (FMath::SegmentIntersection2D(FVector(Start.X, Start.Y, 0.f), FVector(End.X, End.Y, 0.f), FVector(Edge.Start.X, Edge.Start.Y, 0.f), FVector(Edge.End.X, Edge.End.Y, 0.f), Hit)) { return false; }
	}
	FRailSaveData Data; Data.Id = Id.IsValid() ? Id : FGuid::NewGuid(); Data.Start = Start.X < End.X ? Start : End; Data.End = Start.X < End.X ? End : Start; Data.bEndDisconnected = bDisconnect;
	ARailTrackActor* Rail = GetWorld()->SpawnActor<ARailTrackActor>();
	if (Rail == nullptr) { return false; }
	Rail->InitializeRail(Data); Rails.Add(Rail); UpdateEndCaps(); bDirty = true;
	return true;
}

bool UMineTransportSubsystem::TryPlaceCart(AMKCharacter* Player, bool bAutomatic)
{
	if (Player == nullptr) { return false; }
	for (const auto& Rail : Rails)
	{
		if (Rail.IsValid() == false || FVector::DistSquared(Player->GetActorLocation(), FMath::ClosestPointOnSegment(Player->GetActorLocation(), Rail->GetStartWorld(), Rail->GetEndWorld())) > FMath::Square(36.f)) { continue; }
		for (float Alpha : { 0.5f, 0.1f, 0.9f })
		{
			const FVector Location = AMineCartActor::CalculateRailPosition(Rail.Get(),Alpha);
			const FVector Direction = AMineCartActor::CalculateRailDirection(Rail.Get(),Alpha);
			const FQuat Rotation(FVector::RightVector,-FMath::Atan2(Direction.Z,Direction.X));
			if ((FMath::Abs(Location.X-Player->GetActorLocation().X)<12.f && FMath::Abs(Location.Z-Player->GetActorLocation().Z)<12.f) || CanCartOccupy(nullptr,Location,Rotation)==false) { continue; }
			AMineCartActor* Cart = GetWorld()->SpawnActor<AMineCartActor>();
			if (Cart == nullptr) { return false; }
			FMineCartSaveData Data; Data.Id = FGuid::NewGuid(); Data.RailId = Rail->GetSaveData().Id; Data.Position = Alpha; Data.bAutomatic = bAutomatic;
			Cart->InitializeCart(Data, Rail.Get()); Carts.Add(Cart); bDirty = true; return true;
		}
	}
	return false;
}

bool UMineTransportSubsystem::TryMountChest(AMKCharacter* Player, FName ChestItemKey)
{
	AMineCartActor* Cart = Player != nullptr ? FindNearbyCart(Player->GetActorLocation()) : nullptr;
	if (Cart == nullptr || Cart->GetCargo() != nullptr) { return false; }
	return GetWorld()->GetSubsystem<ULevelManagerSubsystem>()->SpawnMountedStorageChest(ChestItemKey, Cart) != nullptr;
}

void UMineTransportSubsystem::RestoreNetwork(const TArray<FRailSaveData>& SavedRails, const TArray<FMineCartSaveData>& SavedCarts)
{
	ClearNetwork();
	ULevelManagerSubsystem* Level = GetWorld()->GetSubsystem<ULevelManagerSubsystem>();
	for (const auto& Edge : SavedRails)
	{
		for (const auto& Node : { Edge.Start, Edge.End })
		{
			for (int32 Height = 1; Height <= 2; ++Height)
			{
				if (ABlockBase* Block = Level->GetBlockAtPosition(FVector2D(Node.X, Node.Y - Height))) { if (Block->IsMineable() && Block->IsClimbableSupport() == false) { Block->SetItemDropEnabled(false); Block->Destroy(); } }
			}
		}
		TryPlaceRailSegment(Edge.Start, Edge.End, Edge.Id, Edge.bEndDisconnected);
	}
	for (const auto& Data : SavedCarts)
	{
		const auto* Rail = Rails.FindByPredicate([&](const auto& Entry) { return Entry.IsValid() && Entry->GetSaveData().Id == Data.RailId; });
		if (Rail == nullptr || FindCart(Data.Id) != nullptr) { continue; }
		AMineCartActor* Cart = GetWorld()->SpawnActor<AMineCartActor>();
		if (Cart != nullptr) { Cart->InitializeCart(Data, Rail->Get()); Carts.Add(Cart); }
	}
	bDirty = false;
}

void UMineTransportSubsystem::NotifyRailRemoved(ARailTrackActor* Rail)
{
	if (bClearing) { return; }
	Rails.RemoveAll([Rail](const auto& Entry) { return Entry.IsValid() == false || Entry.Get() == Rail; });
	UpdateEndCaps(); bDirty = true;
}

void UMineTransportSubsystem::NotifyCartRemoved(AMineCartActor* Cart)
{
	if (bClearing) { return; }
	Carts.RemoveAll([Cart](const auto& Entry) { return Entry.IsValid() == false || Entry.Get() == Cart; }); bDirty = true;
}

ARailTrackActor* UMineTransportSubsystem::FindNextRail(const ARailTrackActor* Current, FIntPoint Node) const
{
	if (Current != nullptr && Current->GetSaveData().bEndDisconnected && Current->GetSaveData().End == Node) { return nullptr; }
	for (const auto& Rail : Rails) { if (Rail.IsValid() && Rail.Get() != Current && (Rail->GetSaveData().Start == Node || (Rail->GetSaveData().End == Node && Rail->GetSaveData().bEndDisconnected == false))) { return Rail.Get(); } }
	return nullptr;
}

AMineCartActor* UMineTransportSubsystem::FindNearbyCart(const FVector& Location) const
{
	AMineCartActor* Found = nullptr; float Distance = FMath::Square(32.f);
	for (const auto& Cart : Carts) { if (Cart.IsValid() && FVector::DistSquared(Cart->GetActorLocation(), Location) < Distance) { Found = Cart.Get(); Distance = FVector::DistSquared(Cart->GetActorLocation(), Location); } }
	return Found;
}

AMineCartActor* UMineTransportSubsystem::FindCart(FGuid Id) const
{
	for (const auto& Cart : Carts) { if (Cart.IsValid() && Cart->GetCartId() == Id) { return Cart.Get(); } }
	return nullptr;
}

bool UMineTransportSubsystem::HasCartOnLine(const ARailTrackActor* Rail) const
{
	if (Rail == nullptr) { return false; }
	for (const auto& Cart : Carts) { if (Cart.IsValid() && Cart->GetRail() != nullptr && AreNodesConnected(Rail->GetSaveData().Start, Cart->GetRail()->GetSaveData().Start)) { return true; } }
	return false;
}

bool UMineTransportSubsystem::HasRailAtSupport(FIntPoint Cell) const
{
	for (const auto& Rail : Rails) { if (Rail.IsValid() && (Rail->GetSaveData().Start == Cell || Rail->GetSaveData().End == Cell || Rail->GetSaveData().Start == Cell + FIntPoint(1, 0) || Rail->GetSaveData().End == Cell + FIntPoint(1, 0))) { return true; } }
	return false;
}

bool UMineTransportSubsystem::CanCartOccupy(const AMineCartActor* Cart, const FVector& Location, const FQuat& Rotation) const
{
	for (const auto& Other : Carts) { if (Other.IsValid() && Other.Get() != Cart && FVector::DistSquared(Other->GetActorLocation(), Location) < FMath::Square(23.f)) { return false; } }
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MineCartClearance),false);
	if (Cart != nullptr) { Params.AddIgnoredActor(Cart); if (Cart->GetCargo() != nullptr) { Params.AddIgnoredActor(Cart->GetCargo()); } }
	TArray<FOverlapResult> Hits;
	FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic); Objects.AddObjectTypesToQuery(ECC_Pawn);
	GetWorld()->OverlapMultiByObjectType(Hits,Location,Rotation,Objects,FCollisionShape::MakeBox(FVector(9.f,6.f,4.4f)),Params);
	for (const auto& Hit : Hits)
	{
		if (AMKCharacter* Player = Cast<AMKCharacter>(Hit.GetActor()))
		{
			// A touching pusher must be able to separate from the cart. Approaching pawns always stop it.
			if (Cart != nullptr && FVector::DotProduct(Location-Cart->GetActorLocation(),Player->GetActorLocation()-Cart->GetActorLocation()) < 0.f) { continue; }
			return false;
		}
		ABlockBase* Block = Cast<ABlockBase>(Hit.GetActor());
		const auto* Row = Block != nullptr ? UDataManager::Get(const_cast<UMineTransportSubsystem*>(this))->GetBlockDataTableRow(Block->GetTileIndex()) : nullptr;
		if (Row != nullptr && Row->bHasCollision && Row->bIsPassableByCharacter == false && Block->IsBreaking() == false) { return false; }
	}
	return true;
}

bool UMineTransportSubsystem::IsDirty() const { return bDirty; }
void UMineTransportSubsystem::ClearDirty() { bDirty = false; }
TArray<FRailSaveData> UMineTransportSubsystem::GetRailSaveData() const { TArray<FRailSaveData> Result; for (const auto& Rail : Rails) { if (Rail.IsValid()) { Result.Add(Rail->GetSaveData()); } } return Result; }
TArray<FMineCartSaveData> UMineTransportSubsystem::GetCartSaveData() const { TArray<FMineCartSaveData> Result; for (const auto& Cart : Carts) { if (Cart.IsValid()) { Result.Add(Cart->GetSaveData()); } } return Result; }
const TArray<TWeakObjectPtr<ARailTrackActor>>& UMineTransportSubsystem::GetRails() const { return Rails; }
const TArray<TWeakObjectPtr<AMineCartActor>>& UMineTransportSubsystem::GetCarts() const { return Carts; }

bool UMineTransportSubsystem::IsNodeSupported(FIntPoint Node) const
{
	ULevelManagerSubsystem* Level = GetWorld()->GetSubsystem<ULevelManagerSubsystem>();
	for (int32 Column : { Node.X - 1, Node.X })
	{
		ABlockBase* Floor = Level->GetBlockAtPosition(FVector2D(Column, Node.Y));
		if (Floor == nullptr)
		{
			TArray<FOverlapResult> Hits;
			GetWorld()->OverlapMultiByObjectType(Hits,FVector((Column+0.5f)*16.f,-20.f,-(Node.Y+0.5f)*16.f),FQuat::Identity,FCollisionObjectQueryParams::AllObjects,FCollisionShape::MakeBox(FVector(2.f,8.f,2.f)));
			for (const auto& Hit : Hits)
			{
				ABlockBase* Candidate=Cast<ABlockBase>(Hit.GetActor());
				if (Candidate!=nullptr && Level->IsBlockShaking(Candidate)) { Floor=Candidate;break; }
			}
		}
		ABlockBase* Above = Level->GetBlockAtPosition(FVector2D(Column, Node.Y - 1));
		if (Floor != nullptr && Floor->IsBreaking() == false && (Floor->IsFalling() == false || Level->IsBlockShaking(Floor)) &&
			(Above == nullptr || Above->IsClimbableSupport())) { return true; }
	}
	return false;
}

bool UMineTransportSubsystem::IsRailCovered(const ARailTrackActor* Rail) const
{
	for (float Alpha : {0.15f,0.5f,0.85f})
	{
		const FVector Position = FMath::Lerp(Rail->GetStartWorld(),Rail->GetEndWorld(),Alpha) + FVector(0.f,0.f,2.f);
		TArray<FOverlapResult> Hits;
		GetWorld()->OverlapMultiByObjectType(Hits,Position,FQuat::Identity,FCollisionObjectQueryParams::AllObjects,FCollisionShape::MakeBox(FVector(0.3f,8.f,0.3f)));
		for (const auto& Hit : Hits)
		{
			ABlockBase* Block = Cast<ABlockBase>(Hit.GetActor());
			if (Block != nullptr && Block->IsBreaking() == false && Block->IsClimbableSupport() == false) { return true; }
		}
	}
	return false;
}

void UMineTransportSubsystem::UpdateUnsupportedRails()
{
	const auto Snapshot = Rails;
	for (const auto& Rail : Snapshot)
	{
		if (Rail.IsValid() == false || (IsNodeSupported(Rail->GetSaveData().Start) && IsNodeSupported(Rail->GetSaveData().End) && IsRailCovered(Rail.Get()) == false)) { continue; }
		const auto CartSnapshot = Carts;
		for (const auto& Cart : CartSnapshot)
		{
			if (Cart.IsValid() == false || Cart->GetRail() != Rail.Get()) { continue; }
			if (AStorageChestActor* Cargo = Cart->GetCargo()) { Cargo->TryBreakByExplosion(); }
			if (AItemBase* Drop = GetWorld()->SpawnActor<AItemBase>(AItemBase::StaticClass(), Cart->GetActorLocation(), FRotator::ZeroRotator))
			{
				Drop->InitializeItemBase(Cart->GetSaveData().bAutomatic ? TEXT("AutoMineCart") : TEXT("MineCart"));
			}
			Cart->Destroy();
		}
		if (AItemBase* Drop = GetWorld()->SpawnActor<AItemBase>(AItemBase::StaticClass(), Rail->GetActorLocation(), FRotator::ZeroRotator)) { Drop->InitializeItemBase(TEXT("Rail")); }
		Rail->Destroy();
	}
}

bool UMineTransportSubsystem::AreNodesConnected(FIntPoint A, FIntPoint B) const
{
	TArray<FIntPoint> Pending = { A }; TSet<FIntPoint> Seen;
	while (Pending.IsEmpty() == false)
	{
		const FIntPoint Node = Pending.Pop(EAllowShrinking::No);
		if (Node == B) { return true; }
		if (Seen.Contains(Node)) { continue; }
		Seen.Add(Node);
		for (const auto& Rail : Rails)
		{
			if (Rail.IsValid() == false) { continue; }
			const auto& Edge = Rail->GetSaveData();
			if (Edge.bEndDisconnected) { continue; }
			if (Edge.Start == Node) { Pending.Add(Edge.End); }
			else if (Edge.End == Node) { Pending.Add(Edge.Start); }
		}
	}
	return false;
}

int32 UMineTransportSubsystem::GetNodeDegree(FIntPoint Node) const
{
	int32 Count = 0;
	for (const auto& Rail : Rails) { if (Rail.IsValid() && (Rail->GetSaveData().Start == Node || (Rail->GetSaveData().End == Node && Rail->GetSaveData().bEndDisconnected == false))) { ++Count; } }
	return Count;
}

void UMineTransportSubsystem::UpdateEndCaps()
{
	for (const auto& Rail : Rails) { if (Rail.IsValid()) { Rail->UpdateEndCaps(GetNodeDegree(Rail->GetSaveData().Start) == 1, Rail->GetSaveData().bEndDisconnected || GetNodeDegree(Rail->GetSaveData().End) == 1); } }
}
