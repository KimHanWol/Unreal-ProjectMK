#pragma once

#include "CoreMinimal.h"
#include "Math/Interval.h"
#include "ProjectMK/Data/DataAsset/MKDataAssetBase.h"

#include "GameSettingDataAsset.generated.h"

/** 절차형 채굴 지형 생성 조정값 */
USTRUCT(BlueprintType)
struct FMineableLevelGenerationSettings
{
	GENERATED_BODY()

	/** 전체 채굴 영역에서 비워 둘 블록 비율 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EmptyBlockRatio = 0.15f;

	/** 일반 채굴 블록을 암석 블록으로 교체할 비율 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RockBlockRatio = 0.08f;

	/** 무작위 빈 공간 너비의 최소·최대 타일 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Empty Pocket")
	FInt32Interval EmptyPocketWidthRange = FInt32Interval(2, 5);

	/** 무작위 빈 공간 높이의 최소·최대 타일 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Empty Pocket")
	FInt32Interval EmptyPocketHeightRange = FInt32Interval(1, 2);

	/** 생성할 낙하 구조물 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure", meta = (ClampMin = "0"))
	int32 FallingStructureCount = 5;

	/** 낙하 구조물 너비의 최소·최대 타일 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure")
	FInt32Interval FallingStructureWidthRange = FInt32Interval(7, 16);

	/** 낙하 구조물 높이의 최소·최대 타일 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure")
	FInt32Interval FallingStructureHeightRange = FInt32Interval(3, 7);

	/** 낙하 구조물과 충돌 지형 사이 거리의 최소·최대 타일 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure")
	FInt32Interval FallingDistanceInTilesRange = FInt32Interval(5, 12);

	/** 몬스터 방 생성 확률 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster Room", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MonsterRoomGenerationChance = 0.35f;

	/** 몬스터 방 너비의 최소·최대 타일 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster Room")
	FInt32Interval MonsterRoomWidthRange = FInt32Interval(6, 10);

	/** 몬스터 방 높이 타일 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster Room", meta = (ClampMin = "1"))
	int32 MonsterRoomHeight = 3;
};

UCLASS(BlueprintType)
class PROJECTMK_API UGameSettingDataAsset : public UMKDataAssetBase
{
	GENERATED_BODY()

public:
	/** 산소가 즉시 최대치로 회복되는 지표면 블록 Y 좌표 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen")
	int32 SurfaceBlockPositionY = 0;

	/** 초당 산소 소모량이 한 단계 증가하는 깊이 간격 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "1"))
	int32 DepthPerOxygenLoss = 1;

	/** 산소 감소 Gameplay Effect 적용 주기 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "0.01"))
	float OxygenDrainTickInterval = 0.1f;

	/** 낙하 블록 하나가 1미터 낙하할 때 캐릭터에게 주는 대미지 계수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Block", meta = (ClampMin = "0", DisplayName = "Character Damage Per Falling Block Per Meter"))
	int32 FallingBlockDamagePerBlock = 1;

	/** 최소 충격 속도를 초과한 속도를 충돌한 낙하 블록의 대미지로 변환하는 계수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Block", meta = (ClampMin = "0.0"))
	float FallingBlockImpactDamagePerSpeed = 0.75f;

	/** 낙하 블록이 충격 대미지를 받기 시작하는 속도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Block", meta = (ClampMin = "0.0"))
	float MinimumFallingBlockImpactSpeed = 150.f;

	/** 낙하 블록이 한 번의 충돌로 받을 수 있는 최대 대미지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Block", meta = (ClampMin = "0.0"))
	float MaximumFallingBlockImpactPower = 1200.f;

	/** 절차형 채굴 지형 생성 조정값 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Level Generation")
	FMineableLevelGenerationSettings MineableLevelGeneration;

	/** 블록에 포함된 아이템 오버레이 스프라이트 배율 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0.01"))
	float BlockItemOverlaySpriteScale = 0.7f;

	/** 월드에 드롭된 아이템 스프라이트 배율 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.01"))
	float WorldItemSpriteScale = 0.7f;

	/** 강제 지정된 캐릭터 시각 방향 반전 여부 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	bool bReverseOverrideVisualFacingDirection = false;
};
