# ArtV1 그래픽 적용 기록

2026-10-19. 캐릭터·몬스터 본체 및 애니메이션을 제외한 게임용 그래픽 세트다.

현재 지형 5종·광맥 8종·지하 배경은 ArtV2로 교체됐다. 아래 기존 디자인 설명은 ArtV1 보관본 기준이다. 최신 픽셀 밀도와 적용 상태는 [ART_V2_APPLICATION.md](ART_V2_APPLICATION.md)를 따른다.

## 파일과 크기

| 분류 | PNG 수 | 실제 크기 | 용도 |
|---|---:|---|---|
| Terrain | 8 | 32×32 | 흙·잔디·돌·얼음·마그마·모래·버팀목·완충 젤 |
| Ores | 8 | 32×32 | 지형 위에 합성하는 광맥 |
| Damage | 4 | 32×32 | 손상 20·40·60·80% 균열 |
| Items | 20 | 32×32 | 재료·광물·소모품 아이콘 |
| Skills | 25 | 32×32 | 유효 38행을 23개 계열 그림으로 연결, 예비 2종 |
| Symbols | 16 | 32×32 | 상점 외관·마커·화살표·체크·닫기 등 |
| UI | 16 | 64×64 또는 64×20 | 패널·슬롯·버튼·HP/O2 바·미니맵 프레임 |
| Effects | 16 | 접촉 24×24, 파괴 64×64, 발파 96×96, 획득 20×20 | 각 4프레임, 원본 아틀라스에서 다시 추출 |
| Backgrounds | 2 | 지상 512×512, 보관 지하 128×128 | 현재 지하는 ArtV2 512×512 사용 |
| 합계 | 115 | | Texture 115개, PaperSprite 115개, Flipbook 5개 |

- 게임 자산: `/Game/Asset/ArtV1` (`Content/Asset/ArtV1`).
- 가공 완료 PNG: `Content/Asset/ArtV1/Source/<분류>`.
- 묶음 생성 원본: `Content/Asset/ArtV1/Source/Atlases`. 원본은 고해상도이며 용도별 해상도로 추출한 게임용 파일과 구분한다.
- 전체 파일 목록: [manifest.json](../../Content/Asset/ArtV1/Source/manifest.json).
- 생성 프롬프트: [ART_V1_PROMPTS.md](ART_V1_PROMPTS.md), 공통 기준: [COMMON_PIXEL_ART_PROMPT.md](COMMON_PIXEL_ART_PROMPT.md).
- 미리보기: [지형](../../output/art-v1/Terrain.png), [아이템](../../output/art-v1/Items.png), [스킬](../../output/art-v1/Skills.png), [UI](../../output/art-v1/UI.png).

## 기존 디자인과 이번 생성분

이전 대화 「공통 픽셀 블록 프롬프트 작성」의 마지막 광물 작업은 v8이며, 합성 미리보기에 사용한 바탕은 v9다.

- 흙·얼음·마그마·돌: `output/imagegen/blocks-and-overlays-v9/blocks.png`의 네 셀을 그대로 사용한다.
- 균열 4장: 같은 폴더의 `damage-overlays.png`를 그대로 사용한다.
- 광맥 8종: `output/imagegen/mineral-overlays-v8/Ore_*.png`를 그대로 사용한다.
- 위 16개 파일은 RGBA 픽셀 비교를 통과했다. 재생성하거나 색상을 변경하지 않았다.
- 잔디 윗면·모래·버팀목·완충 젤, 상점, 아이템/UI/스킬, 배경/효과는 이번 생성분이다. 잔디는 기존 흙 위에 새 윗면 6픽셀을 합성한다.
- 마그마는 자산만 준비됐다. 기존 BlockDataTable에 행이 없어 새 생성 규칙을 추가하지 않았다. 얼음의 기존 생성·채굴 설정도 유지한다.
- 광물 아이콘과 지형 광맥은 별도 이미지다. `ItemIcon`은 인벤토리/드롭용, `BlockOverlaySprite`는 블록 위 합성용이다.

## 게임 연결

