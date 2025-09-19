// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectMK/Interface/Interactable.h"
#include "BlockBase.generated.h"

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
class PROJECTMK_API ABlockBase : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
    ABlockBase();

	void InitializeBlock(const FBlockData& InBlockData);

    //IInteractable
    virtual bool Interact(AActor* Caller) override;
    //~IInteractable

private:
    void OnPaperSpriteLoaded();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoxCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UPaperSpriteComponent> PaperSpriteComponent;

    FBlockData BlockData;
};
