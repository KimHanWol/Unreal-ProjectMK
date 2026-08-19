#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"
#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Skill.h"
#include "GA_FieldRepair.generated.h"

class UAbilityTask_WaitDelay;

UCLASS()
class PROJECTMK_API UGA_FieldRepair : public UGA_Skill
{
	GENERATED_BODY()

public:
	UGA_FieldRepair();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

private:
	void WaitForNextRepair();
	void WaitForBuffEnd();
	void ClearBuffEffects();

	UFUNCTION()
	void OnRepairIntervalFinished();

	UFUNCTION()
	void OnBuffDurationFinished();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Field Repair")
	float RepairInterval = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Field Repair")
	float BuffDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Field Repair")
	TArray<FSkillGameplayEffectData> BuffEffectList;

	TWeakObjectPtr<UAbilityTask_WaitDelay> RepairDelayTask = nullptr;
	TWeakObjectPtr<UAbilityTask_WaitDelay> BuffDelayTask = nullptr;
	TArray<FActiveGameplayEffectHandle> ActiveBuffEffectHandleList;
};