- BlockDataTable: 숨김 Border를 제외한 7행의 TileSprite 교체.
- ItemDataTable: 20행 ItemIcon, 광물 8행 BlockOverlaySprite 연결.
- SkillDataTable: NewRow를 제외한 유효 38행 IconPath 연결.
- `DA_VisualStyle`: 30개 공용 브러시, 완충 젤, 효과 4종을 관리한다. BP_DataManager의 VisualStyleDataAsset으로 연결한다.
- 기존 UMG 10개에서 총 52개 브러시 설정을 교체했다. 인벤토리, 아이템/스킬 슬롯, 스킬트리 화살표, 툴팁, 메뉴/닫기, 초상화 프레임, 코인, HP/O2 바에 적용했다.
- 코드로 생성되는 상점·제작·테스트 패널에도 같은 공용 스타일을 적용한다.
- 하단 퀵슬롯은 8칸의 키·수량·아이콘·사용 가능 강조를 유지하며 단일 테두리를 사용한다.
- 우상단 미니맵은 새 프레임과 플레이어/상점 마커를 사용한다. 범위 배율과 숨김 경계 제외 규칙은 유지한다.
- BP_Block: 새 균열/파괴 Flipbook. 내구도 손실 20%부터 네 균열 단계를 표시한다.
- 완충 젤은 블록 전체 색을 바꾸는 대신 전용 Sprite를 겹친다.
- 채굴 접촉·파괴·발파·아이템 획득 시 새 효과를 재생한다. 공용 효과는 카메라 쪽 Y +3 위치에 생성한다.
- Main 레벨의 `ArtV1/Backgrounds` 폴더에 지상·지하 타일맵을 배치했다. 실제 잔디 표면에 맞추며 충돌이 없고 `ArtBackground` 태그로 게임용 타일맵 탐색에서 제외한다.

## 검증과 재적용

- Development Editor 빌드 성공.
- [PNG/원본/데이터 검증](../../output/art-v1/validation-report.json): 115개 크기·하드 알파, 원본 16개 픽셀 동일, 세 DataTable의 그래픽 외 필드 동일.
- [에디터/PIE 검증](../../output/art-v1/editor-validation-report.json): Texture/Sprite 각각 115개, 4프레임 Flipbook 5개, 런타임 스타일·미니맵 1개·퀵슬롯 8개 확인.
- PIE에서 지상 배경 정렬, 기존 블록, 상점 외관, 제작/상점/스킬 UI, 완충 젤 Sprite를 확인했다.
- 채굴 접촉 효과의 앞/뒤 배치 비교에서 Y -3은 블록에 가려지고 Y +3은 표시되는 것을 확인했다.
- 실제 폭약 사용으로 FB_Explosion 생성, Y -17(플레이어 Y -20에서 +3), 충돌 없음 확인: [실행 기록](../../output/art-v1/effect-runtime-report.json).
- 테스트용 재료/소모품 지급은 기존 Playtest 기능을 사용한다. 테스트 세션에서는 진행 저장을 차단한다.
- 데이터 원본/적용본과 UMG 변경 전후 스냅샷은 `output/art-v1`에 보관한다.

스크립트 실행 순서:

1. `PrepareArtV1.cjs`: 저장된 원본 아틀라스에서 PNG 추출. 이미 보관된 아틀라스가 있으면 생성 캐시가 필요 없다.
2. `ImportArtV1.py`: Unreal 안에서 Texture/Sprite 생성과 렌더링 설정. 기존 Texture는 자동 재임포트하지 않으므로 PNG를 수정한 경우 해당 Texture를 명시적으로 Reimport한다.
3. `ApplyArtV1.py`: Unreal 안에서 DataTable/스타일/Flipbook/BP/배경 연결. Main 레벨을 열고 PIE를 종료한 상태에서 실행한다. 기존 UMG 브러시는 저장된 uasset에 포함되며 이 스크립트의 재작성 대상은 아니다.
4. 현재 ArtV2 상태를 재현하려면 `PrepareArtV2.cjs` → `ApplyArtV2.py`를 이어서 실행한다. ArtV1 적용만 실행하면 기존 지형과 지하 배경으로 돌아간다.
5. `VerifyArtV1.cjs`, `VerifyArtV1Editor.py`: ArtV1과 존재하는 ArtV2를 함께 검증한다. 각각 파일/데이터 비교와 에디터 참조 검증이며, 에디터 검증은 PIE 중이면 런타임 HUD까지 확인한다.

Python 원격 실행은 로컬 에디터 점검 시에만 일시적으로 사용하고 작업 후 비활성화한다. 에디터 종료·빌드·재시작은 기존 `AGENT/Scripts` 수명주기 스크립트를 사용한다.

## 범위

캐릭터·몬스터 본체/애니메이션과 기존 초상화 렌더링은 유지한다. 현재 사용하지 않는 옛 장비 22종과 미구현 화면/NPC는 이번 세트에 포함하지 않는다. Chain/Ore_Test 및 일부 공용 기호는 재사용 가능한 예비 자산이다. 인벤토리 문서의 선택 연출(별도 사망/리스폰·스킬별 전용 VFX 등)을 모두 구현했다는 의미는 아니다.
