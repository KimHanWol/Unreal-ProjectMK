#pragma once

#include "CoreMinimal.h"
#include "Math/Interval.h"
#include "ProjectMK/Data/DataAsset/MKDataAssetBase.h"
#include "ProjectMK/Data/DataTable/BlockDataTableRow.h"

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

	/** 채굴 칸 1000개당 생성할 낙하 구조물 수. 특수 구조물은 가끔 나오는 것이 목표라 낮게 둔다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure", meta = (ClampMin = "0.0"))
	float FallingStructuresPerThousandTiles = 3.f;

	/** 한 지형에 생성할 낙하 구조물 수의 상한 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure", meta = (ClampMin = "0"))
	int32 FallingStructureCount = 12;

	/** 낙하 구조물 너비의 최소·최대 타일 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure")
	FInt32Interval FallingStructureWidthRange = FInt32Interval(7, 16);

	/** 낙하 구조물 높이의 최소·최대 타일 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure")
	FInt32Interval FallingStructureHeightRange = FInt32Interval(3, 7);

	/** 낙하 구조물과 충돌 지형 사이 거리의 최소·최대 타일 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure")
	FInt32Interval FallingDistanceInTilesRange = FInt32Interval(5, 12);

	/** 채굴 영역 맨 윗줄(지표면)에서 이 칸 수만큼은 낙하 구조물을 배치하지 않는다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure", meta = (ClampMin = "0"))
	int32 FallingStructureSurfaceMargin = 4;

	/** 낙하 구조물 한 종류가 배치에 실패했을 때 다른 위치로 다시 시도하는 횟수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure", meta = (ClampMin = "1"))
	int32 FallingStructurePlacementRetryCount = 40;

	/** 보상 블록(미끼, 보물 매달기 등)에 심을 아이템. OreSpawnWeight 는 항목 간 상대 가중치로 쓴다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Structure")
	TArray<FBlockSpawnableItemData> StructureRewardItemDataList;

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

	/** 최대 산소 대비 이 비율 아래로 내려가면 산소 부족 경고음을 재생한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowOxygenWarningRatio = 0.25f;

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

	/** 낙하 블록이 멈춰 있는 블록에 부딪혔을 때 충격 대미지를 전달하는 비율. [0] 직접 맞은 블록, [1] 그 아래, [2] 그 아래 ... 채굴 불가 블록을 만나면 전달을 멈춘다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Falling Block")
	TArray<float> FallingBlockImpactPropagationRatios = { 0.7f, 0.35f, 0.15f };

	/** 한 프레임에 채굴·파괴·착지한 블록 수가 이 값 이상이면 Medium 사운드를 재생한다 (미만이면 Single) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "2"))
	int32 MediumBlockSoundMinCount = 2;

	/** 한 프레임에 채굴·파괴·착지한 블록 수가 이 값 이상이면 Large 사운드를 재생한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "2"))
	int32 LargeBlockSoundMinCount = 6;

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
