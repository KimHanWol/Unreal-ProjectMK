// LINK

#include "ProjectMK/System/SkillDebugUtils.h"

namespace
{
	bool bSkillDebugEnabled = false;
}

bool FSkillDebugUtils::IsSkillDebugEnabled()
{
	return bSkillDebugEnabled;
}

void FSkillDebugUtils::SetSkillDebugEnabled(bool bEnabled)
{
	bSkillDebugEnabled = bEnabled;
}

bool FSkillDebugUtils::ToggleSkillDebugEnabled()
{
	bSkillDebugEnabled = !bSkillDebugEnabled;
	return bSkillDebugEnabled;
}
