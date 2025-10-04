// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectMK/Interface/Interactable.h"
#include "BlockBase.generated.h"

class UAbilitySystemComponent;
class UAttributeSet_Block;
class UBoxComponent;
class UPaperSpriteComponent;

USTRUCT(BlueprintType)
struct FBlockData
{
    GENERATED_BODY()

    UPROPERTY()
    int32 TileSetIndex;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector WorldLocation;

    UPROPERTY()
    FIntPoint TileSize;
};

UCLASS()
class PROJECTMK_API ABlockBase : public AActor, public IInteractable, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:	
    ABlockBase();

	void InitializeBlock(FBlockData InBlockData);

    //IInteractable
    virtual bool Interact(AActor* Caller) override;
    //~IInteractable

    //IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    //~IAbilitySystemInterface

private:
    void OnPaperSpriteLoaded();

    void InitializeBlockAttribute();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoxCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UPaperSpriteComponent> PaperSpriteComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    UAttributeSet_Block* AttributeSet_Block;

    FBlockData BlockData;
};
