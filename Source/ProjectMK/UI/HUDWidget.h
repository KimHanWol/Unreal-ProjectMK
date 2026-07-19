#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "HUDWidget.generated.h"

class UInventoryWidget;
class UShopWidget;

UCLASS()
class PROJECTMK_API UHUDWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	bool ToggleInventoryWidget();
	bool ToggleShopWidget();

	bool IsMenuVisible() const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInventoryWidget> Inventory;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UShopWidget> Shop;
};
