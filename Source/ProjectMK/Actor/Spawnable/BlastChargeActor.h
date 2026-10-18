#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "BlastChargeActor.generated.h"

class UAbilitySystemComponent;
class UPaperSprite;
class UPaperSpriteComponent;

/** 발파 폭약. 설치 후 지연 시간이 지나면 주변 블록을 파괴한다. 캐릭터에게는 대미지를 주지 않는다 */
UCLASS()
class PROJECTMK_API ABlastChargeActor : public AActor
{
	GENERATED_BODY()

public:
	ABlastChargeActor();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	void InitializeBlastCharge(float InDelay, int32 InRadius, UAbilitySystemComponent* InInstigatorASC, UPaperSprite* InSprite);

private:
	void ApplyExplosion();
	void UpdateBlinkVisual();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> PaperSpriteComponent;

	/** 폭발 직전 이 시간 동안 깜빡임이 빨라진다 */
	UPROPERTY(EditDefaultsOnly, Category = "Blast", meta = (ClampMin = "0.0"))
	float FastBlinkDuration = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Blast", meta = (ClampMin = "0.01"))
	float BlinkInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Blast", meta = (ClampMin = "0.01"))
	float FastBlinkInterval = 0.08f;

private:
	TWeakObjectPtr<UAbilitySystemComponent> InstigatorASC = nullptr;
	FTimerHandle ExplosionTimerHandle;
	FTimerHandle BlinkTimerHandle;
	int32 Radius = 1;
	bool bIsBright = true;
};
