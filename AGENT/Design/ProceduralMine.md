# 절차형 채굴 월드

## 지형 설정

`/Game/Data/DataAsset/DA_GameSetting`의 `MineableLevelGeneration`에서 조정한다. 전경 타일맵의 크기·타일·레이어는 생성 입력으로 사용하지 않는다. 기존 배경 타일맵은 시각 배경으로만 남는다.

| 설정 | 적용값 / 의미 |
|---|---|
| WidthInBlocks / DepthInBlocks | 128 / 128, 플레이 가능한 채굴 칸 수 |
| StartColumn | 0, 왼쪽 시작 열 |
| OuterRockThickness | 4, 양쪽과 바닥의 채굴 불가 Rock 두께 |
| ShopColumn | 70, 지표면의 상점 기준 열 |
| SurfaceBlockKey | Dirt_Grass, 지표면 한 줄 |
| DepthLayers | Dirt 0칸, Sand 32칸, Ice 72칸 |
| TransitionDepth | 경계 전후 총 12칸에서 인접 지층을 섞음 |

한 칸은 기존 `BLOCK_SIZE=16` 월드 단위다. 깊이는 `SurfaceBlockPositionY`부터 센다. 월드 좌표 중심은 `(열+0.5)×16, -20, -(행+0.5)×16`이다. 양쪽과 바닥의 바깥 경계는 Rock으로 닫는다. 크기 변경 후 재생성하면 반영된다.

지층 혼합은 시드로 결정한 작은 덩어리와 완만한 경계 높이 변화를 함께 사용한다. 같은 시드는 같은 배치를 만들며, 새 블록 그림은 만들지 않고 기존 흙·모래·얼음을 사용한다. Ice의 기존 중복 타일 번호는 19로 분리했다. 지표면 두 줄은 무작위 빈 공간·돌 배치에서 보호한다.

## 목적이 있는 구조물

활성 생성 종류는 세 가지다. 각 종류를 번갈아 시도하고, 배치 실패 시 크기를 줄여 다시 시도한다. 기존 나머지 모양은 생성 후보에서 제외한다.

| 종류 | 플레이 목표 | 보상 / 선택 |
|---|---|---|
| OrePuzzle / OreVault | 지지대 하나에 매달린 광물 덩어리 | 광물을 먼저 캐면 보상을 얻고, 먼저 떨어뜨리면 충격으로 보상을 잃음 |
| Shortcut / HarvestSlab | 양쪽 지지를 끊어 넓은 판을 제거 | 광물을 먼저 캐거나, 떨어뜨려 빠르게 아래 통로를 엶 |
| CombatTrap / BaitTrap | 몬스터가 있는 방의 천장을 지지대 채굴로 떨어뜨림 | 기둥의 광물과 낙하 공격 기회, 천장이 플레이어에게도 위험 |

세 종류를 각각 설정에서 켜고 끌 수 있다. `StructureMonsterRowKey`는 몬스터 DataTable 행을 사용한다. 보상은 기존 `StructureRewardItemDataList`의 가중치를 따른다. `GetGeneratedStructures()`는 목적·보상 칸·목표 칸·몬스터 칸을 제공한다.

낙하 블록은 몬스터에게 충격 대미지를 주며, 살아남은 캐릭터의 머리에 닿으면 그 접촉 위치에서 정착한다. 플레이어가 빠져나와도 낙하를 다시 시작하지 않는다. 긴 낙하에서 블록 자체가 충격으로 깨지는 기존 규칙은 유지한다.

## 표시와 검증

지하 매장 상자와 기존 구조물 패턴의 확정 등장 설정은 [StorageChest.md](StorageChest.md)를 따른다. 기본값은 매장 상자 후보당 0.06%, 최대 16개이며 OreVault·HarvestSlab·BaitTrap을 각각 최소 한 번 먼저 배치한다. 매장 상자는 확률 콘텐츠이고, 지정 패턴의 등장은 별도로 보장한다. 유물 보상은 기획 단계이며 아직 구조물 보상에 연결하지 않았다.

블록 액터의 불필요한 Tick을 끄고, 플레이어 주변만 표시한다. 멈춘 균열·파괴 플립북의 Tick도 끄며 파괴 재생 시에만 켠다. 128×128 기본 시드에서는 16,834개 블록의 33,668개 플립북이 쉬는 동안 모두 Tick 비활성임을 확인했다. 확인 환경의 안정된 플레이 프레임 간격은 약 14~15ms였다. 초기 PIE 생성은 약 6초로, 로딩 비용은 별개다.

