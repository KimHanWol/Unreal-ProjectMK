# ProjectMK Refactor Exit Checklist

리팩토링을 마감하기 전에 최소한 아래 항목을 다시 확인한다.

## 1. Legacy / Compatibility Code

- 호환용 코드가 정말 필요한지 다시 확인한다.
- 남겨야 한다면 왜 남아있는지, 어떤 자산이나 데이터가 의존하는지, 언제 제거 가능한지 주석으로 남긴다.
- "일단 남겨둔다" 식의 코드 보존은 지양한다.

## 2. Responsibility Regression Check

- `Manager`, `Subsystem`, `Actor`, `Component`, `Widget` 책임이 다시 섞이지 않았는지 확인한다.
- 정리한 뒤에도 DataTable, DataAsset, 설정값 접근이 `UDataManager` 경유 원칙을 지키는지 확인한다.
- 위젯이 Controller, Character, ASC 를 직접 중복 접근하지 않고 `UMKUserWidget` 공통 접근 지점을 사용하는지 확인한다.

## 3. Runtime Cost Check

- `Tick` 이 새로 늘어나지 않았는지 확인한다.
- delegate binding / unbinding 이 생명주기에 맞게 정리되어 있는지 확인한다.
- 위젯 갱신이 가능하면 이벤트 기반으로 유지되고 있는지 확인한다.

## 4. Exposure Check

- `EditAnywhere`, `BlueprintReadWrite` 가 필요 이상으로 넓게 열리지 않았는지 확인한다.
- 게임 전반 규칙값은 `GameSettingDataAsset` 으로 가야 하는지 다시 판단한다.
- 단순 UI 설정값은 BP 노출로 충분한지 확인한다.

## 5. Close-out Check

- 가능하면 빌드 또는 최소 동작 확인까지 하고 마감한다.
- 정리 중 발견한 후속 작업이 있으면 TODO 가 아니라 제거 조건이 있는 메모로 남긴다.
