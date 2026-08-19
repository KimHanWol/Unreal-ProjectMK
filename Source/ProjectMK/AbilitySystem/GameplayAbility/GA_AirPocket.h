#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Skill.h"
#include "GA_AirPocket.generated.h"

UCLASS()
class PROJECTMK_API UGA_AirPocket : public UGA_Skill
{
	GENERATED_BODY()

public:
	UGA_AirPocket();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

private:
	void WaitBlockDestroyedEvent();

	UFUNCTION()
	void OnBlockDestroyedEvent(FGameplayEventData Payload);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Air Pocket")
	float AirPocketChance = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Air Pocket", meta = (ClampMin = "1"))
	int32 OxygenItemCount = 5;
};
