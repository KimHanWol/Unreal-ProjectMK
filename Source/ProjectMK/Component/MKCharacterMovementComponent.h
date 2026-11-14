#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MKCharacterMovementComponent.generated.h"

UCLASS()
class PROJECTMK_API UMKCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	static constexpr uint8 SupportClimbMode = 1;
protected:
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
public:
	void SetClimbSpeed(float Speed);
private:
	float SupportClimbSpeed = 32.f;
};
