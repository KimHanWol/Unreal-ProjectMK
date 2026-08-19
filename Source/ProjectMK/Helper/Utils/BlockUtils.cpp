// LINK

#include "ProjectMK/Helper/Utils/BlockUtils.h"

#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"

TArray<ABlockBase*> FBlockUtils::GetSurroundBlocks(const ULevelManagerSubsystem* LevelManagerSubsystem, ABlockBase* TargetBlock)
{
	TArray<ABlockBase*> SurroundBlocks;
	if (::IsValid(LevelManagerSubsystem) == false || ::IsValid(TargetBlock) == false)
	{
		return SurroundBlocks;
	}

	const FVector2D TargetBlockPosition = UMKBlueprintFunctionLibrary::GetBlockPosition(TargetBlock);
	const FVector2D SurroundDirections[] =
	{
		FVector2D(0.f, 1.f),
		FVector2D(0.f, -1.f),
		FVector2D(1.f, 0.f),
		FVector2D(-1.f, 0.f)
	};

	for (const FVector2D& SurroundDirection : SurroundDirections)
	{
		ABlockBase* SurroundBlock = LevelManagerSubsystem->GetBlockAtPosition(TargetBlockPosition + SurroundDirection);
		if (::IsValid(SurroundBlock))
		{
			SurroundBlocks.Add(SurroundBlock);
		}
	}

	return SurroundBlocks;
}
