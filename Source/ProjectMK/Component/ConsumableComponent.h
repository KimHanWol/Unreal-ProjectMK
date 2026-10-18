#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"

#include "ConsumableComponent.generated.h"

class AMKCharacter;
class UInventoryComponent;
struct FItemDataTableRow;

/** 퀵슬롯 숫자 키로 쓰는 소모품의 효과를 적용한다 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTMK_API UConsumableComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 인벤토리 슬롯(퀵슬롯 숫자 키)에 있는 소모품을 사용한다. 실패하면 소모하지 않는다 */
	UFUNCTION(BlueprintCallable, Category = "Consumable")
	bool TryUseInventorySlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Consumable")
	bool TryUseItem(FName ItemUID);

	/** 사망 시 보험 증서가 있으면 1개 소모하고 true 를 돌려준다 */
	bool TryConsumeInsurance();
	void ClearRecall();

private:
	bool TryApplyConsumableEffect(FName ItemUID, const FItemDataTableRow& ItemDataTableRow);
	bool TryRestoreOxygen(float RestoreRatio);
	bool TryRestoreHealth(float RestoreAmount);
	bool TryStartRecall(FName ItemUID, float RecallDuration);
	bool TryPlaceBlock(FName BlockRowName);
	bool TryApplyCushion();
	bool TryPlaceBlastCharge(const FItemDataTableRow& ItemDataTableRow);
	void UpdateRecall();
	void ApplyRecallComplete();

public:
	UFUNCTION(BlueprintPure, Category = "Consumable")
	bool IsRecalling() const;

	UFUNCTION(BlueprintPure, Category = "Consumable")
	float GetRecallRemainingTime() const;

private:
	AMKCharacter* GetOwnerCharacter() const;
	UInventoryComponent* GetInventoryComponent() const;
	bool IsOwnerOnGround() const;

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnConsumableUsed, FName);
	FOnConsumableUsed OnConsumableUsedDelegate;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRecallStateChanged, bool);
	FOnRecallStateChanged OnRecallStateChangedDelegate;

private:
	/** 귀환 대기 중 이만큼 움직이면 귀환이 취소된다 */
	UPROPERTY(EditDefaultsOnly, Category = "Consumable", meta = (ClampMin = "0.0"))
	float RecallCancelMoveDistance = 4.f;

	/** 귀환 대기 중 이동 여부를 확인하는 간격 */
	UPROPERTY(EditDefaultsOnly, Category = "Consumable", meta = (ClampMin = "0.01"))
	float RecallCheckInterval = 0.1f;

	FTimerHandle RecallTimerHandle;
	FName RecallItemUID = NAME_None;
	FVector RecallStartLocation = FVector::ZeroVector;
	float RecallRemainingTime = 0.f;
	bool bIsRecalling = false;
};
