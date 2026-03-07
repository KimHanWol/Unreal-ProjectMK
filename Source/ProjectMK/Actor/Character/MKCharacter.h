// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "PaperZDCharacter.h"
#include "ProjectMK/Interface/Damageable.h"
#include "MKCharacter.generated.h"

class UCameraComponent;
class UGameplayAbility;
class UInteractComponent;
class UInventoryComponent;
class UPaperSpriteComponent;

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
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	void GiveAbilities();
	void InitializeCharacterAttribute();

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

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	//TODO: ASC 커스텀하게 만들어서 가지고 있게 하기
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UGameplayAbility>> InitialGameplayAbilities;

	UPROPERTY(Transient)
	UAttributeSet_Character* AttributeSet_Character;

	bool bIsFlying = false;

private:
	FVector CharacterDir;
};
