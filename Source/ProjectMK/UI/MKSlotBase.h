#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"

#include "MKSlotBase.generated.h"

class UImage;
class UBorder;
class UTextBlock;
class UPaperSprite;
class UTexture2D;
class UMKTooltipBase;

UCLASS()
class PROJECTMK_API UMKSlotBase : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

protected:
	virtual void UpdateWidget() override;

	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	FReply HandleSlotClicked(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);

protected:
	virtual bool CanClickSlot() const;
	virtual void OnSlotClicked();
	virtual void OnSlotHovered();
	virtual void OnSlotUnhovered();

public:
	virtual void UpdateTooltipWidget(UMKTooltipBase* TooltipWidget) const;
	void SetSlotIcon(UPaperSprite* IconSprite);
	void SetSlotIcon(UTexture2D* IconTexture);
	void SetSlotCount(int32 Count, bool bShowCount = true);
	void SetKeyGuide(const FText& KeyGuide);
	void SetSelected(bool bSelected);
	void SetSlotLocked(bool bLocked);
	void SetRankBadge(const FText& Rank);
	void SetAcquired(bool bAcquired);
	void ClearSlotIcon();

protected:
	void BuildDefaultWidgetTree();

private:
	void UpdateFocusImageVisibility();

protected:
	UPROPERTY(Transient)
	TObjectPtr<UImage> LockImage;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RankBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RankText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> AcquiredImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> AcquiredSeal;

	bool bIsSlotLocked = false;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> FocusImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> KeyGuideBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeyGuideText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectionText;

	UPROPERTY(EditDefaultsOnly, Category = "Slot")
	FVector2D DefaultSlotSize = FVector2D(64.f, 64.f);

private:
	UPROPERTY(Transient)
	bool bHasSlotFocus = false;
};
