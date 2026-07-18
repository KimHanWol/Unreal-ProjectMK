#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "PaperCharacter.h"
#include "GameplayEffectTypes.h"
#include "ProjectMK/Data/DataTable/CharacterDataTableRow.h"
#include "ProjectMK/Data/Struct/CharacterAnimationTextureSet.h"
#include "ProjectMK/Interface/Damageable.h"
#include "MKCharacter.generated.h"

class UCameraComponent;
class UGameplayAbility;
class UGameplayEffect;
class UInteractComponent;
class UInventoryComponent;
class UAttributeSet_Character;
class UMKCharacterVisualComponent;
class UPaperSprite;
class UTexture2D;
class UGameSettingDataAsset;
struct FCharacterDataTableRow;

UCLASS()
class PROJECTMK_API AMKCharacter : public APaperCharacter, public IAbilitySystemInterface
														   , public IDamageable
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
	void GiveAbilities();
	void InitializeCharacterAttributes();
	void ApplyInitialEffects();

	virtual void BindEvents();
	virtual void UnbindEvents();

protected:
	virtual UAbilitySystemComponent* GetOwnerASC() override;
	virtual bool CheckIsDestroyed() override;
	virtual void OnDestroyed() override;

public:
	FVector GetCharacterDirection() const { return CharacterDir; }
	FVector GetDrillingVector() const { return DrillingVector; }
	void SetDrillingVector(const FVector& InDrillingVector);

private:
	friend class UMKCharacterVisualComponent;

	void OnLookRight(float Value);
	void OnLookUp(float Value);
	void OnMoveRight(float Value);
	void OnFly();
	void OnFinishFly();

	void TryDrill();

	void OnItemCollectRangeChanged(const FOnAttributeChangeData& Data);
	void OnCurrentHealthChanged(const FOnAttributeChangeData& Data);
	void OnCurrentOxygenChanged(const FOnAttributeChangeData& Data);
	void ApplyDamageInvincibility();
	void UpdateOxygen();
	void ApplyOxygenDrainEffect(float OxygenDrainPerSecond);
	void ClearOxygenDrainEffect();
	void RestoreOxygenToMax();
	void Apply2DCameraOverrides();
	void ApplyTextureRenderingOverrides(UTexture2D* Texture) const;
	void ApplySpriteRenderingOverrides(const UPaperSprite* PaperSprite) const;
	int32 GetCurrentBlockDepth() const;
	const UGameSettingDataAsset* GetGameSettings() const;
	ECharacterAnimationType ResolveCurrentCharacterAnimationType() const;
	const UPaperSprite* ResolveCurrentBaseFrameSprite() const;
	float ResolveCurrentBasePixelsPerUnrealUnit() const;
	const FCharacterDataTableRow* GetCharacterData() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMKCharacterVisualComponent> CharacterVisualComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character", meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetCharacterRowNames"))
	FName CharacterDataRowKey = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Animation")
	ECharacterAnimationType CurrentCharacterAnimationType = ECharacterAnimationType::Idle_Down;

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

	bool bIsFlying = false;

private:
	FVector CharacterDir;
	FVector DrillingVector = FVector::ZeroVector;
	FActiveGameplayEffectHandle OxygenDrainEffectHandle;
	float AppliedOxygenDrainPerSecond = 0.f;
	mutable FCharacterDataTableRow CharacterDataCompatibilityCache;
};
