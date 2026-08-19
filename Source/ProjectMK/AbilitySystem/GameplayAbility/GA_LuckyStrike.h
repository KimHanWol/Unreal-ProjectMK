#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Skill.h"
#include "GA_LuckyStrike.generated.h"

UCLASS()
class PROJECTMK_API UGA_LuckyStrike : public UGA_Skill
{
	GENERATED_BODY()

public:
	UGA_LuckyStrike();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

private:
	void WaitDrillHitBlockEvent();

	UFUNCTION()
	void OnDrillHitBlockEvent(FGameplayEventData Payload);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Lucky Strike")
	float LuckyStrikeChance = 0.1f;
};
