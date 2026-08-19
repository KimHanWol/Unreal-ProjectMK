#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKSlotBase.h"

#include "SkillSlotWidget.generated.h"

struct FOnAttributeChangeData;

UCLASS()
class PROJECTMK_API USkillSlotWidget : public UMKSlotBase
{
	GENERATED_BODY()

protected:
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;
	virtual bool CanClickSlot() const override;
	virtual void OnSlotClicked() override;
	virtual void UpdateTooltipWidget(UMKTooltipBase* TooltipWidget) const override;

private:
	class USkillTreeMenuContents* GetOwningSkillTreeMenuContents() const;
	void UpdateEnabledState();
	void UpdateCompleteImage();
	void UpdateSkillImage();

	void OnCoinChanged(const FOnAttributeChangeData& Data);
	void OnSkillPurchased(FName PurchasedSkillKeyName);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetSkillRowNames"))
	FName SkillKeyName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CompleteImage;

private:
	UPROPERTY(Transient)
	bool bPendingClickPurchase = false;

	UPROPERTY(Transient)
	FVector2D PendingClickScreenPosition = FVector2D::ZeroVector;
};
