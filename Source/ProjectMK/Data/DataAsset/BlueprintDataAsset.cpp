// LINK

#include "ProjectMK/Data/DataAsset/BlueprintDataAsset.h"

TSubclassOf<AActor> UBlueprintDataAsset::GetBlueprintClass(EBlueprintClassType BlueprintClassType)
{
	TSubclassOf<AActor>* BlueprintClassPtr = BlueprintClassMap.Find(BlueprintClassType);
	if (BlueprintClassPtr)
	{
		return (*BlueprintClassPtr);
	}

	return nullptr;
}
