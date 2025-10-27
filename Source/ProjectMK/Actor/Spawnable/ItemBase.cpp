// LINK

#include "ProjectMK/Actor/Spawnable/ItemBase.h"

#include "Components/SphereComponent.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"

AItemBase::AItemBase()
{
	PrimaryActorTick.bCanEverTick = true;

	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	RootComponent = SphereCollision;

	PaperSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PaperSprite"));
	PaperSpriteComponent->SetupAttachment(RootComponent);
	PaperSpriteComponent->SetRelativeLocation(FVector::ZeroVector);
}

void AItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdatePosition();
}

void AItemBase::InitializeItemBase(FName InItemKey)
{
	ItemKey = InItemKey;
}

bool AItemBase::IsOccupied()
{
	return Looter.IsValid();
}

void AItemBase::TryLoot(TWeakObjectPtr<AMKCharacter> InLooter)
{
	if (IsOccupied())
	{
		return;
	}

	Looter = InLooter;
}

void AItemBase::OnLootFinished()
{
	Destroy();
}

void AItemBase::UpdatePosition()
{
	if (::IsValid(PaperSpriteComponent) == false)
	{
		return;
	}

	if (IsOccupied())
	{
		FVector ItemLocation = GetActorLocation();
		FVector PlayerLocation = Looter->GetActorLocation();

		CurrentLootingSpeed += GetWorld()->GetDeltaSeconds() * LootingAccelerationRate;

		FVector NewLocation = FMath::VInterpConstantTo(ItemLocation, PlayerLocation, GetWorld()->GetDeltaSeconds(), CurrentLootingSpeed);
		SetActorLocation(NewLocation);
	}
	else
	{
		float Time = GetWorld()->GetTimeSeconds();
		float OffsetZ = FMath::Sin(Time * FloatSpeed) * FloatAmplitude;

		PaperSpriteComponent->SetRelativeLocation(FVector(0.f, 0.f, OffsetZ));
	}
}