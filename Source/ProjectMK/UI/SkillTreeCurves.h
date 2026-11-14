#pragma once

#include "CoreMinimal.h"

struct FSkillTreeLayoutEdge;

/** Fit clear, smooth curves to the dependency routes without changing their ownership. */
namespace SkillTreeCurves
{
	void Apply(const TMap<FName, FVector2D>& NodeCenters, TArray<FSkillTreeLayoutEdge>& Edges);
}
