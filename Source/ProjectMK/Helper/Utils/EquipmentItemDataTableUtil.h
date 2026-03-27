#pragma once

#include "CoreMinimal.h"

struct FEquipmentItemDataTableRow;

namespace FEquipmentItemDataTableUtil
{
	const FEquipmentItemDataTableRow* FindEquipmentItemData(UObject* WorldContextObject, FName EquipmentKey);
	TArray<FName> GetEquipmentItemRowNames(UObject* WorldContextObject);
}
