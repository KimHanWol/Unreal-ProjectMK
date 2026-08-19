// LINK

#include "ProjectMK/AbilitySystem/GameplayAbility/GA_FieldRepair.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"

namespace
{
	const FGameplayTag FieldRepairSkillTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.Passive.FieldRepair"));

	FActiveGameplayEffectHandle ApplyFieldRepairEffect(UAbilitySystemComponent* AbilitySystemComponent, const FSkillGameplayEffectData& SkillGameplayEffectData)
	{
		if (::IsValid(AbilitySystemComponent) == false || SkillGameplayEffectData.GameplayEffect.IsNull())
		{
			return FActiveGameplayEffectHandle();
		}

		UClass* LoadedEffectClass = SkillGameplayEffectData.GameplayEffect.LoadSynchronous();
		const TSubclassOf<UGameplayEffect> EffectClass = LoadedEffectClass;
		if (::IsValid(EffectClass) == false)
		{
			return FActiveGameplayEffectHandle();
		}

		const UGameplayEffect* GameplayEffect = EffectClass.GetDefaultObject();
		if (::IsValid(GameplayEffect) == false || GameplayEffect->DurationPolicy != EGameplayEffectDurationType::Infinite)
		{
			UE_LOG(LogTemp, Error, TEXT("[SkillDebug][FieldRepair] Effect must use Infinite policy so it can be removed: %s"),
				*SkillGameplayEffectData.GameplayEffect.ToString());
			return FActiveGameplayEffectHandle();
		}

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, AbilitySystemComponent->MakeEffectContext());
		if (SpecHandle.IsValid() == false)
		{
			return FActiveGameplayEffectHandle();
		}

		for (const FSkillSetByCallerData& SetByCallerData : SkillGameplayEffectData.SetByCallerDataList)
		{
			if (SetByCallerData.SetByCallerTag.IsValid() == false)
			{
				continue;
			}

			SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerData.SetByCallerTag, SetByCallerData.SetByCallerValue);
		}

		return AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

UGA_FieldRepair::UGA_FieldRepair()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FieldRepairSkillTag);
	SetAssetTags(AssetTags);
}

void UGA_FieldRepair::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	WaitForNextRepair();
}

void UGA_FieldRepair::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (RepairDelayTask.IsValid())
	{
		RepairDelayTask->EndTask();
		RepairDelayTask = nullptr;
	}

	ClearBuffEffects();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_FieldRepair::WaitForNextRepair()
{
	if (RepairInterval <= 0.f)
	{
		return;
	}

	if (RepairDelayTask.IsValid())
	{
		RepairDelayTask->EndTask();
	}

	UAbilityTask_WaitDelay* NewDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, RepairInterval);
	RepairDelayTask = NewDelayTask;
	RepairDelayTask->OnFinish.AddDynamic(this, &UGA_FieldRepair::OnRepairIntervalFinished);
	RepairDelayTask->ReadyForActivation();

	UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][FieldRepair] Next activation in %.2f seconds"), RepairInterval);
}

void UGA_FieldRepair::WaitForBuffEnd()
{
	if (BuffDuration <= 0.f || ActiveBuffEffectHandleList.IsEmpty())
	{
		ClearBuffEffects();
		return;
	}

	if (BuffDelayTask.IsValid())
	{
		BuffDelayTask->EndTask();
	}

	UAbilityTask_WaitDelay* NewDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, BuffDuration);
	BuffDelayTask = NewDelayTask;
	BuffDelayTask->OnFinish.AddDynamic(this, &UGA_FieldRepair::OnBuffDurationFinished);
	BuffDelayTask->ReadyForActivation();
}

void UGA_FieldRepair::ClearBuffEffects()
{
	if (BuffDelayTask.IsValid())
	{
		BuffDelayTask->EndTask();
		BuffDelayTask = nullptr;
	}

	if (UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent())
	{
		for (const FActiveGameplayEffectHandle& ActiveBuffEffectHandle : ActiveBuffEffectHandleList)
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveBuffEffectHandle);
		}
	}

	if (ActiveBuffEffectHandleList.IsEmpty() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][FieldRepair] Off EffectCount=%d"), ActiveBuffEffectHandleList.Num());
	}

	ActiveBuffEffectHandleList.Reset();
}

void UGA_FieldRepair::OnRepairIntervalFinished()
{
	RepairDelayTask = nullptr;
	ClearBuffEffects();

	if (UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent())
	{
		for (const FSkillGameplayEffectData& BuffEffectData : BuffEffectList)
		{
			const FActiveGameplayEffectHandle ActiveBuffEffectHandle = ApplyFieldRepairEffect(AbilitySystemComponent, BuffEffectData);
			if (ActiveBuffEffectHandle.IsValid())
			{
				ActiveBuffEffectHandleList.Add(ActiveBuffEffectHandle);
				UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][FieldRepair] On Effect=%s CancelAfter=%.2f seconds"),
					*BuffEffectData.GameplayEffect.ToString(),
					BuffDuration);
			}
		}
	}

	WaitForBuffEnd();
	WaitForNextRepair();
}

void UGA_FieldRepair::OnBuffDurationFinished()
{
	BuffDelayTask = nullptr;
	ClearBuffEffects();
}
