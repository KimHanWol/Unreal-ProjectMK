#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "SkillDataTableRow.generated.h"

class UGameplayEffect;
class UGameplayAbility;
class UTexture2D;

USTRUCT(BlueprintType)
struct FSkillSetByCallerData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag SetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SetByCallerValue = 0.f;
};

USTRUCT(BlueprintType)
struct FSkillGameplayEffectData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSkillSetByCallerData> SetByCallerDataList;
};

USTRUCT(BlueprintType)
struct FSkillGameplayAbilityData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<UGameplayAbility> GameplayAbility;
};

UENUM(BlueprintType)
enum class ESkillAcquisitionType : uint8
{
	SkillTree UMETA(DisplayName = "구매 기술"),
	CharacterPassive UMETA(DisplayName = "캐릭터 고유 패시브"),
};

USTRUCT(BlueprintType)
struct FSkillDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	ESkillAcquisitionType AcquisitionType = ESkillAcquisitionType::SkillTree;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> IconPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSkillGameplayEffectData> GameplayEffectList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSkillGameplayAbilityData> GameplayAbilityList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetSkillRowNames"))
	TArray<FName> RequiredSkill;
};
