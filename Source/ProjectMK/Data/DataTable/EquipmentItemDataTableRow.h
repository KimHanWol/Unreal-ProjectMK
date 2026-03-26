#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"

#include "EquipmentItemDataTableRow.generated.h"

class UGameplayEffect;
class UPaperSprite;
class UPaperZDAnimSequence;
class UTexture2D;

UENUM(BlueprintType)
enum class EEuipmentType : uint8
{
	Halmet UMETA(DisplayName = "Armor"),
	Drill UMETA(DisplayName = "Drill"),
	balloon UMETA(DisplayName = "Balloon"),
	Gloves UMETA(DisplayName = "Gloves"),
	shoes UMETA(DisplayName = "Shoes"),
};

USTRUCT(BlueprintType)
struct FEquipmentEffectEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag SetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SetByCallerValue = 0.f;
};

USTRUCT(BlueprintType)
struct FEquipmentAnimationOverlayEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UPaperZDAnimSequence> AnimationSequence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> OverlayAtlasTexture;
};

USTRUCT(BlueprintType)
struct FEquipmentItemDataTableRow : public FItemDataTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText EquipmentName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UPaperSprite> EquipmentSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FEquipmentAnimationOverlayEntry> AnimationOverlayEntries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEuipmentType EquipmentType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EquipmentPrice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FEquipmentEffectEntry> EqiupEffectClasses;
};
