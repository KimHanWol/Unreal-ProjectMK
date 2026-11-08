#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "MenuDisplayCardWidget.generated.h"

class UImage;
class UBorder;

UCLASS()
class PROJECTMK_API UMenuDisplayCardWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	void SetDisplayRole(FName InRole);

protected:
	virtual void UpdateWidget() override;

private:
	FName DisplayRole = TEXT("Player");

	UPROPERTY(Transient)
	TObjectPtr<UImage> Portrait;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> Stage;
};
