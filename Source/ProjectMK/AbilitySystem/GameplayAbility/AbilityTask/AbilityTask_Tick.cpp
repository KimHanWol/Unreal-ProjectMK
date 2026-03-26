// LINK

#include "AbilityTask_Tick.h"

UAbilityTask_Tick::UAbilityTask_Tick()
{
	bTickingTask = true;
}

UAbilityTask_Tick* UAbilityTask_Tick::CreateTickTask(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UAbilityTask_Tick>(OwningAbility);
}

void UAbilityTask_Tick::TickTask(float DeltaTime)
{
	if (!ShouldBroadcastAbilityTaskDelegates())
	{
		return;
	}

	OnTick.Broadcast(DeltaTime);
}

void UAbilityTask_Tick::OnDestroy(bool AbilityEnded)
{
	OnTick.Clear();
	Super::OnDestroy(AbilityEnded);
}
