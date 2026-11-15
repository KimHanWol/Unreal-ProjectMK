#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectMK/Data/MineTownData.h"
#include "MineTransportSubsystem.generated.h"

class AMKCharacter;
class ARailTrackActor;
class AMineCartActor;

UCLASS()
class PROJECTMK_API UMineTransportSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override;
	void ClearNetwork();
	bool TryPlaceRail(AMKCharacter* Player);
	bool TryPlaceRailSegment(const FIntPoint& Start, const FIntPoint& End, FGuid Id = FGuid(), bool bKeepEndDisconnected = false);
	bool TryPlaceCart(AMKCharacter* Player, bool bAutomatic);
	bool TryMountChest(AMKCharacter* Player, FName ChestItemKey);
	void RestoreNetwork(const TArray<FRailSaveData>& Rails, const TArray<FMineCartSaveData>& Carts);
	void NotifyRailRemoved(ARailTrackActor* Rail);
	void NotifyCartRemoved(AMineCartActor* Cart);
	ARailTrackActor* FindNextRail(const ARailTrackActor* Current, FIntPoint Node) const;
	AMineCartActor* FindNearbyCart(const FVector& Location) const;
	AMineCartActor* FindCart(FGuid Id) const;
	bool HasCartOnLine(const ARailTrackActor* Rail) const;
	bool HasRailAtSupport(FIntPoint Cell) const;
	bool CanCartOccupy(const AMineCartActor* Cart, const FVector& Location, const FQuat& Rotation = FQuat::Identity) const;
	bool IsDirty() const;
	void ClearDirty();
	TArray<FRailSaveData> GetRailSaveData() const;
	TArray<FMineCartSaveData> GetCartSaveData() const;
	const TArray<TWeakObjectPtr<ARailTrackActor>>& GetRails() const;
	const TArray<TWeakObjectPtr<AMineCartActor>>& GetCarts() const;
private:
	bool IsNodeSupported(FIntPoint Node) const;
	bool IsRailCovered(const ARailTrackActor* Rail) const;
	void UpdateUnsupportedRails();
	bool AreNodesConnected(FIntPoint A, FIntPoint B) const;
	int32 GetNodeDegree(FIntPoint Node) const;
	void UpdateEndCaps();
	TArray<TWeakObjectPtr<ARailTrackActor>> Rails;
	TArray<TWeakObjectPtr<AMineCartActor>> Carts;
	float SupportCheckTime = 0.f;
	bool bClearing = false;
	bool bDirty = false;
};
