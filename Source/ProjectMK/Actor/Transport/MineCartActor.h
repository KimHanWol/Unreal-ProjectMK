#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectMK/Interface/Interactable.h"
#include "ProjectMK/Data/MineTownData.h"
#include "MineCartActor.generated.h"

class UBoxComponent;
class UPaperSpriteComponent;
class ARailTrackActor;
class AStorageChestActor;

UCLASS()
class PROJECTMK_API AMineCartActor : public AActor, public IInteractable
{
	GENERATED_BODY()
public:
	AMineCartActor();
	virtual bool CanInteract(AActor* Interactor) override;
	virtual bool ShouldShowInteractionPrompt() const override { return false; }
	virtual int32 GetInteractionPriority() const override { return 5; }
	virtual FName GetRecoveryItemKey() const override;
	virtual FText GetRecoveryBlockedReason(AActor* Interactor) const override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
public:
	void InitializeCart(const FMineCartSaveData& Data, ARailTrackActor* Rail);
	void UpdateCart(float DeltaSeconds);
	void SetCargo(AStorageChestActor* Chest);
	AStorageChestActor* GetCargo() const;
	ARailTrackActor* GetRail() const;
	FGuid GetCartId() const;
	FMineCartSaveData GetSaveData() const;
	float GetSpeed() const;
	float GetCargoMountHeight() const;
	static FVector CalculateRailDirection(const ARailTrackActor* Rail, float Alpha);
	static FVector CalculateRailPosition(const ARailTrackActor* Rail, float Alpha);
private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> CartBody;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPaperSpriteComponent> CartSprite;
	TWeakObjectPtr<ARailTrackActor> CurrentRail;
	TWeakObjectPtr<AStorageChestActor> Cargo;
	FMineCartSaveData CartData;
	float PushTime = 0.f;
};
