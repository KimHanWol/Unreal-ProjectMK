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

	UPROPERTY(Transient)
	FName ItemKey = NAME_None;

	UPROPERTY(Transient)
	int32 ItemCount = 0;
};
