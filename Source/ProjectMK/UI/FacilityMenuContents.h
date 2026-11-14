#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKMenuContentsBase.h"
#include "FacilityMenuContents.generated.h"

class AMineFacilityActor;
class UButton;
class UTextBlock;
class UImage;
class USizeBox;

UCLASS()
class PROJECTMK_API UFacilityMenuContents : public UMKMenuContentsBase
{
	GENERATED_BODY()
public:
	UFacilityMenuContents(const FObjectInitializer& Initializer);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;
public:
	void SetFacility(AMineFacilityActor* Actor);
private:
	void BuildDefaultWidgetTree();
	void ApplyAction(int32 Index);
	UFUNCTION() void OnPrimaryClicked();
	UFUNCTION() void OnSecondaryClicked();
	UFUNCTION() void OnTertiaryClicked();
	TWeakObjectPtr<AMineFacilityActor> Facility;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Dialogue;
	UPROPERTY(Transient) TObjectPtr<UImage> FacilityImage;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> Actions;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> Labels;
	UPROPERTY(Transient) TArray<TObjectPtr<USizeBox>> ActionFrames;
	TArray<int32> Destinations;
};
