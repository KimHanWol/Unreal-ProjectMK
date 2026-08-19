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
class UDataManager;
class UMaterialInterface;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSprite;
class UPaperSpriteComponent;
class IMinable;
struct FBlockDataTableRow;

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

struct FSelectedBlockItemData
{
	FName ItemKey = NAME_None;
	int32 ItemCount = 0;
	bool bShouldSplitDrop = false;
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
	bool TryAddSpawnItem(FName ItemKey, int32 ItemCount);
	void SetLastDamageInstigatorASC(UAbilitySystemComponent* InInstigatorASC);
	UAbilitySystemComponent* GetLastDamageInstigatorASC() const;

	void StartMineBlock(IMinable* Miner);
	void EndMineBlock();
	void SetMineableState(bool bInIsMineableState);

	bool IsMineable();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_EnableDebugState(int32 Count);

private:
	virtual void OnPreDestroy();
	void PlayDestroyFlipbook();
	void DestroyBlockActor();
	void ApplySpriteToComponent(UPaperSpriteComponent* SpriteComponent, UPaperSprite* Sprite, const FIntPoint& TileSize, float ScaleMultiplier = 1.f);
	void OnPaperSpriteLoaded();

	void InitializeBlockAttribute();
	void InitializeDropSelection(const FBlockDataTableRow& BlockDataTableRow, UDataManager* DataManager);
	void UpdateDamageFlipbook();

	void SpawnItems();

	UFUNCTION()
	void OnDestroyFlipbookFinished();

	void OnDurationChanged(const FOnAttributeChangeData& Data);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoxCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> PaperSpriteComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> ItemSpriteComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> DestroyFlipbookComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> DamageFlipbookComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	FBlockTileData BlockTileData;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DamageFlipbookMaterial;

	UPROPERTY(Transient)
	bool bIsMining = false;

	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> LastDamageInstigatorASC = nullptr;

	UPROPERTY(Transient)
	bool bIsMineableState = true;

	TArray<FSelectedBlockItemData> SelectedSpawnItemDataList;

	UPROPERTY(Transient)
	TSoftObjectPtr<UPaperSprite> SelectedBaseTileSprite;

	UPROPERTY(Transient)
	TSoftObjectPtr<UPaperSprite> SelectedItemOverlaySprite;

	UPROPERTY(Transient)
	bool bDropSelectionInitialized = false;

	UPROPERTY(Transient)
	bool bBlockAttributesInitialized = false;

	UPROPERTY(Transient)
	bool bIsDestroying = false;

	UPROPERTY(Transient)
	float MaxDurability = 0.f;

	FTimerHandle BreakingTimerHandle;
};
