#include "ProjectMK/Actor/Spawnable/BlastChargeActor.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ABlastChargeActor::ABlastChargeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	PaperSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PaperSprite"));
	PaperSpriteComponent->SetupAttachment(RootComponent);
	PaperSpriteComponent->SetRelativeLocation(FVector(0.f, -2.f, 0.f));
	PaperSpriteComponent->SetRelativeScale3D(FVector(0.7f, 1.f, 0.7f));
	PaperSpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentSpriteMaterialFinder(TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	if (TranslucentSpriteMaterialFinder.Succeeded())
	{
		PaperSpriteComponent->SetMaterial(0, TranslucentSpriteMaterialFinder.Object);
	}
}

void ABlastChargeActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExplosionTimerHandle);
		World->GetTimerManager().ClearTimer(BlinkTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ABlastChargeActor::InitializeBlastCharge(float InDelay, int32 InRadius, UAbilitySystemComponent* InInstigatorASC, UPaperSprite* InSprite)
{
	Radius = FMath::Max(0, InRadius);
	InstigatorASC = InInstigatorASC;

	if (::IsValid(PaperSpriteComponent) && ::IsValid(InSprite))
	{
		PaperSpriteComponent->SetSprite(InSprite);
	}

	UWorld* World = GetWorld();
	if (::IsValid(World) == false || InDelay <= 0.f)
	{
		ApplyExplosion();
		return;
	}

	World->GetTimerManager().SetTimer(ExplosionTimerHandle, this, &ABlastChargeActor::ApplyExplosion, InDelay, false);
	World->GetTimerManager().SetTimer(BlinkTimerHandle, this, &ABlastChargeActor::UpdateBlinkVisual, BlinkInterval, false);
}

void ABlastChargeActor::ApplyExplosion()
{
	UWorld* World = GetWorld();
	ULevelManagerSubsystem* LevelManagerSubsystem = ::IsValid(World) ? World->GetSubsystem<ULevelManagerSubsystem>() : nullptr;
	if (::IsValid(LevelManagerSubsystem))
	{
		LevelManagerSubsystem->ApplyExplosion(GetActorLocation(), Radius, InstigatorASC.Get());
	}

	Destroy();
}

void ABlastChargeActor::UpdateBlinkVisual()
{
	UWorld* World = GetWorld();
	if (::IsValid(PaperSpriteComponent) == false || ::IsValid(World) == false)
	{
		return;
	}

	bIsBright = !bIsBright;
	PaperSpriteComponent->SetSpriteColor(bIsBright ? FLinearColor::White : FLinearColor(1.f, 0.35f, 0.35f, 1.f));

	// 폭발이 가까워질수록 깜빡임이 빨라진다.
	const float RemainingTime = World->GetTimerManager().GetTimerRemaining(ExplosionTimerHandle);
	const float NextInterval = RemainingTime <= FastBlinkDuration ? FastBlinkInterval : BlinkInterval;
	World->GetTimerManager().SetTimer(BlinkTimerHandle, this, &ABlastChargeActor::UpdateBlinkVisual, NextInterval, false);
}
