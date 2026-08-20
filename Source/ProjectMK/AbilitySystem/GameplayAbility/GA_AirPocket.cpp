// LINK

#include "ProjectMK/AbilitySystem/GameplayAbility/GA_AirPocket.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameplayTagContainer.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/System/SkillDebugUtils.h"

namespace
{
	const FGameplayTag AirPocketSkillAbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.Passive.AirPocket"));
	const FGameplayTag AirPocketBlockDestroyedEventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Block.Destroyed"));
	const FName AirPocketOxygenItemKey = TEXT("Oxygen");
}

UGA_AirPocket::UGA_AirPocket()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(AirPocketSkillAbilityTag);
	SetAssetTags(AssetTags);
}

void UGA_AirPocket::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	WaitBlockDestroyedEvent();
}

void UGA_AirPocket::WaitBlockDestroyedEvent()
{
	UAbilityTask_WaitGameplayEvent* WaitGameplayEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AirPocketBlockDestroyedEventTag, nullptr, false, false);
	WaitGameplayEventTask->EventReceived.AddDynamic(this, &UGA_AirPocket::OnBlockDestroyedEvent);
	WaitGameplayEventTask->ReadyForActivation();
}

void UGA_AirPocket::OnBlockDestroyedEvent(FGameplayEventData Payload)
{
	const float Roll = FMath::FRand();
	const bool bTriggeredAirPocket = Roll <= FMath::Clamp(AirPocketChance, 0.f, 1.f);
	MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][AirPocket] DestroyedBlock=%s Roll=%.3f Chance=%.3f Triggered=%s OxygenItemCount=%d"),
		*GetNameSafe(Payload.Target.Get()),
		Roll,
		AirPocketChance,
		bTriggeredAirPocket ? TEXT("true") : TEXT("false"),
		OxygenItemCount);

	if (bTriggeredAirPocket)
	{
		if (const ABlockBase* DestroyedBlock = Cast<ABlockBase>(Payload.Target.Get()))
		{
			const_cast<ABlockBase*>(DestroyedBlock)->TryAddSpawnItem(AirPocketOxygenItemKey, OxygenItemCount);
		}
	}
}
