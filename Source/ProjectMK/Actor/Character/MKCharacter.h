// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "PaperZDCharacter.h"
#include "MKCharacter.generated.h"

class UCameraComponent;
class UGameplayAbility;
class UInteractComponent;
class UInventoryComponent;
class UPaperSpriteComponent;

UCLASS()
class PROJECTMK_API AMKCharacter : public APaperZDCharacter, public IAbilitySystemInterface
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

public:
	UFUNCTION(BlueprintCallable)
	bool IsInteracting();

private:
	void MoveRight(float Value);
	void LookRight(float Value);
	void LookUp(float Value);
	void Fly();
	void FinishFly();

	void OnItemCollectRangeChanged(const FOnAttributeChangeData& Data);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractComponent> InteractComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(EditAnywhere)
	float MoveSpeed = 1.f;

	UPROPERTY(EditAnywhere)
	float FlyingSpeed = 200.f;

	//TODO: ASC 커스텀하게 만들어서 가지고 있게 하기
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UGameplayAbility>> InitialGameplayAbilities;

	UPROPERTY(EditAnywhere)
	float DoublePressDuration = 0.5f;

	UPROPERTY(Transient)
	float LastUpPressedTime = 0.f;

	UPROPERTY(Transient)
	bool bIsUpPressing = false;

	UPROPERTY(Transient)
	bool bIsUpDoublePressing = false;

	UPROPERTY(EditAnywhere)
	float MaxFlySpeed = 1000.f;

private:
	UPROPERTY(Transient)
	FVector CharacterDir;
};
