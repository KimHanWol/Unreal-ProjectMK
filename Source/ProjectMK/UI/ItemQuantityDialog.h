#pragma once
#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "ItemQuantityDialog.generated.h"

class USpinBox;
class UButton;
class UTextBlock;

UCLASS()
class PROJECTMK_API UItemQuantityDialog : public UMKUserWidget
{
	GENERATED_BODY()
public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	UFUNCTION(BlueprintCallable)
	void ShowQuantity(FName InItemKey, int32 InMaximumCount);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuantityConfirmed, FName, int32);
	FOnQuantityConfirmed OnQuantityConfirmed;
protected:
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
private:
	UFUNCTION()
	void OnConfirm();
	UFUNCTION()
	void OnCancel();
	FName ItemKey;
	int32 MaximumCount = 1;
	UPROPERTY(Transient)
	TObjectPtr<USpinBox> QuantityInput;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> CancelButton;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ItemTitle;
};
