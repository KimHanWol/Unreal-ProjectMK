#pragma once

#include "CoreMinimal.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Data/Struct/CharacterAnimationTextureSet.h"

#include "MKCharacterVisualComponent.generated.h"

class AMKCharacter;
class UAbilitySystemComponent;
class UCharacterAnimationDataAsset;
class UMaterialInterface;
class UMaterialInstanceDynamic;
struct FCharacterSpriteAnimationClip;
struct FGameplayTag;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTMK_API UMKCharacterVisualComponent : public UPaperSpriteComponent
{
	GENERATED_BODY()

public:
	UMKCharacterVisualComponent();

	void InitializeVisuals();
	void UpdateVisuals();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BindVisualDelegates();
	void UnbindVisualDelegates();

	void InitializeInvincibleMaterial();
	void CacheStateSpriteComponents();
	void CacheStateSpriteComponent(FName ComponentName, TObjectPtr<UPaperSpriteComponent>& OutComponent, FVector& OutLeftFacingRelativeLocation);

	void UpdateFacingDirectionCache();
	void UpdateCharacterAnimationVisual();
	void UpdateOverrideVisualFacingDirection();
	void EnsureCharacterVisualMaterialInstance();
	void UpdateDrillShakeVisuals();
	void UpdateStateSpriteVisuals();
	void HideAllStateSprites();
	void SetCharacterVisualOverrideEnabled(bool bEnabled);

	const FCharacterSpriteAnimationClip* GetAnimationClipForCurrentState(const UCharacterAnimationDataAsset* AnimationDataAsset) const;
	int32 CalculateAnimationFrameIndex(const FCharacterSpriteAnimationClip& AnimationClip) const;
	UPaperSpriteComponent* FindSpriteComponentByName(FName ComponentName) const;
	AMKCharacter* GetOwnerCharacter() const;
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	int32 GetStateSpriteSortPriority() const;

	void HandleInvincibleTagChanged(const FGameplayTag Tag, int32 NewCount);

private:
	UPROPERTY(EditDefaultsOnly, Category = "State Sprite")
	FName BalloonSpriteComponentName = TEXT("BalloonSprite");

	UPROPERTY(EditDefaultsOnly, Category = "State Sprite")
	FName DrillLeftSpriteComponentName = TEXT("DrillLeftSprite");

	UPROPERTY(EditDefaultsOnly, Category = "State Sprite")
	FName DrillRightSpriteComponentName = TEXT("DrillRightSprite");

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
	TObjectPtr<UMaterialInstanceDynamic> InvincibleMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CharacterVisualMaterialInstance;

	UPROPERTY(Transient)
	TWeakObjectPtr<UMaterialInterface> CharacterVisualMaterialSource;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> BalloonStateSpriteComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> DrillLeftStateSpriteComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> DrillRightStateSpriteComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> DrillDownStateSpriteComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> DrillUpStateSpriteComponent;

	FVector BalloonLeftFacingRelativeLocation = FVector::ZeroVector;
	FVector DrillLeftLeftFacingRelativeLocation = FVector::ZeroVector;
	FVector DrillRightLeftFacingRelativeLocation = FVector::ZeroVector;
	FVector DrillDownLeftFacingRelativeLocation = FVector::ZeroVector;
	FVector DrillUpLeftFacingRelativeLocation = FVector::ZeroVector;
	FRotator DrillLeftLeftFacingRelativeRotation = FRotator::ZeroRotator;
	FRotator DrillRightLeftFacingRelativeRotation = FRotator::ZeroRotator;
	float CurrentInvincibleDarkenValue = 0.f;
	float CurrentOverrideVisualScale = 1.f;
	FVector BaseCharacterSpriteRelativeLocation = FVector::ZeroVector;
	FVector CachedDrillShakeOffset = FVector::ZeroVector;
	bool bCharacterVisualOverrideEnabled = false;
	bool bFacingRight = false;
	ECharacterAnimationType CachedAnimationType = ECharacterAnimationType::Idle_Down;
	bool bVisualsInitialized = false;
};
