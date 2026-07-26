#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "SkillDataTableRow.generated.h"

class UGameplayEffect;
class UTexture2D;

USTRUCT(BlueprintType)
struct FSkillDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> IconPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag SetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SetByCallerValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetSkillRowNames"))
	TArray<FName> RequiredSkill;
};
