#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "CharacterDataTableRow.generated.h"

class UCharacterAnimationDataAsset;

USTRUCT(BlueprintType)
struct FCharacterDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText CharacterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TSoftObjectPtr<UCharacterAnimationDataAsset> AnimationDataAsset;
};

// Legacy row struct kept only so older Character DataTable assets can still load.
// Remove it after all Character DataTables are migrated to FCharacterDataTableRow and resaved.
USTRUCT(BlueprintType)
struct FCharacterAnimationDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText CharacterName;

	FCharacterDataTableRow ToCharacterDataTableRow() const
	{
		FCharacterDataTableRow ConvertedRow;
		ConvertedRow.CharacterName = CharacterName;
		return ConvertedRow;
	}
};
