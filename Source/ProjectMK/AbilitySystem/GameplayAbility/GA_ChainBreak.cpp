// LINK

#include "ProjectMK/AbilitySystem/GameplayAbility/GA_ChainBreak.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Helper/Utils/BlockUtils.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"

namespace
{
	const FGameplayTag ChainBreakDrillHitBlockEventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Drill.HitBlock"));
	const FGameplayTag ChainBreakSkillAbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.Passive.ChainBreak"));
}

UGA_ChainBreak::UGA_ChainBreak()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(ChainBreakSkillAbilityTag);
	SetAssetTags(AssetTags);
}

void UGA_ChainBreak::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	WaitDrillHitBlockEvent();
}

void UGA_ChainBreak::WaitDrillHitBlockEvent()
{
	UAbilityTask_WaitGameplayEvent* WaitGameplayEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ChainBreakDrillHitBlockEventTag, nullptr, false, false);
	WaitGameplayEventTask->EventReceived.AddDynamic(this, &UGA_ChainBreak::OnDrillHitBlockEvent);
	WaitGameplayEventTask->ReadyForActivation();
}

void UGA_ChainBreak::OnDrillHitBlockEvent(FGameplayEventData Payload)
{
	const ABlockBase* TargetBlock = Cast<ABlockBase>(Payload.Target.Get());
	UWorld* World = GetWorld();
	UAbilitySystemComponent* OwnerAbilitySystemComponent = GetOwnerAbilitySystemComponent();
	const UAttributeSet_Character* CharacterAttributeSet = GetOwnerCharacterAttributeSet();
	if (::IsValid(TargetBlock) == false || ::IsValid(World) == false || ::IsValid(OwnerAbilitySystemComponent) == false || ::IsValid(CharacterAttributeSet) == false)
	{
		return;
	}

	ULevelManagerSubsystem* LevelManagerSubsystem = World->GetSubsystem<ULevelManagerSubsystem>();
	if (::IsValid(LevelManagerSubsystem) == false)
	{
		return;
	}

	const float SurroundDamage = CharacterAttributeSet->GetDrillingPower() * CharacterAttributeSet->GetSurroundDamageRatio();
	TSet<const AActor*> DirectTargetActorSet;
	for (const TWeakObjectPtr<AActor>& DirectTargetActor : Payload.ContextHandle.GetActors())
	{
		if (DirectTargetActor.IsValid())
		{
			DirectTargetActorSet.Add(DirectTargetActor.Get());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][ChainBreak] DirectHitBlock=%s DrillingPower=%.2f SurroundDamageRatio=%.3f SurroundDamage=%.2f"),
		*GetNameSafe(TargetBlock),
		CharacterAttributeSet->GetDrillingPower(),
		CharacterAttributeSet->GetSurroundDamageRatio(),
		SurroundDamage);

	int32 AppliedSurroundBlockCount = 0;
	int32 SkippedDirectBlockCount = 0;
	for (ABlockBase* SurroundBlock : FBlockUtils::GetSurroundBlocks(LevelManagerSubsystem, const_cast<ABlockBase*>(TargetBlock)))
	{
		if (::IsValid(SurroundBlock) == false || SurroundBlock->IsMineable() == false)
		{
			continue;
		}

		if (DirectTargetActorSet.Contains(SurroundBlock))
		{
			SkippedDirectBlockCount++;
			continue;
		}

		if (UAbilitySystemComponent* TargetAbilitySystemComponent = SurroundBlock->GetAbilitySystemComponent())
		{
			FGameplayAbilityUtils::ApplyDamageToDurability(TargetAbilitySystemComponent, OwnerAbilitySystemComponent, SurroundDamage);
			AppliedSurroundBlockCount++;
			UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][ChainBreak] Applied %.2f damage to surround block %s"), SurroundDamage, *GetNameSafe(SurroundBlock));
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][ChainBreak] AppliedTo=%d SkippedDirectTargets=%d"),
		AppliedSurroundBlockCount,
		SkippedDirectBlockCount);
}
