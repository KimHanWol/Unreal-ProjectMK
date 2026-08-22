#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectMK/Interface/Interactable.h"

#include "ItemBase.generated.h"

class AMKCharacter;
class USphereComponent;
class UPaperSpriteComponent;

UCLASS()
class PROJECTMK_API AItemBase : public AActor
{
	GENERATED_BODY()

public:
	AItemBase();

	bool IsInitialized() { return bIsInitialized; }

	void InitializeItemBase(FName InItemKey, int32 InItemCount = 1);
	FName GetItemKey() const { return ItemKey; }
	int32 GetItemCount() const { return ItemCount; }
	float GetCollisionRadius() const;
	bool IsOccupied();

	void TryLoot(TWeakObjectPtr<AMKCharacter> InLooter);
	void OnLootFinished();
	void SetVisualLayer(float InVisualYOffset);

protected:
	virtual void Tick(float DeltaTime) override;

private:
	void ClearLooting();
	void UpdatePosition(float DeltaTime);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> SphereCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> PaperSpriteComponent;

private:
	TWeakObjectPtr<AMKCharacter> Looter = nullptr;

	UPROPERTY(Transient)
	bool bIsInitialized = false;

	UPROPERTY(Transient)
	bool bIsOccupied = false;

	UPROPERTY(Transient)
	FName ItemKey;

	UPROPERTY(Transient)
	int32 ItemCount = 1;

	UPROPERTY(Transient)
	FVector BaseSpriteRelativeLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	float VisualYOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Floating")
	float FloatAmplitude = 5.0f; // 위아래 움직임의 크기

	UPROPERTY(EditDefaultsOnly, Category = "Floating")
	float FloatSpeed = 3.0f; // 움직임 속도

	UPROPERTY(Transient)
	float FloatPhase = 0.f;

	UPROPERTY(Transient)
	float FallingVelocity = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Falling")
	float MaxFallingSpeed = 400.f;

	FTimerHandle LootingTimerHandle;

	UPROPERTY(Transient)
	float CurrentLootingSpeed = 30.f;

	UPROPERTY(Transient)
	FVector LootLateralDirection = FVector::ZeroVector;

	UPROPERTY(Transient)
	float LootLateralOffsetScale = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Looting")
	float InitialLootingSpeed = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Looting")
	float LootingAccelerationRate = 150.f;
};
