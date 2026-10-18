#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SkillComponent.generated.h"

class UAbilitySystemComponent;
class UAttributeSet_Character;
class UGameplayAbility;
class UGameplayEffect;
struct FSkillDataTableRow;
struct FSkillGameplayEffectData;
struct FSkillGameplayAbilityData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTMK_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSkillPurchased, FName);

	bool HasPurchasedSkill(FName SkillKeyName) const;
	bool CanPurchaseSkill(FName SkillKeyName) const;
	bool TryPurchaseSkill(FName SkillKeyName);
	void ApplySavedSkills(const TArray<FName>& SavedSkillKeys);
	bool TryAddCoin(int32 CoinAmount, bool bApplyCoinGainMultiplier = true);
	bool TryAddOxygen(int32 OxygenAmount);
	TArray<FName> GetPurchasedSkillKeys() const;

	FOnSkillPurchased OnSkillPurchasedDelegate;

private:
	bool TryApplySkill(UAbilitySystemComponent* AbilitySystemComponent, FName SkillKeyName, const FSkillDataTableRow& SkillDataTableRow) const;
	bool TryApplySkillGameplayEffect(UAbilitySystemComponent* AbilitySystemComponent, const FSkillGameplayEffectData& SkillGameplayEffectData) const;
	bool TryGrantSkillAbility(UAbilitySystemComponent* AbilitySystemComponent, const FSkillGameplayAbilityData& SkillGameplayAbilityData) const;
	int32 CalculateAdjustedCoinAmount(int32 CoinAmount) const;
	bool AreRequiredSkillsPurchased(const TArray<FName>& RequiredSkillKeys) const;
	bool CanPurchaseSkill(FName SkillKeyName, const FSkillDataTableRow& SkillDataTableRow) const;
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	const UAttributeSet_Character* GetCharacterAttributeSet() const;

private:
	UPROPERTY(Transient)
	TSet<FName> PurchasedSkillKeySet;
};
