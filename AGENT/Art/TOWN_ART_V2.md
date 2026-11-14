# 마을·광산 시설 V2

사용자 요청에 따라 주요 건물을 확대하고 원본 해상도도 함께 높였다. 상자·철도·열차·지지대·캐릭터 자원은 교체하지 않는다.

기존 `COMMON_PIXEL_ART_PROMPT.md`, `WORLD_SHOP_V3.md`와 보급소·주택·시설 V1을 참고해 내장 imagegen으로 6종을 한 장에 생성했다. 생성 원본은 `Content/Asset/TownArtV2/Source/atlas-original.png`에 보존한다.

| 자원 | 네이티브 캔버스 | 설계 |
|---|---|---|
| Elevator | 192×224 | 두꺼운 A자 지지대, 이중 도르래, 케이블, 승강 케이지, 측면 권양기 |
| RailFactory | 288×192 | 큰 개방 작업장, 바퀴·차축·레일 적치, 수레 제작대, 작업용 호이스트 |
| HouseA / HouseB | 각160×128 | 서로 다른 지붕·현관·굴뚝과 낮은 채도의 목재·돌 외벽 |
| VillageBackdrop | 288×160 | 세 채가 겹친 낮은 대비의 배경 주택군 |
| VillageNotice | 96×96 | 글씨 없는 작은 게시판·울타리·작업등 |

모두 2px/월드 단위, Nearest, NoMipmaps, MaskedUnlit, hard alpha다. 원본의 투명 여백을 정리하고 바닥 정렬한 뒤 비율을 유지해 네이티브 캔버스에 넣는다. 작은 자원을 액터 스케일로 늘리지 않는다. 분할 경계는 실제 그림의 여백을 따라 설정하며 건물 하단이 잘리지 않게 한다.

`PrepareTownArtV2.ps1` → `ApplyTownArtV2.py` 순서로 재적용한다. VisualStyle의 TownSprites 중 해당6개 키만 교체하므로 기존 수레·철도·유물 아이콘은 보존한다. 지상 주택은 기존 보급소 근처54·62열에 모으고 배경 주택은49·61열, 안내판은66열에 배치한다. 승강기94열·작업장108열은 오른쪽 돌 지형 위의 운송 구역으로 묶는다.

배경 주택과 안내판은 충돌 없는 PaperSpriteActor다. 생성 Transform으로 위치를 전달하고 렌더 컴포넌트를 Movable로 만든 뒤 스프라이트를 설정한다. Static 컴포넌트에 런타임 위치·스프라이트 변경을 시도하면 원점에 남는 문제가 실제 화면에서 재현되어 이 경로를 수정했다. 최종 Town PIE 검사는 배경3개가 올바른 위치·실제 스프라이트·비충돌 상태인지 확인한다.

2026-11-15 최종 화면: `Saved/AgentLogs/ui-20261115-075507.png` (마을), `ui-20261115-074843.png` (운송 시설), `ui-20261115-075024.png` (잡담). 기능 회귀17종과 배경·대화 재검사를 통과했다. 로그는 `PIEPerformance-TownV2-regression17.log`, `TownV2-final-dialogue-scenery.log`에 보존했다.

## 생성 지시 핵심

```text
New transparent production sprite atlas for ProjectMK. Match the existing supply shop and houses: muted weathered wood, slate iron, restrained teal/brass, crisp irregular pixel clusters, modest upper-left lighting and low-relief stepped shades. Six isolated front-facing orthographic side-view sprites, no people or characters. Build larger native sources at a similar pixel scale instead of enlarging small sprites. A substantial broad mining headframe with heavy bracing, double pulley, thick cables, cage and winding drum; a wide cozy railway workshop containing loose train wheels, axles, rail stock and an unfinished minecart on a work stand; two different larger cottages; a low-contrast group of three background cottages; a small blank noticeboard and fence. Transparent openings and outside alpha, grounded bases, generous separation. No lettering, logos, UI frames, chests, soil, scenic background, glossy 3D, gradients, bloom, antialiasing or black halo.
```
