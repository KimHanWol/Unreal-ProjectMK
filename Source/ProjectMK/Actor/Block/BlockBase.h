#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "ProjectMK/Interface/Damageable.h"

#include "BlockBase.generated.h"

class UAbilitySystemComponent;
class UAttributeSet_Block;
class UBoxComponent;
class UPaperSprite;
class UPaperSpriteComponent;
class IMinable;

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

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBlockDestroyed, ABlockBase*);
	FOnBlockDestroyed OnBlockDestroyedDelegate;
};

UCLASS()
class PROJECTMK_API ABlockBase : public AActor, public IDamageable, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABlockBase();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void BindEvents();
	void UnbindEvents();

	virtual UAbilitySystemComponent* GetOwnerASC() override;
	virtual bool CheckIsDestroyed() override;
	virtual void OnDestroyed() override;

public:
	void InitializeBlock(FBlockTileData InBlockData);

	void StartMineBlock(IMinable* Miner);
	void EndMineBlock();
	void SetMineableState(bool bInIsMineableState);

	bool IsMineable();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_EnableDebugState(int32 Count);

private:
	virtual void OnPreDestroy();

	void ApplySpriteToComponent(UPaperSpriteComponent* SpriteComponent, UPaperSprite* Sprite, const FIntPoint& TileSize, float ScaleMultiplier = 1.f);
	void OnPaperSpriteLoaded();

	void InitializeBlockAttribute();

	void SpawnItem();

	void OnDurationChanged(const FOnAttributeChangeData& Data);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoxCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> PaperSpriteComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> ItemSpriteComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	FBlockTileData BlockTileData;

private:
	UPROPERTY(Transient)
	bool bIsMining = false;

	UPROPERTY(Transient)
	bool bIsMineableState = true;

	UPROPERTY(Transient)
	FName SpawnableItemKey;

	UPROPERTY(Transient)
	TSoftObjectPtr<UPaperSprite> SelectedBaseTileSprite;

	UPROPERTY(Transient)
	TSoftObjectPtr<UPaperSprite> SelectedItemOverlaySprite;

	UPROPERTY(Transient)
	bool bVisualSelectionInitialized = false;

	UPROPERTY(Transient)
	bool bBlockAttributesInitialized = false;

	FTimerHandle BreakingTimerHandle;
};
