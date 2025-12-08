// LINK

#include "ProjectMK/Core/Manager/SoundManager.h"

#include "Components/AudioComponent.h"
#include "DataManager.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Core/MKGameInstance.h"
#include "ProjectMK/Data/DataTable/SoundDataTableRow.h"


void USoundManager::Init()
{
    TWeakObjectPtr<USoundManager> WeakThis = this;
    GetWorld()->GetTimerManager().SetTimer(ContinuousParamCheckHandle, [WeakThis]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->UpdateContinuousParam();
            }
        }, 0.01f, true);
}

USoundManager* USoundManager::Get(UObject* WorldContextObject)
{
    UMKGameInstance* GameInstance = Cast<UMKGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
    if (::IsValid(GameInstance))
    {
        return GameInstance->GetSoundManager();
    }

    return nullptr; 
}

void USoundManager::PlaySFX(ESFXType SFXType)
{
    if (CheckIntialParams(SFXType) == false)
    {
        return;
    }

    PlaySFX_Internal(SFXType);
}

void USoundManager::SetFloatSoundParam(ESFXType SFXType, ESFXInstanceParamType ParamType, float FloatParam)
{
    InstantSFXParamMap.FindOrAdd(SFXType).ParamDataMap.FindOrAdd(ParamType) = FString::SanitizeFloat(FloatParam);
}

void USoundManager::SetBoolSoundParam(ESFXType SFXType, ESFXInstanceParamType ParamType, bool BoolParam)
{
    FString BoolString = BoolParam ? TEXT("true") : TEXT("false");
    InstantSFXParamMap.FindOrAdd(SFXType).ParamDataMap.FindOrAdd(ParamType) = BoolString;
}

void USoundManager::SetInt32SoundParam(ESFXType SFXType, ESFXInstanceParamType ParamType, int32 Int32Param)
{
    InstantSFXParamMap.FindOrAdd(SFXType).ParamDataMap.FindOrAdd(ParamType) = FString::FromInt(Int32Param);
}

void USoundManager::PlaySFX_Internal(ESFXType SFXType)
{
    UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        return;
    }

    const FSoundDataTableRow* SoundDataTableRow = DataManager->GetSoundDataTableRow(SFXType);
    if (SoundDataTableRow == nullptr)
    {
        return;
    }

    UAudioComponent* AudioComp = UGameplayStatics::SpawnSound2D(this, SoundDataTableRow->SoundSource.Get());
    SetInstantParams(AudioComp, SFXType);

    PlayingSFXList.Add(TPair<ESFXType, TWeakObjectPtr<UAudioComponent>>(SFXType, AudioComp));
    InstantSFXParamMap.Remove(SFXType);
}

void USoundManager::SetInstantParams(UAudioComponent* AudioComp, ESFXType SFXType)
{
    if (::IsValid(AudioComp) == false)
    {
        return;
    }

    UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        return;
    }

    const FSoundDataTableRow* SoundDataTableRow = DataManager->GetSoundDataTableRow(SFXType);
    if (SoundDataTableRow == nullptr)
    {
        return;
    }

    for (const auto& InitialParamType : SoundDataTableRow->InitialParams)
    {
        SetInstantParam_Internal(AudioComp, SFXType, InitialParamType);
    }
}

void USoundManager::SetInstantParam_Internal(UAudioComponent* AudioComp, ESFXType SFXType, ESFXInstanceParamType ParamType)
{
    const auto* InstanceSFXParamDataPtr = InstantSFXParamMap.Find(SFXType);
    if (InstanceSFXParamDataPtr == nullptr)
    {
        return;
    }

    const FString* ParamStringPtr = InstanceSFXParamDataPtr->ParamDataMap.Find(ParamType);
    if (ParamStringPtr == nullptr)
    {
        return;
    }

    ESoundParamType SoundParamType;
    switch (ParamType)
    {
    case ESFXInstanceParamType::DestroyedBlockCount:
        SoundParamType = ESoundParamType::Float;
        break;
    default:
        ensureAlwaysMsgf(false, TEXT("Initial param type must be added."));
        break;
    }

    switch (SoundParamType)
    {
    case ESoundParamType::Float:
        AudioComp->SetFloatParameter(GetParamKeyFromType(ParamType), FCString::Atoi(**ParamStringPtr));
        break;
    case ESoundParamType::Bool:
        AudioComp->SetBoolParameter(GetParamKeyFromType(ParamType), (*ParamStringPtr) == TEXT("true") ? true : false);
        break;
    default:
        ensure(false);
        break;
    }
}

