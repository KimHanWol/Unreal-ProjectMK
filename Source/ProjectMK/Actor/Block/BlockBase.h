// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "ProjectMK/Interface/Interactable.h"

#include "BlockBase.generated.h"

class UAbilitySystemComponent;
class UAttributeSet_Block;
class UBoxComponent;
class UGameplayAbility;
class UPaperSpriteComponent;

enum class EGameplayAbilityType : uint8;

USTRUCT(BlueprintType)
struct FBlockTileData
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

	void InitializeBlock(FBlockTileData InBlockData);

    //IInteractable
    virtual const FGameplayTag GetInteractEventTag() override;
    virtual bool TryInteract(AActor* Interactor) override;
    virtual void EndInteract() override;
    //~IInteractable

    void TryInteract_Internal(AActor* Interactor);

    //IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    //~IAbilitySystemInterface

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    void BindEvents();
    void UnbindEvents();

private:
    void OnPaperSpriteLoaded();
    void InitializeBlockAttribute(); 

    void OnDurationChanged(const FOnAttributeChangeData& Data);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoxCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UPaperSpriteComponent> PaperSpriteComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mining")
    float MiningDuration = 0.5f;

    FBlockTileData BlockTileData;

private:
    UPROPERTY(Transient)
    bool bIsInteracting = false;

    FTimerHandle InteractingTimerHandle;
};
