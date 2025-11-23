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
};