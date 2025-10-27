//LINK

#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Data/DataAsset/MKDataAssetBase.h"
#include "GameplayEffectDataAsset.generated.h"

class UGameplayEffect;

UENUM()
enum class EGameplayEffectType : uint8
{
    None = 0,
    Block_Init,
    Character_Init,
    Duration_Add,
};

enum class EGameplayAbilityType : uint8;

class UGameplayAbility;

UCLASS(BlueprintType)
class PROJECTMK_API UGameplayEffectDataAsset : public UMKDataAssetBase
{
    GENERATED_BODY()

public:
    TSubclassOf<UGameplayEffect> GetGameplayEffect(EGameplayEffectType EffectType);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<EGameplayEffectType, TSubclassOf<UGameplayEffect>> GameplayEffectList;
};
