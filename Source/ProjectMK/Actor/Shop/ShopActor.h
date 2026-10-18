#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectMK/Interface/Interactable.h"
#include "ShopActor.generated.h"

class UBoxComponent;

/** 지표면 Shop 블록 위에 붙어 클릭 / 상호작용 키로 상점 메뉴를 여는 액터 */
UCLASS()
class PROJECTMK_API AShopActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AShopActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

	virtual float GetInteractRange() const override;
	virtual bool OnInteract(AActor* Interactor) override;

	virtual void BindEvents();
	virtual void UnbindEvents();

public:
	void InitializeShop(const FIntPoint& TileSize);

private:
	UFUNCTION()
	void OnOwnerDestroyed(AActor* DestroyedActor);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionBox;

	/** 캐릭터와 상점 중심 사이의 최대 상호작용 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop", meta = (ClampMin = "0.0"))
	float InteractRange = 40.f;
};
