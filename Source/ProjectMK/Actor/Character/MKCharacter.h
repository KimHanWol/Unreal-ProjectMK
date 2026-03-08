// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "PaperZDCharacter.h"
#include "GameplayEffectTypes.h"
#include "ProjectMK/Interface/Damageable.h"
#include "MKCharacter.generated.h"

class UCameraComponent;
class UGameplayAbility;
class UGameplayEffect;
class UInteractComponent;
class UInventoryComponent;
class UMaterialInstanceDynamic;
class UPaperSpriteComponent;
struct FGameplayTag;
class UGameSettingDataAsset;

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
	int32 GetCurrentBlockDepth() const;
	const UGameSettingDataAsset* GetGameSettings() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	//TODO: ASC 커스텀하게 만들어서 가지고 있게 하기
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

	bool bIsFlying = false;

private:
	FVector CharacterDir;
	FActiveGameplayEffectHandle OxygenDrainEffectHandle;
	float AppliedOxygenDrainPerSecond = 0.f;
};

