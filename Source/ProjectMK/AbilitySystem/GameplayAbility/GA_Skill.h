#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"

#include "GA_Skill.generated.h"

class UAbilitySystemComponent;
class UAttributeSet_Character;
class USkillComponent;
class AMKCharacter;

UCLASS()
class PROJECTMK_API UGA_Skill : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Skill();

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	const UAttributeSet_Character* GetOwnerCharacterAttributeSet() const;
	USkillComponent* GetOwnerSkillComponent() const;
	AMKCharacter* GetOwnerCharacter() const;

	bool TryRollChance(float Chance) const;

private:
	void TryActivatePassiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;
};
