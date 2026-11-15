#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "BountyNoticeWidget.generated.h"

class AMineFacilityActor;
UCLASS()
class PROJECTMK_API UBountyNoticeWidget : public UMKUserWidget
{
	GENERATED_BODY()
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
public:
	void SetContract(AMineFacilityActor* Board, int32 Index);
	FSimpleDelegate OnChosen;
private:
	UFUNCTION() void OnAction();
	UPROPERTY(Transient) TObjectPtr<class UImage> Portrait;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> Title;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> Objective;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> Reward;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> ActionLabel;
	UPROPERTY(Transient) TObjectPtr<class UButton> Action;
};
