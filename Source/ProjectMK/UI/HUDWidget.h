#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "HUDWidget.generated.h"

class UHealthBarWidget;
class UInventoryWidget;
class UOxygenBarWidget;
class UShopWidget;

UCLASS()
class PROJECTMK_API UHUDWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	bool ToggleInventoryWidget();
	bool ToggleShopWidget();
	bool IsMenuVisible() const;

protected:
	UPROPERTY(meta = (BindWidget))
	UHealthBarWidget* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UOxygenBarWidget* OxygenBar;

	UPROPERTY(meta = (BindWidgetOptional))
	UInventoryWidget* Inventory;

	UPROPERTY(meta = (BindWidget))
	UShopWidget* Shop;
};
