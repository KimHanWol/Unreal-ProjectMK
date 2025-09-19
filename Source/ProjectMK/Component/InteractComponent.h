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

	void UpdateCharacterDirection(const FVector& NewDir);

private:

	UPROPERTY(EditAnywhere)
	float InteractDistance = 20.f;

	TWeakObjectPtr<AActor> InteractingActor;
};
