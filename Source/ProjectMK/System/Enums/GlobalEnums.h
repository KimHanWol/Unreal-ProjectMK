// LINK

#pragma once

#include "CoreMinimal.h"
#include "GlobalEnums.generated.h"

UENUM(BlueprintType)
enum class EDataTableType : uint8
{
	None = 0,
	Block = 1,
	Item = 2,
	// Do not reuse this value. It preserves historic enum ordering used by older serialized assets.
	LegacyReserved = 3 UMETA(Hidden),
	Character = 4,
	ShopRecipe = 5,
	Sound = 6,
};

UENUM(BlueprintType)
enum class ESFXType : uint8
{
	None,
	Floating,
	Drill,
	Break_Dirt,
};

UENUM(BlueprintType)
enum class ESFXInstanceParamType : uint8
{
	None,
	DestroyedBlockCount,
	Max,
};

UENUM(BlueprintType)
enum class ESFXContinuousParamType : uint8
{
	None,
	HP,
	Flying,
	Drill,
};
