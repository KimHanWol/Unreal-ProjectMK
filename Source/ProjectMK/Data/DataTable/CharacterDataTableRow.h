#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "CharacterDataTableRow.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ECharacterAnimationType : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Run UMETA(DisplayName = "Run"),
	Fly UMETA(DisplayName = "Fly"),
	Fall UMETA(DisplayName = "Fall"),
	Drill_Side UMETA(DisplayName = "Drill_Side"),
	Drill_Up UMETA(DisplayName = "Drill_Up"),
	Drill_Down UMETA(DisplayName = "Drill_Down"),
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCharacterAnimationEntry> AnimationEntries;
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
		ConvertedRow.AnimationEntries = AnimationEntries;
		return ConvertedRow;
	}
};
