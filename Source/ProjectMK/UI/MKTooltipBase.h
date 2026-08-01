#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"

#include "MKTooltipBase.generated.h"

class UImage;
class UTextBlock;
struct FItemDataTableRow;
struct FSkillDataTableRow;

UCLASS()
class PROJECTMK_API UMKTooltipBase : public UMKUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ShowItemTooltip(FName ItemDataKey);

	UFUNCTION(BlueprintCallable)
	void ShowSkillTooltip(FName SkillDataKey);

	UFUNCTION(BlueprintCallable)
	void HideTooltip();

protected:
	virtual void UpdateWidget() override;

private:
	void ClearTooltip();
	void UpdateTooltipFromItemData(FName ItemDataKey, const FItemDataTableRow& ItemDataTableRow);
	void UpdateTooltipFromSkillData(const FSkillDataTableRow& SkillDataTableRow);
	void UpdateTooltipIcon(UTexture2D* IconTexture);
	void UpdateTooltipIcon(class UPaperSprite* IconSprite);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Title;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Desc;
};
