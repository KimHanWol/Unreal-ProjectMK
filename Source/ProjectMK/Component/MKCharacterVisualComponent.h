#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"

#include "MKCharacterVisualComponent.generated.h"

class AMKCharacter;
class UAbilitySystemComponent;
class UInventoryComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMKRuntimePaperSprite;
class UPaperSprite;
class UPaperSpriteComponent;
class UTexture2D;
struct FGameplayTag;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTMK_API UMKCharacterVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMKCharacterVisualComponent();

	void InitializeVisuals();
	void UpdateVisuals();
	void HandleInventoryChanged();
	void HandleInvincibleTagChanged(const FGameplayTag Tag, int32 NewCount);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BindVisualDelegates();
	void UnbindVisualDelegates();
	void InitializeInvincibleMaterial();
	void InitializeEquipmentOverlayComponents();
	void CacheStateSpriteComponents();
	void CacheStateSpriteComponent(FName ComponentName, TObjectPtr<UPaperSpriteComponent>& OutComponent, FVector& OutLeftFacingRelativeLocation);

	void UpdateFacingDirectionCache();
	void UpdateCharacterAnimationVisual();
	void SetCharacterVisualOverrideEnabled(bool bEnabled);
	void UpdateOverrideVisualFacingDirection();
	void EnsureCharacterVisualMaterialInstance();

	void RefreshEquippedOverlayItems();
	void UpdateEquipmentOverlays();
	void UpdateEquipmentOverlayZOrders();
	void UpdateDrillShakeVisuals();

	void UpdateStateSpriteVisuals();
	void UpdateStateSpriteLocations();
	void HideAllStateSprites();
	void ApplyStateSpriteDisplay(UPaperSpriteComponent* TargetComponent, const UPaperSprite* TargetSprite);

	UPaperSpriteComponent* FindSpriteComponentByName(FName ComponentName) const;
	AMKCharacter* GetOwnerCharacter() const;
	UInventoryComponent* GetInventoryComponent() const;
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	int32 ResolveStateSpriteSortPriority() const;

	const FEquipmentItemDataTableRow* GetEquipmentItemData(FName EquipmentKey) const;
	const UPaperSprite* ResolveEquipmentOverlaySprite(const FEquipmentItemDataTableRow& EquipmentData, ECharacterAnimationType AnimationType, int32 AnimationFrameIndex);
	const UPaperSprite* ResolveEquipmentStateSprite(const FEquipmentItemDataTableRow& EquipmentData);
	const UPaperSprite* ResolveAnimationAtlasSprite(UTexture2D* AtlasTexture, int32 AnimationFrameIndex, float PixelsPerUnrealUnit);
	UMKRuntimePaperSprite* GetOrCreateRuntimeAtlasSprite(UTexture2D* AtlasTexture, int32 AtlasCellIndex, float PixelsPerUnrealUnit);
	FName MakeRuntimeAtlasSpriteCacheKey(const UTexture2D* AtlasTexture, int32 AtlasCellIndex, float PixelsPerUnrealUnit) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "State Sprite")
	FName BalloonSpriteComponentName = TEXT("BalloonSprite");

	UPROPERTY(EditDefaultsOnly, Category = "State Sprite")
	FName DrillSideSpriteComponentName = TEXT("DrillSideSprite");

	UPROPERTY(EditDefaultsOnly, Category = "State Sprite")
	FName DrillDownSpriteComponentName = TEXT("DrillDownSprite");

	UPROPERTY(EditDefaultsOnly, Category = "State Sprite")
	FName DrillUpSpriteComponentName = TEXT("DrillUpSprite");

	UPROPERTY(EditDefaultsOnly, Category = "Drill Shake", meta = (ClampMin = "0.0"))
	float DrillShakeHorizontalAmplitude = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Drill Shake", meta = (ClampMin = "0.0"))
	float DrillShakeVerticalAmplitude = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category = "Drill Shake", meta = (ClampMin = "0.0"))
	float DrillShakeOscillationSpeed = 40.f;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> CharacterVisualComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> InvincibleMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CharacterVisualMaterialInstance;

	UPROPERTY(Transient)
	TWeakObjectPtr<UMaterialInterface> CharacterVisualMaterialSource;

	UPROPERTY(Transient)
	TMap<EEuipmentType, TObjectPtr<UPaperSpriteComponent>> EquipmentOverlayComponents;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMKRuntimePaperSprite>> RuntimeAtlasSpriteCache;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> BalloonStateSpriteComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> DrillSideStateSpriteComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> DrillDownStateSpriteComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> DrillUpStateSpriteComponent;

	TMap<EEuipmentType, FName> EquippedOverlayItemKeys;
	FVector BalloonLeftFacingRelativeLocation = FVector::ZeroVector;
	FVector DrillSideLeftFacingRelativeLocation = FVector::ZeroVector;
	FVector DrillDownLeftFacingRelativeLocation = FVector::ZeroVector;
	FVector DrillUpLeftFacingRelativeLocation = FVector::ZeroVector;
	FRotator DrillSideLeftFacingRelativeRotation = FRotator::ZeroRotator;
	float CurrentInvincibleDarkenValue = 0.f;
	float CurrentOverrideVisualScale = 1.f;
	FVector BaseCharacterSpriteRelativeLocation = FVector::ZeroVector;
	FVector CachedDrillShakeOffset = FVector::ZeroVector;
	bool bCharacterVisualOverrideEnabled = false;
	bool bFacingRight = false;
	ECharacterAnimationType CachedAnimationType = ECharacterAnimationType::Idle;
	int32 CachedAnimationFrameIndex = 0;
	bool bHasCachedPlaybackData = false;
	bool bVisualsInitialized = false;
};
