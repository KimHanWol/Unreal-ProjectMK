#pragma once

#include "Blueprint/DragDropOperation.h"

#include "ItemSlotDragDropOperation.generated.h"

UCLASS()
class PROJECTMK_API UItemSlotDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	int32 SourceSlotIndex = INDEX_NONE;
};
