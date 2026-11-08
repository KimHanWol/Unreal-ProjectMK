#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"

#include "MKTooltipBase.generated.h"

class UImage;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
struct FItemDataTableRow;
struct FSkillDataTableRow;

UCLASS()
class PROJECTMK_API UMKTooltipBase : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable)
	void ShowItemTooltip(FName ItemDataKey);

	UFUNCTION(BlueprintCallable)
	void ShowSkillTooltip(FName SkillDataKey);
	void ShowRecipeTooltip(FName RecipeKey);

	UFUNCTION(BlueprintCallable)
	void HideTooltip();

protected:
	virtual void UpdateWidget() override;

private:
	void BuildDefaultWidgetTree();
	void ClearTooltip();
	void UpdateTooltipFromItemData(FName ItemDataKey, const FItemDataTableRow& ItemDataTableRow);
	void UpdateTooltipFromSkillData(FName SkillKey, const FSkillDataTableRow& SkillDataTableRow);
	void SetCost(int32 Cost, bool bShow);
	void SetStatus(const FText& Text, const FLinearColor& Color);
	void UpdateTooltipIcon(UTexture2D* IconTexture);
	void UpdateTooltipIcon(class UPaperSprite* IconSprite);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Title;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Desc;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CategoryText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CostText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> MaterialRows;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> CostRow;
};
