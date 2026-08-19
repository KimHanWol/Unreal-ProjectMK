#pragma once

#include "CoreMinimal.h"

class ABlockBase;
class ULevelManagerSubsystem;

struct FBlockUtils
{
	static TArray<ABlockBase*> GetSurroundBlocks(const ULevelManagerSubsystem* LevelManagerSubsystem, ABlockBase* TargetBlock);
};
