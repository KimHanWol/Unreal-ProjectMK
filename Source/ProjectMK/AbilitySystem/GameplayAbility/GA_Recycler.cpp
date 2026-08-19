// LINK

#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Recycler.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameplayTagContainer.h"
#include "ProjectMK/Component/SkillComponent.h"

namespace
{
	const FGameplayTag RecyclerBlockDestroyedEventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Block.Destroyed"));
	const FGameplayTag RecyclerSkillAbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.Passive.Recycler"));
}

UGA_Recycler::UGA_Recycler()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(RecyclerSkillAbilityTag);
	SetAssetTags(AssetTags);
}

void UGA_Recycler::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	WaitBlockDestroyedEvent();
}

void UGA_Recycler::WaitBlockDestroyedEvent()
{
	UAbilityTask_WaitGameplayEvent* WaitGameplayEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, RecyclerBlockDestroyedEventTag, nullptr, false, false);
	WaitGameplayEventTask->EventReceived.AddDynamic(this, &UGA_Recycler::OnBlockDestroyedEvent);
	WaitGameplayEventTask->ReadyForActivation();
}

void UGA_Recycler::OnBlockDestroyedEvent(FGameplayEventData Payload)
{
	const float Roll = FMath::FRand();
	const bool bGrantedRecyclerReward = Roll <= FMath::Clamp(RecyclerChance, 0.f, 1.f);
	UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][Recycler] DestroyedBlock=%s Roll=%.3f Chance=%.3f Triggered=%s Reward=%d"),
		*GetNameSafe(Payload.Target.Get()),
		Roll,
		RecyclerChance,
		bGrantedRecyclerReward ? TEXT("true") : TEXT("false"),
		RecyclerCoinReward);

	if (bGrantedRecyclerReward)
	{
		if (USkillComponent* SkillComponent = GetOwnerSkillComponent())
		{
			SkillComponent->TryAddCoin(RecyclerCoinReward);
		}
	}
}
