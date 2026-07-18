#pragma once

#include "CoreMinimal.h"

#include "CharacterAnimationTextureSet.generated.h"

UENUM(BlueprintType)
enum class ECharacterAnimationType : uint8
{
	Idle_Down UMETA(DisplayName = "Idle Down"),
	Idle_Up UMETA(DisplayName = "Idle Up"),
	Idle_Left UMETA(DisplayName = "Idle Left"),
	Idle_Right UMETA(DisplayName = "Idle Right"),
	Walk_Left UMETA(DisplayName = "Walk Left"),
	Walk_Right UMETA(DisplayName = "Walk Right"),
};