전체 지형 데이터·충돌·미니맵 조회는 유지한다. 폭·깊이를 크게 늘리면 액터 생성 수와 초기 생성 시간이 함께 증가하므로, 청크 스트리밍 구현 전에는 최대값을 실제 운용 크기로 가정하지 않는다.

회귀 테스트: `ProjectMK.PIE.World.ProceduralDepthAndRewardEncounters`, `FallingBlockStaysAtHeadContact`, `OxygenWispFlight`. 지형 테스트는 전경 타일맵을 삭제한 PIE에서 경계·각 지층·혼합·시드 재현·구조물 보상·몬스터 생성을 검증한다. 테스트는 저장 방지 세션에서 실행한다.

## 고정 지상 높낮이와 광산 장식

지상 높낮이는 지하 생성 시드와 무관한 고정 프로필이다. MineableLevelGeneration.SurfaceRises의 StartColumnOffset은 StartColumn 기준이며 WidthInBlocks와 HeightInBlocks로 이어진 언덕을 조절한다. 기본값은 (8,18,2), (34,12,1), (88,28,2)이고 SurfaceReliefHeight=2가 최대 높이를 제한한다. 3칸씩 이어진 어깨와 넓은 단차를 만들며 한 칸짜리 돌출은 제거한다. 시작 지점부터 상점까지와 양쪽 6칸은 평지로 유지한다. 각 열의 잔디 아래는 빈틈 없이 채운다.

장식은 VisualStyleDataAsset.MineDecorations의 스프라이트를 사용한다. bGenerateDecorations, SurfaceDecorationSpacing, UndergroundDecorationCount로 배치를 조절한다. 광산 입구·광차·등불·도구대·레일은 충돌 없이 실제 지지 블록에 부착된다. SupplyCrates 배치 지점은 장식 더미 대신 실제 보관 상자를 생성한다. 캠프에는 고정 장식을, 나머지 지상과 보상 방에는 드문 장식을 배치한다. 원본과 재적용 절차는 [MINE_DECOR_V1.md](../Art/MINE_DECOR_V1.md)를 따른다.

SurfaceReliefAndMineDecoration 테스트는 서로 다른 지하 시드에서 지상 높이가 동일한지, 높이별 연결과 잔디·하부 채움, 장식의 지지·무충돌·픽셀 밀도를 확인한다.

## 보호 기반암·광맥·얼음

- 고정으로 솟은 지상 지형의 윗부분부터 기준 지표 아래 한 칸까지 `SurfaceFoundation` 돌을 채운다. 평평한 지상 시설·장식과 상점의 바닥은 한 줄만 같은 돌을 사용한다. 고정으로 솟은 큰 지형의 여러 칸 돌은 유지한다. 지상 돌의 맨 윗면은 마스크로 깎지 않아 실제 바닥 높이까지 채운다.
- `SurfaceFoundation`(TileIndex 901)은 기존 돌 그림을 사용하지만 채굴 불가·지지 앵커·폭발 면역으로 구분한다. 지하 장식 바닥에는 이 보호를 적용하지 않는다. 나머지 지면은 기존 흙·잔디다.
- 개별 무작위 광물은 유지하면서 `OreVeinsPerThousandTiles=0.6` 빈도로 연결 광맥을 추가한다. `OreVeinSizeRange=4~8`, `OreVeinShapes`에서 Branch·Cluster·Crescent를 선택한다. 곡선 패턴은 7칸 고정 형태를 회전한다.
- 광맥은 각 시작 블록의 기존 광물 가중치로 한 종류를 선택하고 실제 연결된 블록에 기존 광물 오버레이와 보상을 적용한다. 빈 공간·구조물 예약 영역·기존 구조물 보상·지표 바로 밑은 피한다. 시드가 같으면 위치·형태·종류·수량이 같다.
- Ice 블록을 밟으면 마찰·제동·가속이 낮아져 미끄러진다. 얼음에서 벗어나거나 공중 상태가 되면 진입 전 이동 설정으로 복구한다. `IceGroundFriction=0.2`, `IceBrakingDeceleration=35`, `IceAcceleration=320`은 GameSettingDataAsset에서 조절한다.

## 산소 보상 표시

