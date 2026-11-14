#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Actor/Shop/ShopActor.h"
#include "ProjectMK/Data/StorageChestData.h"
#include "StorageChestActor.generated.h"

class AMKCharacter;
class ABlockBase;
class AMineCartActor;

UCLASS()
class PROJECTMK_API AStorageChestActor : public AShopActor
{
	GENERATED_BODY()

public:
	virtual bool CanInteract(AActor* Interactor) override;
	virtual int32 GetInteractionPriority() const override;
	virtual FName GetRecoveryItemKey() const override;
	virtual FText GetRecoveryBlockedReason(AActor* Interactor) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool OnInteract(AActor* Interactor) override;
	virtual void OnRecovered() override;

public:
	void InitializeChest(FGuid Id, FName ItemKey, bool bBuried, bool bUsePhysics = true);
	void ApplySavedContents(const TArray<FInventorySlotData>& Contents);
	void RevealChest();
	bool TryBreakByExplosion();
	void NotifyContentsChanged();
	const TArray<FInventorySlotData>& GetItemSlots() const;
	FStorageChestSaveData GetSaveData() const;
	FGuid GetChestId() const;
	bool IsBuried() const;
	void SetPhysicsBlock(ABlockBase* Block);
	void SetMountedCart(AMineCartActor* Cart);
	static constexpr int32 SlotCount = 10;

private:
	void UpdateBuriedCover();
	UFUNCTION()
	void OnPhysicsBlockDestroyed(AActor* Actor);

public:
	DECLARE_MULTICAST_DELEGATE(FOnContentsChanged);
	FOnContentsChanged OnContentsChanged;

private:
	friend class UInventoryComponent;
	UPROPERTY(Transient)
	TArray<FInventorySlotData> ItemSlots;
	FGuid ChestId;
	FName ChestItemKey = TEXT("StorageChest");
	bool bIsBuried = false;
	bool bIsBreaking = false;
	FTimerHandle CoverTimer;
	TWeakObjectPtr<ABlockBase> PhysicsBlock;
	TWeakObjectPtr<AMineCartActor> MountedCart;
};
