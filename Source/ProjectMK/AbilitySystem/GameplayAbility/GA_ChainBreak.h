#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Skill.h"
#include "GA_ChainBreak.generated.h"

UCLASS()
class PROJECTMK_API UGA_ChainBreak : public UGA_Skill
{
	GENERATED_BODY()

public:
	UGA_ChainBreak();

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
};
