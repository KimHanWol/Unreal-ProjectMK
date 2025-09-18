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

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void CheckInteractableActor();

	UPROPERTY(EditAnywhere)
	float InteractDistance = 100.f;

	TWeakObjectPtr<AActor> InteractingActor;
};
