#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "RecipeInfoWidget.generated.h"

class UBorder;

UCLASS()
class PROJECTMK_API URecipeInfoWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	void SetInfoContent(UWidget* Content);
	void SetRecipe(FName Key, bool bBlueprint);
	void SetItem(FName Key);

protected:
	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBorder> ContentFrame;
	FName RecipeKey;
	bool bIsBlueprint = false;
	bool bIsItem = false;
};
