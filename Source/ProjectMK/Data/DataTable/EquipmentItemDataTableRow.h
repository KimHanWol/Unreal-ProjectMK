#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "ProjectMK/Data/Struct/CharacterAnimationTextureSet.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"

#include "EquipmentItemDataTableRow.generated.h"

class UGameplayEffect;
class UPaperSprite;

UENUM(BlueprintType)
enum class EEuipmentType : uint8
{
	Halmet UMETA(DisplayName = "Helmet"),
	Armor UMETA(DisplayName = "Armor"),
	Drill UMETA(DisplayName = "Drill"),
	Balloon UMETA(DisplayName = "Balloon"),
	Gloves UMETA(DisplayName = "Gloves"),
	Shoes UMETA(DisplayName = "Shoes"),
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
struct FEquipmentItemDataTableRow : public FItemDataTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText EquipmentName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UPaperSprite> EquipmentSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Overlay")
	FCharacterAnimationTextureSet AnimationOverlayTextures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEuipmentType EquipmentType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EquipmentPrice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FEquipmentEffectEntry> EqiupEffectClasses;
};
