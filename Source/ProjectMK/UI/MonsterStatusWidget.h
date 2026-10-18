#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "MonsterStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class PROJECTMK_API UMonsterStatusWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

public:
	void UpdateStatus(const FText& Text, float HealthRatio, const FLinearColor& Color);

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthBar;
};
