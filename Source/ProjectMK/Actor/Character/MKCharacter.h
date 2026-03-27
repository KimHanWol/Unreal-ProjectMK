#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "PaperZDCharacter.h"
#include "GameplayEffectTypes.h"
#include "ProjectMK/Data/DataTable/CharacterDataTableRow.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"
#include "ProjectMK/Interface/Damageable.h"
#include "MKCharacter.generated.h"

class UCameraComponent;
class UGameplayAbility;
class UGameplayEffect;
class UInteractComponent;
class UInventoryComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMKRuntimePaperSprite;
class UPaperSprite;
class UPaperSpriteComponent;
class UPaperZDAnimPlayer;
class UPaperZDAnimSequence;
class UTexture2D;
struct FGameplayTag;
class UGameSettingDataAsset;
struct FCharacterDataTableRow;
struct FEquipmentItemDataTableRow;

UCLASS()
class PROJECTMK_API AMKCharacter : public APaperZDCharacter, public IAbilitySystemInterface
														   , public IDamageable
{
	GENERATED_BODY()

public:
	AMKCharacter();

	//IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~IAbilitySystemInterface

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	void GiveAbilities();
	void InitializeCharacterAttributes();
	void ApplyInitialEffects();

	virtual void BindEvents();
	virtual void UnbindEvents();

	//IDamageable
protected:
	virtual UAbilitySystemComponent* GetOwnerASC() override;
	virtual bool CheckIsDestroyed() override;
	virtual void OnDestroyed() override;
	//~IDamageable

public:
	FVector GetCharacterDirection() const { return CharacterDir; }
	void SetDrillingVector(const FVector& InDrillingVector);

private:
	void OnLookRight(float Value);
	void OnLookUp(float Value);
	void OnMoveRight(float Value);
	void OnFly();
	void OnFinishFly();

	void TryDrill();

	void OnItemCollectRangeChanged(const FOnAttributeChangeData& Data);
	void OnCurrentHealthChanged(const FOnAttributeChangeData& Data);
	void OnCurrentOxygenChanged(const FOnAttributeChangeData& Data);
	void OnInvincibleTagChanged(const FGameplayTag Tag, int32 NewCount);
	void ApplyDamageInvincibility();
	void InitializeInvincibleMaterial();
	void UpdateOxygen();
	void ApplyOxygenDrainEffect(float OxygenDrainPerSecond);
	void ClearOxygenDrainEffect();
	void RestoreOxygenToMax();
	void Apply2DCameraOverrides();
	void ApplyTextureRenderingOverrides(UTexture2D* Texture) const;
	void ApplySpriteRenderingOverrides(const UPaperSprite* PaperSprite) const;
	int32 GetCurrentBlockDepth() const;
	const UGameSettingDataAsset* GetGameSettings() const;
	void UpdateCharacterAnimationVisual();
	void SetCharacterVisualOverrideEnabled(bool bEnabled);
	void UpdateOverrideVisualFacingDirection();
	ECharacterAnimationType ResolveCurrentCharacterAnimationType() const;
	const UPaperSprite* ResolveCurrentBaseFrameSprite() const;
	float ResolveCurrentBasePixelsPerUnrealUnit() const;
	float ResolveOverrideVisualScale(const UPaperSprite* OverrideSprite) const;
	const FCharacterDataTableRow* GetCharacterData() const;
	const TSoftObjectPtr<UTexture2D>* FindCharacterAnimationTexture(const FCharacterDataTableRow& CharacterData, ECharacterAnimationType AnimationType) const;
	void EnsureCharacterVisualMaterialInstance();
	void InitializeEquipmentOverlayComponents();
	void RefreshEquippedOverlayItems();
	void UpdateEquipmentOverlays();
	void UpdateEquipmentOverlayZOrders();
	bool GetCurrentAnimationPlaybackData(const UPaperZDAnimSequence*& OutAnimationSequence, float& OutPlaybackTime, float& OutPlaybackProgress) const;
	int32 ResolveCurrentAnimationFrameIndex(const UPaperZDAnimSequence* CurrentAnimationSequence, float PlaybackTime, float PlaybackProgress) const;
	const FEquipmentItemDataTableRow* GetEquipmentItemData(FName EquipmentKey);
	const UPaperSprite* ResolveEquipmentOverlaySprite(const FEquipmentItemDataTableRow& EquipmentData, ECharacterAnimationType CurrentAnimationType, const UPaperZDAnimSequence* CurrentAnimationSequence, float PlaybackTime, float PlaybackProgress);
	const UPaperSprite* ResolveAnimationAtlasSprite(UTexture2D* AtlasTexture, int32 AnimationFrameIndex);
	UMKRuntimePaperSprite* GetOrCreateRuntimeAtlasSprite(UTexture2D* AtlasTexture, int32 AtlasCellIndex, float PixelsPerUnrealUnit);
	FName MakeRuntimeAtlasSpriteCacheKey(const UTexture2D* AtlasTexture, int32 AtlasCellIndex, float PixelsPerUnrealUnit) const;
	void SetAnimInstanceVectorVariable(FName VariableName, const FVector& Value);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> CharacterVisualComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character", meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetCharacterRowNames"))
	FName CharacterDataRowKey = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Animation")
	ECharacterAnimationType CurrentCharacterAnimationType = ECharacterAnimationType::Idle;

	// TODO: ASC를 커스텀하게 만들어서 직접 보관하도록 정리
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UGameplayAbility>> InitialGameplayAbilities;

	UPROPERTY(EditAnywhere, Category = "Ability System")
	TArray<TSubclassOf<UGameplayEffect>> InitialGameplayEffects;

	UPROPERTY(Transient)
	UAttributeSet_Character* AttributeSet_Character;

	UPROPERTY(EditAnywhere, Category = "Invincible")
	FName InvincibleDarkenParameterName = TEXT("DarkenAmount");

	UPROPERTY(EditAnywhere, Category = "Invincible")
	float InvincibleDarkenAmount = 1.f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> InvincibleMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CharacterVisualMaterialInstance;

	UPROPERTY(Transient)
	TWeakObjectPtr<UMaterialInterface> CharacterVisualMaterialSource;

	bool bIsFlying = false;

private:
	FVector CharacterDir;
	FVector DrillingVector = FVector::ZeroVector;
	bool bHasAppliedDrillingVectorToAnimInstance = false;
	FActiveGameplayEffectHandle OxygenDrainEffectHandle;
	float AppliedOxygenDrainPerSecond = 0.f;
	float CurrentInvincibleDarkenValue = 0.f;
	float CurrentOverrideVisualScale = 1.f;
	bool bCharacterVisualOverrideEnabled = false;

	UPROPERTY(Transient)
	TMap<EEuipmentType, TObjectPtr<UPaperSpriteComponent>> EquipmentOverlayComponents;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMKRuntimePaperSprite>> RuntimeAtlasSpriteCache;

	TMap<EEuipmentType, FName> EquippedOverlayItemKeys;
	mutable FCharacterDataTableRow CharacterDataCompatibilityCache;
};
