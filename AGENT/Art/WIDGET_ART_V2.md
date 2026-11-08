# Widget Art V2 — 역할별 UI 부품

기존 황동·청록 테마를 유지하면서, 버튼 한 종류를 모든 조작에 사용하던 구조를 역할별로 나눈다. 새 캐릭터·몬스터 아트는 만들지 않는다.

## 생성·보관

- 내장 image_gen으로 4×4 투명 시트 한 장을 생성했다. 원본은 `Content/Asset/WidgetArtV2/Source/component-atlas.png`이며 실제 크기는 1254×1254이다.
- 사용한 전체 프롬프트는 `AGENT/Art/WIDGET_ART_V2_PROMPT.txt`에 보관한다.
- 개별 PNG와 `manifest.json`은 같은 Source의 UI 디렉터리에, Unreal 텍스처는 `/Game/Asset/WidgetArtV2/UI`에 둔다.
- `PrepareWidgetArtV2.cjs`는 셀별 알파 경계를 찾고 네이티브 UI 크기로 최근접 리샘플링한다. 가장자리 알파는 0/255로 정리한다. 생성 원본은 덮어쓰지 않는다.
- `ImportWidgetArtV2.py`는 Nearest·NoMipmaps·UI 압축/그룹으로 가져와 기존 DA_VisualStyle의 역할 브러시에 연결한다. 기존 지형·캐릭터·아이템 텍스처를 교체하지 않는다.

## 역할과 사용 위치

| 역할 | 크기 | 용도 |
| --- | --- | --- |
| ActionPrimary | 128×40 | 판매·제작·해금 등 주요 동작 |
| ActionSecondary | 128×40 | 취소·보조 동작 |
| ActionDanger | 128×40 | 버리기 |
| QuantityPreset / Selected | 96×32 | 1개·절반·전체 및 현재 수량 표시 |
| Stepper | 32×32 | 수량 −/+ |
| InputWell | 128×40 | 편집 가능한 숫자 입력 |
| SaleTotal | 128×40 | 판매 금액의 가죽 받침 |
| MerchantMat | 160×88 | 상점의 드롭 판매 영역 |
| RecipeRow | 192×64 | 제작·설계도 목록의 아이콘 소켓과 금속 행 |
| InventoryDrawer | 160×88 | 퀵슬롯·인벤토리의 서랍형 받침 |
| SectionDivider | 192×16 | 팝업 영역 구분 장식 |
| IconSatchel / IconStorage | 48×48 / 48×40 | 인벤토리·퀵슬롯 프레임의 작은 표식 |
| IconWorkbench / IconTrade | 64×64 / 48×48 | 제작·상점 상단 대표 그래픽 |

## 재사용 규칙

- UMKMockupButton이 VisualRole을 가진다. BuildActionButton의 호출자가 역할을 지정하고, 일반·호버·누름·비활성 상태는 같은 역할의 그래픽을 기준으로 만든다. 상위 위젯이 스타일을 다시 적용해도 지정한 역할을 보존한다.
- QuantityPresetSelected는 현재 입력 수량에 맞춰 상위 수량 위젯이 갱신한다. 주요/보조/증감 버튼이 실제로 서로 다른 텍스처를 쓰는지 회귀 테스트로 확인한다.
- 그림에 텍스트·숫자를 굽지 않는다. 제목·수량·가격·± 기호는 기존 공용 텍스트와 재화 위젯으로 표시한다.
- 모서리는 manifest의 9-slice 여백으로 보존한다. 가운데 다이아몬드 구분 장식과 아이콘은 고정 비율로 표시한다.
- 레시피 아이콘은 배경에 그려진 소켓 안에 배치하며, 별도 테두리를 중복하지 않는다.

## 추가 드롭 수정

- WorldCoinSizeMultiplier: 1.5 → 0.9. 같은 월드 아이콘 기준에서 동전은 이전 크기의 60%이다.
- 전역 시간과 무작위 위상 대신 아이템별 FloatElapsedTime을 사용한다. 초기 위치는 정확히 아래쪽이며 `1 - cos(age × speed)`로 먼저 올라간다.
- FloatAmplitude: 5 → 2.5, 실제 위아래 범위는 10 → 5 월드 단위이다. 충돌·중력·자석 수집 경로는 변경하지 않는다.

## 검증

- [x] 16개 PNG의 알파·해상도와 Unreal 참조 확인
- [x] Editor Development 빌드
- [x] 서로 다른 버튼 아트·수량 판매·호버·길게 구매 회귀 테스트
- [x] 동전 크기·떠오름 시작 위치·처음 방향·절반 이동 폭·드롭/수집 회귀 테스트
- [x] 실제 HUD·제작·상점·수량 화면 확인
- [x] 기능별 커밋·푸시 및 작업 폴더 정리

2026-11-09 검증 기록:

- Editor Development 빌드 성공(50.27초).
- 00:04:04 `ProjectMK.PIE.Inventory.SeparateQuickSlotsAndWorldDrop`, 00:05:15 `ProjectMK.PIE.UI.DragSaleAndSkillHold`, 00:05:17 `ProjectMK.PIE.UI.PopupScrollAndPassiveClassification` 성공.
- 임포트 후 디스크의 텍스처 16개와 DA_VisualStyle의 16개 참조를 확인했다. 저장은 only_if_is_dirty=False로 강제하고 반환값을 검사한다.
- `output/widget-art-v2/source-validation.json`에 각 PNG의 크기와 알파(0/255) 검증 결과를 기록했다.
- `Saved/AgentLogs/ui-20261109-000621.png` HUD, `000628` 제작, `000701` 상점, `000708` 판매 수량, `000734` 버리기 수량을 직접 확인했다. 수량 미리보기는 확정하지 않고 취소했다.
- 테스트 전 백업과 종료 후 저장의 MKSaveGame 데이터 전체가 같았다. 비교는 `Saved/AgentLogs/widget-art-v2-save-comparison.json`에 기록했다. 게임 데이터 SHA256은 `4ab5c8dcf325ddb5861df3569b5afa094505ac88a61de2a5102032a3d58278c8`이다.
- 드롭 조정은 `c2bafff`로 별도 커밋·푸시했다. 아트·UI 구현과 검증·문서도 각각 묶어 보관한다. 종료 시 PIE와 미리보기를 정리하고 백그라운드 CPU 제한을 복원했다.
