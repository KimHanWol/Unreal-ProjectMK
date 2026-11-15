#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectMK/Interface/Interactable.h"
#include "ProjectMK/Data/MineTownData.h"
#include "RailTrackActor.generated.h"

class UBoxComponent;
class UPaperSpriteComponent;

UCLASS()
class PROJECTMK_API ARailTrackActor : public AActor, public IInteractable
{
	GENERATED_BODY()
public:
	ARailTrackActor();
	virtual bool CanInteract(AActor* Interactor) override;
	virtual bool ShouldShowInteractionPrompt() const override { return false; }
	virtual FName GetRecoveryItemKey() const override;
	virtual FText GetRecoveryBlockedReason(AActor* Interactor) const override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
public:
	void InitializeRail(const FRailSaveData& Data);
	void UpdateEndCaps(bool bStart, bool bEnd);
	FVector GetStartWorld() const;
	FVector GetEndWorld() const;
	const FRailSaveData& GetSaveData() const;
private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPaperSpriteComponent> RailSprite;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPaperSpriteComponent> Trestle;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPaperSpriteComponent> StartCap;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPaperSpriteComponent> EndCap;
	FRailSaveData RailData;
};
