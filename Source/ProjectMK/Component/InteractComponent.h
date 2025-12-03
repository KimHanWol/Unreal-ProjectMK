// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractComponent.generated.h"

UCLASS()
class PROJECTMK_API UInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractComponent();

	bool TryInteract();
	bool IsInteracting() const { return InteractedActor != nullptr; };

	void UpdateCharacterDirection(const FVector& NewDir);

private:
	void UpdateInteractPosition();

private:
	UPROPERTY(EditAnywhere)
	float InteractDistance = 20.f;

	UPROPERTY(EditAnywhere)
	float InteractPositionMoveDuration = 0.5f;

	UPROPERTY(Transient)
	float InteractingTime = 0.f;

	UPROPERTY(Transient)
	FVector InteractStartPoint = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector InteractDir = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bIsInteracting = false;

	TWeakObjectPtr<AActor> InteractedActor;

	FTimerHandle UpdateInteractPositionTimerHandle;
};
