# Widget Art V1

기존의 한 가지 사각 패널을 모든 화면에 늘려 쓰던 구성을 용도별 그래픽으로 분리한다. 캐릭터·몬스터 애니메이션은 포함하지 않는다.

## 그래픽 역할

- `Panel`, `InsetPanel`: 메인 창과 내부 콘텐츠 영역.
- `WindowTitle`: 제작·상점 제목판.
- `RecipeCard`: 제작·판매·레시피 목록 행.
- `Button`, `ButtonHover`, `ButtonPressed`, `ButtonDisabled`: 버튼 네 상태.
- `Slot`, `SelectedSlot`: 아이템·스킬·퀵슬롯의 기본/선택 상태.
- `Quickbar`: 하단 퀵슬롯 전체 받침.
- `Tooltip`: 툴팁 전용 프레임.
- `BarFrame`, `OxygenFrame`: 체력·산소 게이지 하우징.
- `Minimap`: 미니맵 전용 프레임.
- `Keycap`: 퀵슬롯 번호 키캡.

원본 생성 시트는 `Content/Asset/WidgetArtV1/Source/skin-atlas.png`에 보관한다. `PrepareWidgetArt.cjs`가 각 역할의 실제 사용 비율에 맞춘 PNG로 분리하며, `ImportWidgetArt.py`와 `ApplyWidgetArt.py`가 텍스처 및 위젯 참조를 적용한다.

## 생성 프롬프트

기본 내장 image_gen을 사용했다. 최종 프롬프트는 `output/widget-art/generation-prompt.txt`에 보관한다. 4×4 시트, 투명 배경, 텍스트 없는 광산 테마 픽셀 UI, 용도별 비율과 9-slice 안전 영역을 지정했다.

## 적용 범위

- 제작창과 상점은 `InsetPanel`, `WindowTitle`, `RecipeCard`, `Slot`을 조합하며 기존 아이템 아이콘과 실제 레시피·판매 데이터를 그대로 사용한다.
- 인벤토리, 스킬 슬롯, 툴팁, 체력·산소 게이지, 미니맵은 각 용도에 맞는 전용 프레임을 사용한다.
- 퀵슬롯은 `Quickbar` 받침 위에 8개의 슬롯과 `Keycap`을 배치한다. 아이템 이름은 아이콘과 수량을 가리지 않도록 슬롯 안에서 숨기고 툴팁에서 확인한다.
- 드릴 접촉 이펙트는 텍스처 해상도와 무관하게 블록 폭의 `1.15`배로 표시한다.

## 두 번째 UI 패스

- 광산 게임 참고 자료의 정보 구조를 분석해 ProjectMK 고유의 황동·청록 테마로 다시 제작했다. 특정 게임의 그래픽은 복제하지 않는다.
- 장식 밀도는 외곽 창, 제목판, 선택 상태에 집중하고 일반 목록과 슬롯의 금색 테두리는 얇고 어둡게 낮췄다.
- 제작 목록은 두 열 카드 배치로 변경해 한 화면에서 더 많은 레시피를 비교할 수 있게 했다.
- 상점의 판매/레시피 분할 구조와 하단 퀵슬롯은 유지하면서 새 버튼 상태, 슬롯, 게이지와 미니맵 프레임을 공통 적용한다.
- 이전 시트는 `output/widget-art/skin-atlas-v1.png`에 비교용으로 보관한다.

## 공통 레이아웃 시스템

- 텍스트는 `Caption 12`, `Body/Button 14`, `Label/Numeric 16`, `SectionTitle 18`, `WindowTitle 22`의 역할 단위로 지정한다. 개별 화면에서 임의 크기를 추가하지 않는다.
- 기본 여백은 `4 / 8 / 12 / 16 / 24` 단계만 사용한다. 팝업, 카드, HUD의 패딩과 요소 간격은 `FMKUILayout` 상수를 공유한다.
- 제작·상점·테스트 HUD는 `BuildSectionHeader`, `BuildSectionLabel`, `BuildItemIconFrame`, `BuildActionButton`을 통해 제목판, 섹션 제목, 아이템 슬롯, 상태형 버튼을 재사용한다.
- 공통 버튼 라벨은 자동 줄바꿈을 끈다. 의도적으로 두 줄을 표시하는 제작 잠금 안내만 명시적 줄바꿈을 사용한다.
- 스킬 트리는 기존 노드 배치와 드래그·확대 기능을 보존하면서 공통 `WindowTitle` 제목판을 런타임에 합성한다.
- 블루프린트 기반 아이템 슬롯, 통화, 툴팁도 위젯 이름에 따른 공통 텍스트 역할을 런타임에 적용한다.

## UX 구조 개선

- 재화는 명칭을 반복 표기하지 않고 화면 우상단의 아이콘과 수치로 유지한다. 상점·기술 노드의 가격도 같은 아이콘과 숫자 조합을 재사용한다.
- 판매 화면은 드래그 앤 드롭을 필수 동작으로 사용하지 않는다. 보유 광물 목록에서 묶음 판매를 명시적으로 선택하거나 하단의 일괄 판매를 사용하도록 해 발견성과 조작 속도를 함께 확보한다.
- 설계도는 제작 재료를 중복 노출하지 않고 결과 아이템, 이름, 재화 가격만 보여준다. 가격 버튼 자체가 해금 동작이다.
- 기술 트리는 블루프린트에 임시 배치한 화살표 이미지를 사용하지 않는다. `SkillDataTable`의 `RequiredSkill` 관계에서 열 깊이와 연결선을 생성해 선행 기술을 직접 읽을 수 있게 한다.
- 체력·산소 게이지는 기존 이미지 위젯을 겹쳐 쓰지 않고 공통 프레임 안에 진행도와 짧은 상태 라벨을 합성한다.
- 초상화의 오른쪽 끝과 상태 게이지 시작점 사이에 약 `9.4px`의 안전 간격을 확보하고, 퀵슬롯·상점 카드·창 제목에도 최소 `8px`의 외부 여백을 둔다.

## 검증 결과

- UI 텍스처 16개와 스타일 브러시 16개의 로드 및 연결을 확인했다.
- 실행 중 퀵슬롯 8개와 제작 레시피 카드 8개의 생성을 확인했다.
- 드릴 접촉 이펙트의 실제 렌더 폭은 `18.4` 월드 단위이며, 16 단위 블록 기준 `1.15`블록이다.
- 기존 Art V1 텍스처 130개, 스프라이트 130개, 플립북 5개와 데이터 테이블 비시각 필드가 유지되는 것을 확인했다.
- 결과 보고서는 `output/widget-art/validation-report.json`, `output/widget-art/editor-validation-report.json`, `output/widget-art/drill-effect-validation.json`에 보관한다.
- 레이아웃 감사 결과는 `output/widget-art/layout-audit.json`에, 대표 화면은 `crafting-layout-pass.png`, `shop-layout-stable.png`, `skills-layout-pass.png`에 보관한다.
