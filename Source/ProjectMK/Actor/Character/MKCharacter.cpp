// LINK

#include "ProjectMK/Actor/Character/MKCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Component/InteractComponent.h"

AMKCharacter::AMKCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SetRootComponent(GetCapsuleComponent());

	SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PaperSpriteComponent"));
	SpriteComponent->SetupAttachment(RootComponent);
	SpriteComponent->SetRelativeLocation(FVector::ZeroVector);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpriteComponent);

	InteractComponent = CreateDefaultSubobject<UInteractComponent>(TEXT("InteractComponent"));
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

	PlayerInputComponent->BindAxis("LookRight", this, &AMKCharacter::LookRight);
	PlayerInputComponent->BindAxis("LookUp", this, &AMKCharacter::LookUp);
}

void AMKCharacter::MoveRight(float Value)
{
	AddMovementInput(FVector::ForwardVector, Value * MoveSpeed);
}

void AMKCharacter::LookRight(float Value)
{
	if (::IsValid(InteractComponent))
	{
		InteractComponent->UpdateCharacterDirection(FVector(Value, 0.f, 0.f));
	}
}

void AMKCharacter::LookUp(float Value)
{
	if (::IsValid(InteractComponent))
	{
		InteractComponent->UpdateCharacterDirection(FVector(0.f, 0.f, Value));
	}
}

