#include "ProjectMK/Component/MKCharacterMovementComponent.h"

void UMKCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (CustomMovementMode != SupportClimbMode) { Super::PhysCustom(DeltaTime, Iterations); return; }
	if (DeltaTime < MIN_TICK_TIME || UpdatedComponent == nullptr) { return; }
	Velocity = FVector(0.f, 0.f, SupportClimbSpeed);
	FHitResult Hit;
	SafeMoveUpdatedComponent(Velocity * DeltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
	if (Hit.IsValidBlockingHit()) { HandleImpact(Hit, DeltaTime, Velocity * DeltaTime); Velocity = FVector::ZeroVector; }
}

void UMKCharacterMovementComponent::SetClimbSpeed(float Speed) { SupportClimbSpeed = Speed; }
