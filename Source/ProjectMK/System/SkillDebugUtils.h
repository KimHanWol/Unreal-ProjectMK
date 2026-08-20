#pragma once

#include "CoreMinimal.h"

namespace FSkillDebugUtils
{
	PROJECTMK_API bool IsSkillDebugEnabled();
	PROJECTMK_API void SetSkillDebugEnabled(bool bEnabled);
	PROJECTMK_API bool ToggleSkillDebugEnabled();
}

#define MK_SKILL_DEBUG_LOG(Verbosity, Format, ...) \
	do \
	{ \
		if (FSkillDebugUtils::IsSkillDebugEnabled()) \
		{ \
			UE_LOG(LogTemp, Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while (false)
