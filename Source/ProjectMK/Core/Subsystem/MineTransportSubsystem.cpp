#include "ProjectMK/Core/Subsystem/MineTransportSubsystem.h"

#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Transport/RailTrackActor.h"
#include "ProjectMK/Actor/Transport/MineCartActor.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"

void UMineTransportSubsystem::Tick(float DeltaSeconds)
{
	if (GetWorld()->IsGameWorld() == false || bClearing) { return; }
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
	ABlockBase* Ground = Level->GetBlockBelowWorldLocation(Player->GetActorLocation());
	if (Ground == nullptr) { return false; }
	const FVector2D Cell = UMKBlueprintFunctionLibrary::GetBlockPosition(Ground);
	const FIntPoint Start(FMath::RoundToInt(Cell.X), FMath::RoundToInt(Cell.Y));
	const float Facing = FMath::Abs(Player->GetCharacterDirection().X) > 0.1f ? Player->GetCharacterDirection().X : Player->GetHorizontalFacingSign();
	const int32 Direction = Facing < 0.f ? -1 : 1;
	const int32 Vertical = Player->GetCharacterDirection().Z > 0.5f ? -1 : (Player->GetCharacterDirection().Z < -0.5f ? 1 : 0);
	for (int32 Offset : { Vertical, Vertical == 0 ? -1 : 0, 1 })
	{
		if (TryPlaceRailSegment(Start, Start + FIntPoint(Direction, Offset))) { return true; }
	}
	return false;
}

bool UMineTransportSubsystem::TryPlaceRailSegment(const FIntPoint& Start, const FIntPoint& End, FGuid Id, bool bKeepEndDisconnected)
{
	if (FMath::Abs(Start.X - End.X) != 1 || FMath::Abs(Start.Y - End.Y) > 1 || GetNodeDegree(Start) >= 2 || GetNodeDegree(End) >= 2) { return false; }
	for (const auto& Existing : Rails) { if (Existing.IsValid() && ((Existing->GetSaveData().Start == Start && Existing->GetSaveData().End == End) || (Existing->GetSaveData().Start == End && Existing->GetSaveData().End == Start))) { return false; } }
	const bool bDisconnect = bKeepEndDisconnected || AreNodesConnected(Start, End);
	ULevelManagerSubsystem* Level = GetWorld()->GetSubsystem<ULevelManagerSubsystem>();
	for (const FIntPoint& Node : { Start, End })
	{
		ABlockBase* Floor = Level->GetBlockAtPosition(FVector2D(Node));
		ABlockBase* Above = Level->GetBlockAtPosition(FVector2D(Node.X, Node.Y - 1));
		if (Floor == nullptr || Floor->IsBreaking() || (Above != nullptr && Above->GetRecoveryItemKey() != TEXT("SupportBeam"))) { return false; }
	}
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
			const FVector Location = FMath::Lerp(Rail->GetStartWorld(), Rail->GetEndWorld(), Alpha) + FVector(0.f, 0.f, 5.5f);
			if (FMath::Abs(Location.X - Player->GetActorLocation().X) < 17.f || CanCartOccupy(nullptr, Location) == false) { continue; }
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
				if (ABlockBase* Block = Level->GetBlockAtPosition(FVector2D(Node.X, Node.Y - Height))) { if (Block->IsMineable()) { Block->SetItemDropEnabled(false); Block->Destroy(); } }
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
	for (const auto& Rail : Rails) { if (Rail.IsValid() && (Rail->GetSaveData().Start == Cell || Rail->GetSaveData().End == Cell)) { return true; } }
	return false;
}

bool UMineTransportSubsystem::CanCartOccupy(const AMineCartActor* Cart, const FVector& Location) const
{
	for (const auto& Other : Carts) { if (Other.IsValid() && Other.Get() != Cart && FVector::DistSquared(Other->GetActorLocation(), Location) < FMath::Square(23.f)) { return false; } }
	return true;
}

bool UMineTransportSubsystem::IsDirty() const { return bDirty; }
void UMineTransportSubsystem::ClearDirty() { bDirty = false; }
TArray<FRailSaveData> UMineTransportSubsystem::GetRailSaveData() const { TArray<FRailSaveData> Result; for (const auto& Rail : Rails) { if (Rail.IsValid()) { Result.Add(Rail->GetSaveData()); } } return Result; }
TArray<FMineCartSaveData> UMineTransportSubsystem::GetCartSaveData() const { TArray<FMineCartSaveData> Result; for (const auto& Cart : Carts) { if (Cart.IsValid()) { Result.Add(Cart->GetSaveData()); } } return Result; }
const TArray<TWeakObjectPtr<ARailTrackActor>>& UMineTransportSubsystem::GetRails() const { return Rails; }
const TArray<TWeakObjectPtr<AMineCartActor>>& UMineTransportSubsystem::GetCarts() const { return Carts; }

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
