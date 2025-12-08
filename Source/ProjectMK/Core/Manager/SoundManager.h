//LINK

#pragma once

#include "CoreMinimal.h"

#include "SoundManager.generated.h"

enum class ESFXType : uint8;

USTRUCT(BlueprintType)
struct FInstantSFXParamData
{
    GENERATED_BODY()

    UPROPERTY()
    TMap<ESFXInstanceParamType, FString> ParamDataMap;
};

UENUM(BlueprintType)
enum class ESoundParamType : uint8
{
    Float,
    Bool,
};

UCLASS(Blueprintable)
class PROJECTMK_API USoundManager : public UObject
{
    GENERATED_BODY()

public:
    void Init();
    static USoundManager* Get(UObject* WorldContextObject);

    void PlaySFX(ESFXType SFXType);

    void SetFloatSoundParam(ESFXType SFXType, ESFXInstanceParamType ParamType, float FloatParam);
    void SetBoolSoundParam(ESFXType SFXType, ESFXInstanceParamType ParamType, bool BoolParam);
    void SetInt32SoundParam(ESFXType SFXType, ESFXInstanceParamType ParamType, int32 Int32Param);

private:
    FName GetParamKeyFromType(ESFXInstanceParamType ParamType) const;
    FName GetParamKeyFromType(ESFXContinuousParamType ParamType) const;
    bool CheckIntialParams(ESFXType SFXType);

    void UpdateContinuousParam();

    void PlaySFX_Internal(ESFXType SFXType);

    void SetInstantParams(UAudioComponent* AudioComp, ESFXType SFXType);
    void SetInstantParam_Internal(UAudioComponent* AudioComp, ESFXType SFXType, ESFXInstanceParamType ParamType);

    void SetContinuousParams(UAudioComponent* AudioComp, ESFXType SFXType);
    void SetContinuousParam_Internal(UAudioComponent* AudioComp, ESFXContinuousParamType ParamType);

private:
    TMap<ESFXType, FInstantSFXParamData> InstantSFXParamMap;
    TArray<TPair<ESFXType, TWeakObjectPtr<UAudioComponent>>> PlayingSFXList;

    //Instance Param
    int32* DestroyedBlockCountPtr;

    //Continuous Param
    UPROPERTY(Transient)
    float HP = 0.f;

    UPROPERTY(Transient)
    bool bIsFlying = false;
    
    UPROPERTY(Transient)
    bool bIsDrilling = false;

    FTimerHandle ContinuousParamCheckHandle;
};