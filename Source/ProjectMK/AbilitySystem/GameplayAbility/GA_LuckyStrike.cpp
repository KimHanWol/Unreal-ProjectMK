// LINK

#include "ProjectMK/AbilitySystem/GameplayAbility/GA_LuckyStrike.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Block.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"

namespace
{
	const FGameplayTag LuckyStrikeDrillHitBlockEventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Drill.HitBlock"));
	const FGameplayTag LuckyStrikeSkillAbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.Passive.LuckyStrike"));
}

UGA_LuckyStrike::UGA_LuckyStrike()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(LuckyStrikeSkillAbilityTag);
	SetAssetTags(AssetTags);
}

void UGA_LuckyStrike::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	WaitDrillHitBlockEvent();
}

void UGA_LuckyStrike::WaitDrillHitBlockEvent()
{
	UAbilityTask_WaitGameplayEvent* WaitGameplayEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, LuckyStrikeDrillHitBlockEventTag, nullptr, false, false);
	WaitGameplayEventTask->EventReceived.AddDynamic(this, &UGA_LuckyStrike::OnDrillHitBlockEvent);
	WaitGameplayEventTask->ReadyForActivation();
}

void UGA_LuckyStrike::OnDrillHitBlockEvent(FGameplayEventData Payload)
{
	const float Roll = FMath::FRand();
	const bool bTriggeredLuckyStrike = Roll <= FMath::Clamp(LuckyStrikeChance, 0.f, 1.f);
	UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][LuckyStrike] TargetBlock=%s Roll=%.3f Chance=%.3f Triggered=%s"),
		*GetNameSafe(Payload.Target.Get()),
		Roll,
		LuckyStrikeChance,
		bTriggeredLuckyStrike ? TEXT("true") : TEXT("false"));

	if (bTriggeredLuckyStrike == false)
	{
		return;
	}

	const ABlockBase* TargetBlock = Cast<ABlockBase>(Payload.Target.Get());
	UAbilitySystemComponent* OwnerAbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (::IsValid(TargetBlock) == false || ::IsValid(OwnerAbilitySystemComponent) == false)
	{
		return;
	}

	UAbilitySystemComponent* TargetAbilitySystemComponent = TargetBlock->GetAbilitySystemComponent();
	const UAttributeSet_Block* BlockAttributeSet = ::IsValid(TargetAbilitySystemComponent)
		? Cast<UAttributeSet_Block>(TargetAbilitySystemComponent->GetAttributeSet(UAttributeSet_Block::StaticClass()))
		: nullptr;
	if (::IsValid(TargetAbilitySystemComponent) == false || ::IsValid(BlockAttributeSet) == false)
	{
		return;
	}

	FGameplayAbilityUtils::ApplyDamageToDurability(TargetAbilitySystemComponent, OwnerAbilitySystemComponent, BlockAttributeSet->GetDurability());
	UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][LuckyStrike] Applied jackpot damage %.2f to %s"), BlockAttributeSet->GetDurability(), *GetNameSafe(TargetBlock));
}
