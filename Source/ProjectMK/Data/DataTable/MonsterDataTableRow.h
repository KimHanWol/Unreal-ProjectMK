#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MonsterDataTableRow.generated.h"

class UPaperSprite;

USTRUCT(BlueprintType)
struct FMonsterDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UPaperSprite> Sprite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "1.0"))
	float SpriteSize = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor NormalColor = FLinearColor(0.65f, 1.f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor AngryColor = FLinearColor(1.f, 0.25f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "1.0"))
	float MaxHealth = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wander", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MoveSpeed = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wander", meta = (ClampMin = "0.1", Units = "s"))
	float IdleDurationMin = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wander", meta = (ClampMin = "0.1", Units = "s"))
	float IdleDurationMax = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wander", meta = (ClampMin = "0.1", Units = "s"))
	float MoveDurationMin = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wander", meta = (ClampMin = "0.1", Units = "s"))
	float MoveDurationMax = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wander", meta = (ClampMin = "1"))
	int32 WanderRadiusCells = 4;

	/** 마지막 피격부터 이 시간이 지나면 블록 유무와 채굴 진척에 관계없이 분노를 끝낸다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rage", meta = (ClampMin = "0.1", Units = "s", DisplayName = "Angry Duration"))
	float AngryDuration = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rage", meta = (ClampMin = "0.05", Units = "s"))
	float DrillWindup = 0.35f;

	/** 한 번 분노할 때 한 칸씩 파괴할 블록 수. 기존 DataTable 값을 유지한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rage", meta = (ClampMin = "1", ClampMax = "8", DisplayName = "Drill Count Min"))
	int32 DrillCellsMin = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rage", meta = (ClampMin = "1", ClampMax = "8", DisplayName = "Drill Count Max"))
	int32 DrillCellsMax = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rage", meta = (ClampMin = "0.1"))
	float DrillingPower = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rage", meta = (ClampMin = "0.05", Units = "s"))
	float DrillingPeriod = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contact", meta = (ClampMin = "0.0"))
	float ContactDamage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contact", meta = (ClampMin = "0.0", Units = "cm/s"))
	float ContactKnockbackSpeed = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contact", meta = (ClampMin = "0.0", Units = "cm/s"))
	float ContactKnockbackUpwardSpeed = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contact", meta = (ClampMin = "0.0", Units = "s", DisplayName = "Knockback Recovery Time"))
	float ContactKnockbackDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contact", meta = (ClampMin = "0.1", Units = "s"))
	float ContactDamageInterval = 1.f;
};
