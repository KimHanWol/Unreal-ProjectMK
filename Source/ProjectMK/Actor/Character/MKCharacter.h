#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "PaperCharacter.h"
#include "GameplayEffectTypes.h"
#include "ProjectMK/Data/DataTable/CharacterDataTableRow.h"
#include "ProjectMK/Data/Struct/CharacterAnimationTextureSet.h"
#include "ProjectMK/Interface/Damageable.h"
#include "MKCharacter.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UInventoryComponent;
class UAttributeSet_Character;
class UConsumableComponent;
class UCraftingComponent;
class UMagnetRangeComponent;
class UMKCharacterVisualComponent;
class UWidgetComponent;
class UPaperSprite;
class USkillComponent;
class UTexture2D;
class UGameSettingDataAsset;
struct FCharacterDataTableRow;
class IInteractable;
class ABlockBase;

UCLASS()
class PROJECTMK_API AMKCharacter : public APaperCharacter, public IAbilitySystemInterface, public IDamageable
{
	GENERATED_BODY()

public:
	AMKCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void BindEvents();
	virtual void UnbindEvents();

	virtual UAbilitySystemComponent* GetOwnerASC() override;
	virtual bool CheckIsDestroyed() override;
	virtual void OnDestroyed() override;

public:
	void RespawnAtSpawnLocation();
	void ApplyKnockback(float HorizontalVelocity, float UpwardVelocity, float RecoveryTime);
	bool TrySaveGame();
	void SetMenuInputBlocked(bool bBlocked);
	void SetDrillingVector(const FVector& InDrillingVector);
	void AdjustDrillReach(int32 Delta);
	int32 GetMaximumDrillCells() const;
	int32 GetSelectedDrillCells() const;

	FVector GetCharacterDirection() const;
	float GetHorizontalFacingSign() const;
	FVector GetDrillingVector() const;
	bool IsKnockedBack() const;
	bool IsOnGround() const;
	USkillComponent* GetSkillComponent() const;
	UConsumableComponent* GetConsumableComponent() const;
	UCraftingComponent* GetCraftingComponent() const;
	const UPaperSprite* GetPortraitSprite() const;

private:
	void GiveAbilities();
	void InitializeCharacterAttributes();
	void ApplyInitialEffects();
	void ApplyDeathPenalty();
	float CalculateCurrentMoveSpeed() const;

	void TryDrill();
	IInteractable* FindNearestInteractable(bool bRecoveryOnly = false);
	bool TryInteractNearby();
	void UpdateInteractionPrompt();
	void ClearInteractionHold();

	void UpdateHorizontalMovement();
	void UpdateGroundTraction();
	void UpdateSupportPlatforms();
	void UpdateFlyingVerticalVelocity() const;

	void Apply2DCameraOverrides();

	void ApplyTextureRenderingOverrides(UTexture2D* Texture) const;
	void ApplySpriteRenderingOverrides(const UPaperSprite* PaperSprite) const;

	void ApplyDamageInvincibility();
	void ClearKnockback();

	void ApplyOxygenDrainEffect(float OxygenDrainPerSecond);
	void ClearOxygenDrainEffect();
	void RestoreOxygenToMax();
	void UpdateOxygen();

	void ApplySaveGame();

	TSubclassOf<UGameplayAbility> GetPrimaryDrillAbilityClass() const;

	int32 GetCurrentBlockDepth() const;
	const UGameSettingDataAsset* GetGameSettings() const;

	ECharacterAnimationType GetCurrentCharacterAnimationType() const;
	const UPaperSprite* GetCurrentBaseFrameSprite() const;
	float GetCurrentBasePixelsPerUnrealUnit() const;

	const FCharacterDataTableRow* GetCharacterData() const;

	void OnLookRight(float Value);
	void OnLookUp(float Value);
	void OnMoveRight(float Value);
	void OnFly();
	void OnFinishFly();
	void OnInteract();
	void OnFinishInteract();
	void OnQuickSlot(int32 SlotIndex);
	void OnDrillWheel(float Value);

	void OnItemCollectRangeChanged(const FOnAttributeChangeData& Data);
	void OnInventoryChanged();
	void OnSkillPurchased(FName PurchasedSkillKeyName);
	void OnRecipeChanged(FName RecipeKey);
	void OnCurrentHealthChanged(const FOnAttributeChangeData& Data);
	void OnCurrentOxygenChanged(const FOnAttributeChangeData& Data);

private:
	friend class UMKCharacterVisualComponent;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMagnetRangeComponent> MagnetRangeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkillComponent> SkillComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UConsumableComponent> ConsumableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCraftingComponent> CraftingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMKCharacterVisualComponent> CharacterVisualComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> InteractionPromptComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character", meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetCharacterRowNames"))
	FName CharacterDataRowKey = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Animation")
	ECharacterAnimationType CurrentCharacterAnimationType = ECharacterAnimationType::Idle_Down;

	UPROPERTY(EditDefaultsOnly, Category = "Ability System")
	TArray<TSubclassOf<UGameplayAbility>> InitialGameplayAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Ability System")
	TArray<TSubclassOf<UGameplayEffect>> InitialGameplayEffects;

	UPROPERTY(Transient)
	UAttributeSet_Character* AttributeSet_Character;

	UPROPERTY(EditDefaultsOnly, Category = "Invincible")
	FName InvincibleDarkenParameterName = TEXT("DarkenAmount");

	UPROPERTY(EditDefaultsOnly, Category = "Invincible")
	float InvincibleDarkenAmount = 1.f;

	/** 상호작용 키 입력 시 주변 상호작용 대상을 찾는 반경 */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractSearchRadius = 200.f;

	bool bIsFlying = false;
	TWeakObjectPtr<AActor> HeldInteractionActor;
	TWeakObjectPtr<AActor> TappedInteractionActor;
	float InteractionHoldStartedAt = 0.f;
	int32 SelectedDrillCells = MAX_int32;
	bool bInteractionHoldConsumed = false;

private:
	FVector CharacterDir;
	FVector DrillingVector = FVector::ZeroVector;
	FTransform SpawnTransform;

	FTimerHandle KnockbackRecoveryTimerHandle;
	double KnockbackEndTime = 0.0;
	ECharacterAnimationType KnockbackAnimationType = ECharacterAnimationType::Idle_Down;
	bool bPreserveKnockbackFacing = false;
	FActiveGameplayEffectHandle OxygenDrainEffectHandle;
	float AppliedOxygenDrainPerSecond = 0.f;
	bool bIsRespawning = false;
	bool bIsOnSurface = true;
	bool bHasAppliedSaveGame = false;
	bool bIsApplyingInitialEffects = false;
	bool bIsMenuInputBlocked = false;
	bool bUsingIceTraction = false;
	float SavedGroundFriction = 8.f;
	float SavedBrakingDeceleration = 2048.f;
	float SavedBrakingFriction = 0.f;
	float SavedAcceleration = 2048.f;
	bool bSavedSeparateBrakingFriction = false;

	mutable FCharacterDataTableRow CharacterDataCompatibilityCache;
};
