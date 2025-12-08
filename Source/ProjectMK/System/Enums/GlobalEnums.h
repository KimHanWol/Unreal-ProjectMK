//LINK

#pragma once

#include "CoreMinimal.h"
#include "GlobalEnums.generated.h"

UENUM(BlueprintType)
enum class EDataTableType : uint8
{
    None,
    Block,
    Item,
    EquipmentItem,
	Sound,
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
};

UENUM(BlueprintType)
enum class ESFXContinuousParamType : uint8
{
	None,
	HP,
	Flying,
	Drill,
};
