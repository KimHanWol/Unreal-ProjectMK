#pragma once

#include "CoreMinimal.h"

struct FSkillTreeLayoutEdge
{
	FName Source;
	FName Target;
	TArray<FVector2D> Points;
};

/** Prerequisite-driven radial branches with exclusive, obstacle-free connection lanes. */
struct PROJECTMK_API FSkillTreeLayout
{
	static bool Calculate(const TMap<FName, TArray<FName>>& Prerequisites, FSkillTreeLayout& OutLayout);

	TMap<FName, FVector2D> Positions;
	TArray<FSkillTreeLayoutEdge> Edges;
	FBox2D Bounds = FBox2D(ForceInit);
};
