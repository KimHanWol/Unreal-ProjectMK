#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SkillComponent.generated.h"

class UAbilitySystemComponent;
class UAttributeSet_Character;
class UGameplayEffect;
struct FSkillDataTableRow;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTMK_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	bool HasPurchasedSkill(FName SkillKeyName) const;
	bool CanPurchaseSkill(FName SkillKeyName) const;
	bool TryPurchaseSkill(FName SkillKeyName);

private:
	bool AreRequiredSkillsPurchased(const TArray<FName>& RequiredSkillKeys) const;
	bool CanPurchaseSkill(FName SkillKeyName, const FSkillDataTableRow& SkillDataTableRow) const;
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	const UAttributeSet_Character* GetCharacterAttributeSet() const;

private:
	UPROPERTY(Transient)
	TSet<FName> PurchasedSkillKeySet;
};
