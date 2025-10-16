// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

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
class PROJECTMK_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UInventoryComponent();

	int32 GetItemCount(FName ItemUID);
	void SetItemCount(FName ItemUID, int32 ItemCount);

	UFUNCTION(BlueprintCallable)
	TMap<FName, int32> GetInventoryItems() { return InventoryItemMap; }

private:
	void OnGainItem(FName ItemUID, int32 ItemCount);
	void OnSpendItem(FName ItemUID, int32 ItemCount);

	int32 GetInventoryCount();
	void OnInventoryUpdated();

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged OnInventoryChangedDelegate;

private:
	UPROPERTY(Transient)
	TMap<FName, int32> InventoryItemMap;

	UPROPERTY(EditAnywhere)
	int32 MaxInventoryCount = 10;
};
