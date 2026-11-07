#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "InteractionPromptWidget.generated.h"

UCLASS()
class PROJECTMK_API UInteractionPromptWidget : public UMKUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildDefaultWidgetTree();
};