void USoundManager::SetContinuousParams(UAudioComponent* AudioComp, ESFXType SFXType)
{
    if (::IsValid(AudioComp) == false)
    {
        return;
    }

    UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        return;
    }

    const FSoundDataTableRow* SoundDataTableRow = DataManager->GetSoundDataTableRow(SFXType);
    if (SoundDataTableRow == nullptr)
    {
        return;
    }

    for (const auto& ContinuousParamType : SoundDataTableRow->ContinuousParams)
    {
        SetContinuousParam_Internal(AudioComp, ContinuousParamType);
    }
}

void USoundManager::SetContinuousParam_Internal(UAudioComponent* AudioComp, ESFXContinuousParamType ParamType)
{
    switch (ParamType)
    {
    case ESFXContinuousParamType::Drill:
        AudioComp->SetFloatParameter(GetParamKeyFromType(ParamType), bIsDrilling);
        break;
    case ESFXContinuousParamType::Flying:
        AudioComp->SetFloatParameter(GetParamKeyFromType(ParamType), bIsFlying);
        break;
    case ESFXContinuousParamType::HP:
        AudioComp->SetFloatParameter(GetParamKeyFromType(ParamType), HP);
        break;
    default:
        ensureAlwaysMsgf(false, TEXT("Continuous param type must be added."));
        break;
    }
}

FName USoundManager::GetParamKeyFromType(ESFXInstanceParamType ParamType) const
{
    switch (ParamType)
    {
    case ESFXInstanceParamType::DestroyedBlockCount:
        return TEXT("DestroyedBlockCount");
        break;
    default:
        ensureMsgf(false, TEXT("The key name has to be added before use param."));
        break;
    }

    return TEXT("");
}

FName USoundManager::GetParamKeyFromType(ESFXContinuousParamType ParamType) const
{
    switch (ParamType)
    {
    case ESFXContinuousParamType::HP:
        return TEXT("HP");
        break;
    case ESFXContinuousParamType::Flying:
        return TEXT("Flying");
        break;
    case ESFXContinuousParamType::Drill:
        return TEXT("Drill");
        break;
    default:
        ensureMsgf(false, TEXT("The key name has to be added before use param."));
        break;
    }

    return TEXT("");
}

bool USoundManager::CheckIntialParams(ESFXType SFXType)
{
    UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        return true;
    }

    const FSoundDataTableRow* SoundDataTableRow = DataManager->GetSoundDataTableRow(SFXType);
    if (SoundDataTableRow == nullptr)
    {
        return true;
    }

    bool bIsInitialParamSet = true;
    for (const auto& InitialParam : SoundDataTableRow->InitialParams)
    {
        const auto& InstantSFXParamDataPtr = InstantSFXParamMap.Find(SFXType);
        if (InstantSFXParamDataPtr == nullptr)
        {
            bIsInitialParamSet = false;
            break;
        }

        const auto& InitialParamStringPtr = (*InstantSFXParamDataPtr).ParamDataMap.Find(InitialParam);
        if (InitialParamStringPtr == nullptr)
        {
            bIsInitialParamSet = false;
            break;
        }
    }

    if (bIsInitialParamSet == false)
    {
        ensureAlwaysMsgf(false, TEXT("The initial param must be set before play sfx. It will be ignored."));
    }

    return bIsInitialParamSet;
}

void USoundManager::UpdateContinuousParam()
{
    PlayingSFXList.RemoveAll([](const TPair<ESFXType, TWeakObjectPtr<UAudioComponent>>& SFXData)
        {
            return !SFXData.Value.IsValid();
        });

    for (const auto& PlayingSFX : PlayingSFXList)
    {
        if (PlayingSFX.Value.IsValid() == false)
        {
            continue;
        }

        SetContinuousParams(PlayingSFX.Value.Get(), PlayingSFX.Key);
    }
}