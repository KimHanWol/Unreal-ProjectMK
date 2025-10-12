// LINK

#include "ProjectMK/Data/DataAsset/GameplayEffectDataAsset.h"

#include "GameplayEffect.h"

TSubclassOf<UGameplayEffect> UGameplayEffectDataAsset::GetGameplayEffect(EGameplayEffectType EffectType)
{
	TSubclassOf<UGameplayEffect>* TargetGameplayEffect = GameplayEffectList.Find(EffectType);
	if (TargetGameplayEffect)
	{
		return *TargetGameplayEffect;
	}

	return nullptr;
}