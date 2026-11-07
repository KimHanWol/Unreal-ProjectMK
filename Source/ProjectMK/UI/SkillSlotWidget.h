#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKSlotBase.h"

#include "SkillSlotWidget.generated.h"

struct FOnAttributeChangeData;
class UHorizontalBox;
class UTextBlock;

UCLASS()
class PROJECTMK_API USkillSlotWidget : public UMKSlotBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
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

public:
	void InitializeSkill(FName InSkillKeyName);

private:
	void BuildDefaultWidgetTree();
	class USkillTreeMenuContents* GetOwningSkillTreeMenuContents() const;
	void UpdateEnabledState();
	void UpdateCompleteImage();
	void UpdateSkillImage();
	void UpdatePriceText();
	bool AreRequiredSkillsPurchased() const;

	void OnCoinChanged(const FOnAttributeChangeData& Data);
	void OnSkillPurchased(FName PurchasedSkillKeyName);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetSkillRowNames"))
	FName SkillKeyName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CompleteImage;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> PriceRow;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PriceText;

private:
	UPROPERTY(Transient)
	bool bPendingClickPurchase = false;

	UPROPERTY(Transient)
	FVector2D PendingClickScreenPosition = FVector2D::ZeroVector;
};
