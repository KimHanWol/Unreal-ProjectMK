#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "InventoryComponent.generated.h"

struct FShopRecipeDataTableRow;

UCLASS(BlueprintType)
class PROJECTMK_API UInventoryItemData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FName ItemUID;

	UPROPERTY(BlueprintReadWrite)
	int32 ItemCount;
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
	void SetItemCount(FName ItemUID, int32 ItemCount);
	bool AddItem(FName ItemUID, int32 ItemCount);
	bool CanGainItem(FName ItemUID, int32 ItemCount);
	void SetGainRadius(float NewRadius);
	bool CraftShopRecipe(const FShopRecipeDataTableRow& ShopRecipeData);
	bool CanCraftShopRecipe(const FShopRecipeDataTableRow& ShopRecipeData) const;

	TMap<FName, int32> GetInventoryItems() const { return InventoryItemMap; }
	const TArray<FName>& GetInventoryItemOrder() const { return InventoryItemOrder; }

private:
	void AddItemOrder(FName ItemUID);
	void RemoveItemOrder(FName ItemUID);
	void GainItem(FName ItemUID, int32 ItemCount);
	void SpendItem(FName ItemUID, int32 ItemCount);
	void OnInventoryUpdated();

	const class UAttributeSet_Character* GetCharacterAttributeSet() const;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
						 bool bFromSweep, const FHitResult& SweepResult);

public:
	DECLARE_MULTICAST_DELEGATE(FOnInventoryChanged);
	FOnInventoryChanged OnInventoryChangedDelegate;

private:
	UPROPERTY(Transient)
	TMap<FName, int32> InventoryItemMap;

	UPROPERTY(Transient)
	TArray<FName> InventoryItemOrder;

	UPROPERTY(EditDefaultsOnly)
	float ItemGainRange = 5.f;

	UPROPERTY(Transient)
	float ItemCollectRange = 0.f;

	UPROPERTY(EditDefaultsOnly)
	int32 MaxInventoryCount = 8;

	UPROPERTY(EditDefaultsOnly)
	float LootableDistance = 0.f;
};