산소는 아이템 아이콘 대신 작은 하늘색 기포로 보인다. `M_OxygenWispCore`는 16단계 픽셀 좌표로 청록 테두리·반투명 내부·작은 계단형 반사를 그리며 두 개의 짧아지는 잔상이 뒤따른다. 기존 Pickup 입자는 작은 보조 효과로만 쓴다. 전경 블록 앞에 표시하며 금색 원본 텍스처의 RGB는 사용하지 않는다. 새로운 산소 비트맵은 만들지 않는다.

`DA_GameSetting`의 `OxygenPickupArcHeight=14`, `OxygenPickupDisplaySize=8`로 조절한다. 동전과 같은 흡인 속도·가속을 사용하고 플레이어로 향하는 방향에 짧은 위쪽 곡선을 더한다. 거리가 가까워질수록 곡선이 사라진다. 이전 고정 시간·64단위 포물선은 제거했다. 일반 아이템의 떠오르기·동전 크기·드롭 소유권 규칙은 유지한다.

## 새 아이템 표시

플레이어 소지품의 `FInventorySlotData.bIsNew`는 처음 획득한 아이템 종류에만 설정한다. 이미 획득했던 아이템의 추가 수집·상자 이동·재구매에는 새 표시를 붙이지 않는다. 공통 슬롯 위젯의 우상단 빨간 점으로 표시하고 첫 마우스 호버에 같은 종류의 미확인 표시를 함께 해제한다. 획득 종류 목록을 저장하고, 기존 저장의 보유 아이템은 이미 아는 종류로 이관한다. HUD·팝업은 같은 데이터를 표시하므로 동시에 갱신된다. 자기 소지품 안에서 이동할 때에는 미확인 상태를 유지한다. 상자 자체 슬롯에는 빨간 점을 표시하지 않는다. 읽음 상태는 전용 슬롯 이벤트로 전달해 호버만으로 상점 구매 목록과 레시피 엔트리를 재생성하지 않는다.

## 적용 검증 기록

2026-11-09 KST 기준 Editor Development 빌드와 다음 8개 PIE 테스트를 통과했다. 상세 시각과 저장 비교는 `output/world-pass-validation.json`에 보관한다.

- `Inventory.SeparateQuickSlotsAndWorldDrop`: 분리된 보관함 이동·수량 보존·드롭 소유권·점멸·소멸
- `Monster.SequentialSingleBlockDrilling`: 몬스터의 근접 단일 칸 순차 채굴
- `Shop.GroundDrillingAndInteraction`: 상점 안에서 실제 하향 드릴 입력과 상호작용
- `UI.DragSaleAndSkillHold`: 판매·길게 구매·캡처 해제·필요 선행 스킬 이름
- `UI.PopupScrollAndPassiveClassification`: 여러 프레임에 걸친 스크롤·실제 선행 경로·화살표 레이어·구매와 드래그 전환
- `World.FallingBlockStaysAtHeadContact`: 머리에 닿은 뒤 이동을 재개하지 않는 낙하 블록
- `World.OxygenWispFlight`: 아이콘 대신 중심광·애니메이션·세 잔상, 큰 곡선 이동과 수집
- `World.ProceduralDepthAndRewardEncounters`: 타일맵 삭제 후 지형 생성·경계·지층 혼합·시드 재현·보상과 몬스터 배치

Tick 최적화 후 숨겨진 균열 플립북의 0번 프레임이 초기화되지 않으면 첫 드릴 때 Paper2D 렌더 갱신에서 assert가 발생했다. 생성 시 0초 프레임 계산을 한 번 수행하고 쉬는 동안 Tick을 끄는 방식으로 수정했으며 상점의 실제 드릴 테스트를 다시 통과했다.

`Saved/AgentLogs/ui-20261109-020945.png`에서 실제 기술 트리 경로와 화살표를, `oxygen-flight-live.png`에서 산소 중심광과 궤적을 확인했다. 저장 검증은 엔진 헤더 다음의 게임 데이터로 비교했다. 수동 확인 중 얻은 재화는 시작 시 백업으로 되돌렸으며, 기본 저장과 백업 저장 모두 작업 시작 시 데이터와 일치한다.

2026-11-15 변경 이후 검증 결과는 `output/mine-polish-20261115-validation.json`을 따른다. 위 11월 9일 산소·상점 바닥 기록은 당시 설정의 검증이며, 현재 산소 크기·보호 기반암 규칙은 이 문서의 최신 항목을 기준으로 한다.

장식 RailSegment는 자동 배치 후보에서 제거했다. 향후 실제 철도 시스템과 겹치는 장식 철도는 생성하지 않는다.
