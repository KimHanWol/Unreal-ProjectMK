#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ProjectMK/Data/Struct/CharacterAnimationTextureSet.h"

#include "CharacterDataTableRow.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FCharacterAnimationEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECharacterAnimationType AnimationType = ECharacterAnimationType::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> AnimationAtlasTexture;
};

USTRUCT(BlueprintType)
struct FCharacterDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText CharacterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FCharacterAnimationTextureSet AnimationTextures;
};

// Legacy row struct kept for compatibility with existing DataTable assets.
USTRUCT(BlueprintType)
struct FCharacterAnimationDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText CharacterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCharacterAnimationEntry> AnimationEntries;

	FCharacterDataTableRow ToCharacterDataTableRow() const
	{
		FCharacterDataTableRow ConvertedRow;
		ConvertedRow.CharacterName = CharacterName;
		for (const FCharacterAnimationEntry& AnimationEntry : AnimationEntries)
		{
			ConvertedRow.AnimationTextures.SetTexture(AnimationEntry.AnimationType, AnimationEntry.AnimationAtlasTexture);
		}
		return ConvertedRow;
	}
};
