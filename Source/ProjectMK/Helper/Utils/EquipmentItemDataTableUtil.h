#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class UPaperSprite;
struct FEquipmentItemDataTableRow;

namespace FEquipmentItemDataTableUtil
{
	const FEquipmentItemDataTableRow* FindEquipmentItemData(UObject* WorldContextObject, FName EquipmentKey);
	TArray<FName> GetEquipmentItemRowNames(UObject* WorldContextObject);
	UTexture2D* LoadIdlePreviewTexture(const FEquipmentItemDataTableRow& EquipmentData);
	bool GetIdlePreviewTextureRegion(const FEquipmentItemDataTableRow& EquipmentData, UTexture2D*& OutTexture, FVector2D& OutSourceUV, FVector2D& OutSourceSize);
	UPaperSprite* CreateIdlePreviewSprite(UObject* Outer, const FEquipmentItemDataTableRow& EquipmentData, float DefaultPixelsPerUnrealUnit = 2.56f);
}
