// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "InventoryComponent.generated.h"

enum class EEuipmentType : uint8;

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

	UFUNCTION(BlueprintCallable)
	TMap<FName, int32> GetInventoryItems() { return InventoryItemMap; }

	bool CanGainItem(FName ItemUID, int32 ItemCount);
	void SetGainRadius(float NewRadius);

	bool EquipItem(FName ItemUID);
	bool UnEquipItem(FName ItemUID);

	bool CraftEquipmentItem(FName EquipmentItemKey);
	bool IsCraftable(FName EquipmentItemKey);
private:

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
						 bool bFromSweep, const FHitResult& SweepResult);

	void GainItem(FName ItemUID, int32 ItemCount);
	void SpendItem(FName ItemUID, int32 ItemCount);

	int32 GetInventoryCount();
	void OnInventoryUpdated();

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged OnInventoryChangedDelegate;

private:
	UPROPERTY(Transient)
	TMap<FName, int32> InventoryItemMap; //ItemKey, ItemCount

	UPROPERTY(Transient)
	TMap<EEuipmentType, FName> EquipmentItemMap; // EEuipmentType, EquipmentKey

	TMap<FName, TArray<FActiveGameplayEffectHandle>> ActivatedEquipementEffects;

	UPROPERTY(EditAnywhere) 
	float ItemGainRange = 5.f;

	UPROPERTY(Transient)
	float ItemCollectRange = 0.f;

	UPROPERTY(EditAnywhere)
	int32 MaxInventoryCount = 10;

	UPROPERTY(EditAnywhere)
	float LootableDistance = 0.f;
};
