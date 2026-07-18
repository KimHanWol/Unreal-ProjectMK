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

// Legacy row struct kept for compatibility with existing DataTable assets.
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
