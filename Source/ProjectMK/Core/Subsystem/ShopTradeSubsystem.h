#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ShopTradeSubsystem.generated.h"

class AMKCharacter;

USTRUCT()
struct FShopBuybackEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;
	UPROPERTY()
	FName ItemKey;
	UPROPERTY()
	int32 Count = 0;
	UPROPERTY()
	int32 UnitPrice = 0;
	UPROPERTY()
	double ExpiresAt = 0.0;
};

UCLASS()
class PROJECTMK_API UShopTradeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& World) override;
	virtual void Deinitialize() override;
	bool TrySell(AMKCharacter* Player, EItemContainer Container, int32 SlotIndex, int32 Count);
	bool TryBuyback(AMKCharacter* Player, FGuid Id, int32 Count);
	const TArray<FShopBuybackEntry>& GetBuybackEntries() const;
	const FShopBuybackEntry* FindBuyback(FGuid Id) const;
	int32 GetMaximumBuybackCount(AMKCharacter* Player, FGuid Id) const;
	DECLARE_MULTICAST_DELEGATE(FOnStockChanged);
	FOnStockChanged OnStockChanged;

private:
	void RemoveExpiredEntries();
	UPROPERTY(Transient)
	TArray<FShopBuybackEntry> Entries;
	FTimerHandle ExpirationTimer;
	bool bTransactionActive = false;
};
