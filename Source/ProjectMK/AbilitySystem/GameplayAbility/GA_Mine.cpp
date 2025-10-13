// LINK

#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Mine.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameplayEffectDataAsset.h"

void UGA_Mine::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        return;
    }

	TSubclassOf<UGameplayEffect> MineEffectClass = DataManager->GetGameplayEffect(EGameplayEffectType::Duration_Add);
	if (::IsValid(MineEffectClass) == false)
	{
		return;
	}

    if (ActorInfo->AvatarActor.IsValid() == false)
    {
        return;
    }

    UAbilitySystemComponent* SourceASC = ActorInfo->AvatarActor->GetComponentByClass<UAbilitySystemComponent>();
    if (::IsValid(SourceASC) == false)
    {
        return;
    }

    if (::IsValid(TriggerEventData->Target) == false)
    {
        return;
    }

    UAbilitySystemComponent* TargetASC = TriggerEventData->Target->GetComponentByClass<UAbilitySystemComponent>();
    if (::IsValid(TargetASC) == false)
    {
        return;
    }

    FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
    Context.AddSourceObject(this);

    FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(MineEffectClass, 1.f);
    SpecHandle.Data->SetContext(Context);

    // �ϴ� ������� ĳ������ ���ݷ¿� ���� �޶������� ��
    // ���߿� ������ ���������� DamageCalc �� �ű���
    SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("SetByCaller.Common.Value")), -100.0f);

    SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void UGA_Mine::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
