#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "InventoryWidget.generated.h"

class UInventoryBagWidget;

UCLASS()
class PROJECTMK_API UInventoryWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	UInventoryBagWidget* InventoryBag;
};
