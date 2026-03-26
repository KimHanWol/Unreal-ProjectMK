// LINK

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Drill.generated.h"

class UAbilitySystemComponent;
class UAbilityTask_WaitDelay;
class AMKCharacter;
class UGameplayEffect;

UCLASS()
class PROJECTMK_API UGA_Drill : public UGameplayAbility
{
	GENERATED_BODY()

public:
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

	UFUNCTION()
	virtual void Tick(float DeltaTime);

private:
	void Tick_UpdateTarget();
	void Tick_UpdateSourcePosition(float DeltaTime);

	void WaitPeriodAndMine();

	UFUNCTION()
	void Drill_Instant();

	void EnableDrill(bool bEnable);

	float DrillingPeriod = 0.f;
	float DrillingPower = 0.f;
	float DrillingDistance = 0.f;

	TWeakObjectPtr<AMKCharacter> SourceCharacter = nullptr;
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;
	TArray<TWeakObjectPtr<UAbilitySystemComponent>> TargetASCList;
	TWeakObjectPtr<UAbilityTask_WaitDelay> DelayTask = nullptr;

	bool bIsDrilling = false;

	TWeakObjectPtr<AActor> InteractionDirTarget = nullptr;
	FVector InteractionStartPoint = FVector::ZeroVector;
	float InteractStartTime = 0.f;
	float InteractingTime = 0.f;
	float InteractionMoveDuration = 0.15f;

	TSubclassOf<UGameplayEffect> MineEffectClass;

	FActiveGameplayEffectHandle DrillEffectHandle;
};
