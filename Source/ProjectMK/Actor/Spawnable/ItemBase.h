// Fill out your copyright notice in the Description page of Project Settings.

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

	void InitializeItemBase(FName InItemKey);
	FName GetItemKey() const { return ItemKey; }
	bool IsOccupied();

	void TryLoot(TWeakObjectPtr<AMKCharacter> InLooter);
	void OnLootFinished();

protected:
	virtual void Tick(float DeltaTime) override;

private:
	void UpdatePosition();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USphereComponent* SphereCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UPaperSpriteComponent* PaperSpriteComponent;

private:
	TWeakObjectPtr<AMKCharacter> Looter = nullptr;

	UPROPERTY(Transient)
	bool bIsInitialized = false;

	UPROPERTY(Transient)
	bool bIsOccupied = false;

	UPROPERTY(Transient)
	FName ItemKey;

	UPROPERTY(EditAnywhere, Category = "Floating")
	float FloatAmplitude = 5.0f; // 위아래 움직임의 크기

	UPROPERTY(EditAnywhere, Category = "Floating")
	float FloatSpeed = 3.0f; // 움직임 속도

	FTimerHandle LootingTimerHandle;

	UPROPERTY(Transient)
	float CurrentLootingSpeed = 30.f;

	UPROPERTY(EditAnywhere, Category = "Looting")
	float LootingAccelerationRate = 150.f;
};