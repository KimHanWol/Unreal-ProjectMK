#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "HUDWidget.generated.h"

class UInventoryWidget;
class UMKMenuBase;

UCLASS()
class PROJECTMK_API UHUDWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	bool ToggleInventoryWidget();
	bool ToggleSkillTree();
	bool IsMenuVisible() const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInventoryWidget> Inventory;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMKMenuBase> Menu;
};
