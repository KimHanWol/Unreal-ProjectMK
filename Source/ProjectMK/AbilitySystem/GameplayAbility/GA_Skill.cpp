// LINK

#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Skill.h"

#include "AbilitySystemComponent.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/System/SkillDebugUtils.h"

UGA_Skill::UGA_Skill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_Skill::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	TryActivatePassiveAbility(ActorInfo, Spec);
}

void UGA_Skill::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	TryActivatePassiveAbility(ActorInfo, Spec);
}

UAbilitySystemComponent* UGA_Skill::GetOwnerAbilitySystemComponent() const
{
	return GetAbilitySystemComponentFromActorInfo();
}

const UAttributeSet_Character* UGA_Skill::GetOwnerCharacterAttributeSet() const
{
	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return nullptr;
	}

	return Cast<UAttributeSet_Character>(AbilitySystemComponent->GetAttributeSet(UAttributeSet_Character::StaticClass()));
}

USkillComponent* UGA_Skill::GetOwnerSkillComponent() const
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	return ::IsValid(OwnerCharacter) ? OwnerCharacter->GetSkillComponent() : nullptr;
}

AMKCharacter* UGA_Skill::GetOwnerCharacter() const
{
	return Cast<AMKCharacter>(GetAvatarActorFromActorInfo());
}

bool UGA_Skill::TryRollChance(float Chance) const
{
	return FMath::FRand() <= FMath::Clamp(Chance, 0.f, 1.f);
}

void UGA_Skill::TryActivatePassiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const
{
	if (ActorInfo == nullptr || ActorInfo->AbilitySystemComponent.IsValid() == false || Spec.IsActive())
	{
		return;
	}

	const bool bActivated = ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
	MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][Passive] Try activate %s Result=%s"), *GetName(), bActivated ? TEXT("true") : TEXT("false"));
}
