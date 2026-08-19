#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "InventoryComponent.generated.h"

struct FShopRecipeDataTableRow;
struct FItemDataTableRow;
class AItemBase;

USTRUCT(BlueprintType)
struct FInventorySlotData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FName ItemUID = NAME_None;

	UPROPERTY(BlueprintReadWrite)
	int32 ItemCount = 0;

	bool IsEmpty() const
	{
		return ItemUID.IsNone() || ItemCount <= 0;
	}

	void Clear()
	{
		ItemUID = NAME_None;
		ItemCount = 0;
	}
};

UCLASS(BlueprintType)
class PROJECTMK_API UInventoryComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	int32 GetItemCount(FName ItemUID);
	int32 GetMaxInventoryCount() const;
	float GetInventoryOccupancyRatio() const;
	float GetItemGainRange() const;
	void SetItemCount(FName ItemUID, int32 ItemCount);
	bool AddItem(FName ItemUID, int32 ItemCount);
	void RemoveAllOreItems();
	bool CanGainItem(FName ItemUID, int32 ItemCount);
	bool TryCollectWorldItem(AItemBase* ItemActor);
	bool TryMoveItemSlot(int32 SourceSlotIndex, int32 TargetSlotIndex);
	void SetGainRadius(float NewRadius);
	bool CraftShopRecipe(const FShopRecipeDataTableRow& ShopRecipeData);
	bool CanCraftShopRecipe(const FShopRecipeDataTableRow& ShopRecipeData) const;

	TMap<FName, int32> GetInventoryItems() const { return InventoryItemMap; }
	const TArray<FInventorySlotData>& GetInventorySlotDataList();

private:
	void GainItem(FName ItemUID, int32 ItemCount);
	void SpendItem(FName ItemUID, int32 ItemCount);
	void OnInventoryUpdated();
	void UpdateInventorySlotDataList();
	void UpdateInventoryItemMap();
	void NormalizeInventorySlotDataList();

	const class UAttributeSet_Character* GetCharacterAttributeSet() const;
	int32 GetItemMaxStackCount(FName ItemUID) const;
	int32 FindEmptySlotIndex() const;
	bool IsValidSlotIndex(int32 SlotIndex) const;
	int32 FindStackableSlotIndex(FName ItemUID) const;
	int32 CalculateAvailableItemCapacity(FName ItemUID) const;
	bool CanCollectWorldItem(AItemBase* ItemActor) const;
	bool CanGrantNonInventoryItem(FName ItemKey, int32 ItemCount) const;
	bool TryGrantNonInventoryItem(FName ItemKey, int32 ItemCount);

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
						 bool bFromSweep, const FHitResult& SweepResult);

public:
	DECLARE_MULTICAST_DELEGATE(FOnInventoryChanged);
	FOnInventoryChanged OnInventoryChangedDelegate;

private:
	UPROPERTY(Transient)
	TArray<FInventorySlotData> InventorySlotDataList;

	UPROPERTY(Transient)
	TMap<FName, int32> InventoryItemMap;

	UPROPERTY(EditDefaultsOnly)
	float ItemGainRange = 5.f;

	UPROPERTY(Transient)
	float ItemCollectRange = 0.f;

	UPROPERTY(EditDefaultsOnly)
	int32 MaxInventoryCount = 8;

	UPROPERTY(EditDefaultsOnly)
	float LootableDistance = 0.f;
};
