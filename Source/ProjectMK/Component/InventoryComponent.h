#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameplayEffectTypes.h"
#include "InventoryComponent.generated.h"

enum class EEuipmentType : uint8;
struct FShopRecipeDataTableRow;
class UGameplayEffect;
struct FEquipmentEffectEntry;

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

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UInventoryComponent();

	int32 GetItemCount(FName ItemUID);
	void SetItemCount(FName ItemUID, int32 ItemCount);
	bool AddItem(FName ItemUID, int32 ItemCount);
	bool IsItemEquipped(FName ItemUID) const;
	FName GetEquippedItem(EEuipmentType EquipmentType) const;

	TMap<FName, int32> GetInventoryItems() const { return InventoryItemMap; }
	const TArray<FName>& GetInventoryItemOrder() const { return InventoryItemOrder; }

	bool CanGainItem(FName ItemUID, int32 ItemCount);
	void SetGainRadius(float NewRadius);

	bool EquipItem(FName ItemUID);
	bool UnEquipItem(FName ItemUID);

	bool CraftShopRecipe(const FShopRecipeDataTableRow& ShopRecipeData);
	bool CanCraftShopRecipe(const FShopRecipeDataTableRow& ShopRecipeData) const;

private:
	void AddItemOrder(FName ItemUID);
	void RemoveItemOrder(FName ItemUID);

	bool ApplyEquipmentEffects(FName EquipmentKey, const TArray<FEquipmentEffectEntry>& EffectClasses);
	void RemoveEquipmentEffects(FName EquipmentKey);
	const struct FEquipmentItemDataTableRow* GetEquipmentItemData(FName EquipmentKey) const;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
						 bool bFromSweep, const FHitResult& SweepResult);

	void GainItem(FName ItemUID, int32 ItemCount);
	void SpendItem(FName ItemUID, int32 ItemCount);

	void OnInventoryUpdated();

public:
	DECLARE_MULTICAST_DELEGATE(FOnInventoryChanged);
	FOnInventoryChanged OnInventoryChangedDelegate;

private:
	UPROPERTY(Transient)
	TMap<FName, int32> InventoryItemMap; //ItemKey, ItemCount

	UPROPERTY(Transient)
	TArray<FName> InventoryItemOrder; // insertion order for stable UI slots

	UPROPERTY(Transient)
	TMap<EEuipmentType, FName> EquipmentItemMap; // EEuipmentType, EquipmentKey

	TMap<FName, TArray<FActiveGameplayEffectHandle>> ActivatedEquipementEffects;

	UPROPERTY(EditAnywhere)
	float ItemGainRange = 5.f;

	UPROPERTY(Transient)
	float ItemCollectRange = 0.f;

	UPROPERTY(EditAnywhere)
	int32 MaxInventoryCount = 30;

	UPROPERTY(EditAnywhere)
	float LootableDistance = 0.f;
};
