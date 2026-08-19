#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Skill.h"
#include "GA_Recycler.generated.h"

UCLASS()
class PROJECTMK_API UGA_Recycler : public UGA_Skill
{
	GENERATED_BODY()

public:
	UGA_Recycler();

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
	UPROPERTY(EditDefaultsOnly, Category = "Recycler")
	float RecyclerChance = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Recycler")
	int32 RecyclerCoinReward = 1;
};
