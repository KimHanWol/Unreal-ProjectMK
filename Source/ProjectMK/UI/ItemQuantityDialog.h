#pragma once
#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "ItemQuantityDialog.generated.h"

class USpinBox;
class UButton;
class UTextBlock;
class UImage;
class UHorizontalBox;

UCLASS()
class PROJECTMK_API UItemQuantityDialog : public UMKUserWidget
{
	GENERATED_BODY()
public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	UFUNCTION(BlueprintCallable)
	void ShowQuantity(FName InItemKey, int32 InMaximumCount, bool bDrop = false);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuantityConfirmed, FName, int32);
	FOnQuantityConfirmed OnQuantityConfirmed;
	DECLARE_MULTICAST_DELEGATE(FOnQuantityCancelled);
	FOnQuantityCancelled OnQuantityCancelled;
protected:
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
private:
	void SetQuantity(int32 Count);
	UFUNCTION()
	void OnConfirm();
	UFUNCTION()
	void OnCancel();
	UFUNCTION()
	void OnQuantityChanged(float Value);
	UFUNCTION()
	void OnQuantityCommitted(float Value, ETextCommit::Type CommitType);
	UFUNCTION()
	void OnOne();
	UFUNCTION()
	void OnHalf();
	UFUNCTION()
	void OnAll();
	UFUNCTION()
	void OnDecrease();
	UFUNCTION()
	void OnIncrease();
	FName ItemKey;
	int32 MaximumCount = 1;
	int32 UnitPrice = 0;
	UPROPERTY(Transient)
	TObjectPtr<USpinBox> QuantityInput;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> CancelButton;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ItemTitle;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ActionTitle;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ConfirmLabel;
	UPROPERTY(Transient)
	TObjectPtr<UImage> ItemIcon;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> OwnedText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TotalPrice;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> PriceRow;
	UPROPERTY(Transient)
	TObjectPtr<UButton> OneButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> HalfButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> AllButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> DecreaseButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> IncreaseButton;
};
