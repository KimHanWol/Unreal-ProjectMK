#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ProjectMK/Data/Struct/CharacterAnimationTextureSet.h"

#include "CharacterDataTableRow.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FCharacterInitialInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetItemAndEquipmentRowNames"))
	FName ItemKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 ItemCount = 1;

	FName GetItemKey() const
	{
		return ItemKey;
	}
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FCharacterAnimationTextureSet AnimationTextures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FCharacterInitialInventoryEntry> InitialInventoryItems;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FCharacterInitialInventoryEntry> InitialInventoryItems;

	FCharacterDataTableRow ToCharacterDataTableRow() const
	{
		FCharacterDataTableRow ConvertedRow;
		ConvertedRow.CharacterName = CharacterName;
		ConvertedRow.InitialInventoryItems = InitialInventoryItems;
		for (const FCharacterAnimationEntry& AnimationEntry : AnimationEntries)
		{
			ConvertedRow.AnimationTextures.SetTexture(AnimationEntry.AnimationType, AnimationEntry.AnimationAtlasTexture);
		}
		return ConvertedRow;
	}
};
