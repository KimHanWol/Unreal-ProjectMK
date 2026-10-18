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
class UPaperSprite;
class USkillComponent;
class UTexture2D;
class UGameSettingDataAsset;
struct FCharacterDataTableRow;

UCLASS()
class PROJECTMK_API AMKCharacter : public APaperCharacter, public IAbilitySystemInterface, public IDamageable
{
	GENERATED_BODY()

public:
	AMKCharacter();
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

	FVector GetCharacterDirection() const;
	FVector GetDrillingVector() const;
	bool IsKnockedBack() const;
	bool IsOnGround() const;
	USkillComponent* GetSkillComponent() const;
	UConsumableComponent* GetConsumableComponent() const;
	UCraftingComponent* GetCraftingComponent() const;

private:
	void GiveAbilities();
	void InitializeCharacterAttributes();
	void ApplyInitialEffects();
	void ApplyDeathPenalty();
	float CalculateCurrentMoveSpeed() const;

	void TryDrill();
	bool TryInteractNearby();

	void UpdateHorizontalMovement();
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
	void OnActivateMagnet();
	void OnDeactivateMagnet();
	void OnInteract();
	void OnQuickSlot(int32 SlotIndex);
	void OnMagnetRangeExpansionFinished();

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
	float InteractSearchRadius = 40.f;

	bool bIsFlying = false;

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
	bool bIsMagnetInputHeld = false;
	bool bIsOnSurface = true;
	bool bHasAppliedSaveGame = false;
	bool bIsApplyingInitialEffects = false;
	bool bIsMenuInputBlocked = false;

	mutable FCharacterDataTableRow CharacterDataCompatibilityCache;
};
