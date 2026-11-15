#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectMK/Data/MineTownData.h"
#include "ForestrySubsystem.generated.h"

class ABlockBase;

struct FManagedTree
{
	int32 Column = 0;
	int64 RegrowAt = 0;
	TArray<TWeakObjectPtr<ABlockBase>> Pieces;
};

UCLASS()
class PROJECTMK_API UForestrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Deinitialize() override;
	void GenerateTrees();
	void ClearTrees();
	void RestoreTrees(const TArray<FTreeSaveData>& Saved);
	TArray<FTreeSaveData> GetSaveData() const;
	const TArray<FManagedTree>& GetTrees() const;
private:
	bool TryGrowTree(FManagedTree& Tree);
	void OnGrowthTimer();
	TArray<FManagedTree> Trees;
	FTimerHandle GrowthTimer;
};
