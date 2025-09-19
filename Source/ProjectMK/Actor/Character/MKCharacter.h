// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MKCharacter.generated.h"

class UCameraComponent;
class UInteractComponent;
class UPaperSpriteComponent;

UCLASS()
class PROJECTMK_API AMKCharacter : public ACharacter
{
	GENERATED_BODY()
	
public:	
	AMKCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

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

	UPROPERTY(EditAnywhere)
	float MoveSpeed = 1.f;
};
