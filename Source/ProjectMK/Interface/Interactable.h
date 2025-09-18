// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h" 
#include "Interactable.generated.h"

UINTERFACE(Blueprintable)
class PROJECTMK_API UInteractable : public UInterface
{
	GENERATED_BODY()
};

class IInteractable
{
    GENERATED_BODY()

public:
    virtual bool TryInteract(AActor* Interactor);
    virtual bool CanInteract(AActor* Interactor);

private:
    virtual bool Interact(AActor* Interactor) = 0;
};
