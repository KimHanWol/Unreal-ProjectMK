// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "MKCharacter.generated.h"

class UCameraComponent;
class UGameplayAbility;
class UInteractComponent;
class UPaperSpriteComponent;

UCLASS()
class PROJECTMK_API AMKCharacter : public ACharacter, public IAbilitySystemInterface
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
	void GiveAbilities();
	void InitializeCharacterAttribute();

private:
	void MoveRight(float Value);
	void LookRight(float Value);
	void LookUp(float Value);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> SpriteComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractComponent> InteractComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(EditAnywhere)
	float MoveSpeed = 1.f;

	//TODO: ASC Ŀ�����ϰ� ���� ������ �ְ� �ϱ�
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UGameplayAbility>> InitialGameplayAbilities;

private:
	UPROPERTY(Transient)
	FVector CharacterDir;
};
