// LINK

#include "ProjectMK/Core/MKGameInstance.h"

#include "HAL/IConsoleManager.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Manager/SoundManager.h"

namespace
{
	constexpr int32 LightAntiAliasingMethod = 1;
	constexpr int32 LightPostProcessAAQuality = 2;
	constexpr int32 DisabledQuality = 0;
	constexpr float NativeScreenPercentage = 100.f;

	void SetConsoleVariableInt(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			ConsoleVariable->Set(Value, ECVF_SetByGameSetting);
		}
	}

	void SetConsoleVariableFloat(const TCHAR* Name, float Value)
	{
		if (IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			ConsoleVariable->Set(Value, ECVF_SetByGameSetting);
		}
	}
}

void UMKGameInstance::Init()
{
	Super::Init();

	Apply2DRenderOverrides();

	if (::IsValid(DataManagerClass))
	{
		DataManager = NewObject<UDataManager>(this, DataManagerClass);
	}

	if (::IsValid(SoundManagerClass))
	{
		SoundManager = NewObject<USoundManager>(this, SoundManagerClass);
		SoundManager->Init();
	}
}

UDataManager* UMKGameInstance::GetDataManager()
{
	return DataManager;
}

USoundManager* UMKGameInstance::GetSoundManager()
{
	return SoundManager;
}

void UMKGameInstance::Apply2DRenderOverrides() const
{
	// For this project, a light post AA is a better balance than no AA at all.
	SetConsoleVariableInt(TEXT("r.AntiAliasingMethod"), LightAntiAliasingMethod);
	SetConsoleVariableInt(TEXT("r.PostProcessAAQuality"), LightPostProcessAAQuality);
	SetConsoleVariableInt(TEXT("r.TemporalAA.Upsampling"), DisabledQuality);
	SetConsoleVariableFloat(TEXT("r.ScreenPercentage"), NativeScreenPercentage);
	SetConsoleVariableInt(TEXT("r.MotionBlurQuality"), DisabledQuality);
	SetConsoleVariableInt(TEXT("r.DepthOfFieldQuality"), DisabledQuality);
	SetConsoleVariableInt(TEXT("r.BloomQuality"), DisabledQuality);
	SetConsoleVariableInt(TEXT("r.EyeAdaptationQuality"), DisabledQuality);
}
