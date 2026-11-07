#pragma once

#include "CoreMinimal.h"
#include "MKUserWidget.h"
#include "CurrencyWidget.generated.h"

struct FOnAttributeChangeData;

UCLASS()
class PROJECTMK_API UCurrencyWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

private:
	void BuildDefaultWidgetTree();
	void RefreshCoinText();
	void OnCoinChanged(const FOnAttributeChangeData& Data);

protected:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_Coin;
};
