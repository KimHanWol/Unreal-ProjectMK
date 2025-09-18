// LINK

#include "ProjectMK/Actor/Character/MKCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "PaperSpriteComponent.h"

AMKCharacter::AMKCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SetRootComponent(GetCapsuleComponent());

	SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PaperSpriteComponent"));
	SpriteComponent->SetupAttachment(RootComponent);
	SpriteComponent->SetRelativeLocation(FVector::ZeroVector);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpriteComponent);
}

void AMKCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AMKCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveRight", this, &AMKCharacter::MoveRight);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMKCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMKCharacter::StopJumping);
}

void AMKCharacter::MoveRight(float Value)
{
	AddMovementInput(FVector::ForwardVector, Value * MoveSpeed);
}

