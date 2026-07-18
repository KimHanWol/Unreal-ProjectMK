#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "InventoryBagWidget.generated.h"

UCLASS()
class PROJECTMK_API UInventoryBagWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
};
