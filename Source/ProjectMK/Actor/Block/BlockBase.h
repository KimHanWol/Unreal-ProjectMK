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
    virtual const FGameplayTag GetInteractEventTag() override;
    //~IInteractable

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

    FBlockData BlockData;
};
