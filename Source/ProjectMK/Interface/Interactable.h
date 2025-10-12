// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"
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
    virtual bool CanInteract(AActor* Interactor);
    virtual bool TryInteract(AActor* Interactor);
    virtual void EndInteract() {}

protected:
    virtual const FGameplayTag GetInteractEventTag() = 0;
};
