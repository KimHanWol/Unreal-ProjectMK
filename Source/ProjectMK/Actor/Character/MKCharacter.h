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
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
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
	void MoveRight(float Value);
	void LookRight(float Value);
	void LookUp(float Value);
	void Fly();
	void FinishFly();

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

	float DoublePressDuration = 0.5f;
	float LastUpPressedTime = 0.f;
	bool bIsUpPressing = false;
	bool bIsUpDoublePressing = false;

private:
	FVector CharacterDir;
};
