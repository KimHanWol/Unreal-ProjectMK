# ProjectMK Coding Convention

이 문서는 `Source/ProjectMK` 이하 코드 수정 전에 먼저 확인하는 기준 문서다.  
코드 작성, 리팩토링, 구조 정리, 주석 정리, 레거시 제거 판단은 이 문서를 기준으로 맞춘다.

## 1. 기본 원칙

- Unreal Engine 기본 스타일과 네이밍 규칙을 따른다.
- 기존 프로젝트 문맥과 충돌하지 않는 범위에서 정리한다.
- 구조를 억지로 잘게 쪼개지 않는다.
- 수정한 파일은 가능하면 같은 작업 안에서 함께 정리한다.
- 이 문서를 수정하면 `CODING_CONVENTION.txt` 도 같은 작업에서 같이 수정한다.

## 2. 인코딩과 줄바꿈

- 코드와 문서는 `UTF-8 BOM` 을 사용한다.
- 줄바꿈은 `CRLF` 로 통일한다.
- 모든 텍스트 파일은 마지막 줄바꿈을 포함한다.
- trailing whitespace 는 남기지 않는다.

## 3. 들여쓰기와 공백

- C++ 과 C# 은 `Tab` 들여쓰기를 사용한다.
- 정렬 목적의 공백 맞춤은 최소화한다.
- 의미 없는 빈 줄은 만들지 않는다.
- 함수 묶음과 변수 묶음 사이에는 한 줄 개행을 둔다.

## 4. 파일 헤더와 include

- 새 파일에 Unreal 기본 placeholder 주석은 넣지 않는다.
- 기존 `// LINK` 배너를 유지해야 하는 파일은 그대로 둔다.
- 헤더는 `#pragma once` 를 사용한다.
- `*.generated.h` 는 항상 마지막 include 로 둔다.
- `.cpp` 는 자기 자신의 헤더를 가장 먼저 include 한다.
- 헤더 include 는 가능하면 줄이고 forward declaration 을 우선한다.

## 5. 네이밍

- 클래스, 구조체, 함수, enum, UPROPERTY 이름은 PascalCase 를 사용한다.
- bool 은 `b` 접두어를 사용한다.
- UObject 계열은 Unreal 접두어 규칙 `U`, `A`, `F`, `E`, `I` 를 따른다.
- 애매한 축약은 피하고 의미가 드러나는 이름을 사용한다.
- 같은 역할의 함수는 같은 접두어를 사용한다.

## 6. 함수 이름 접두어 규칙

- `On~`
  - delegate, attribute, input binding 처럼 외부 이벤트를 직접 받는 콜백 전용이다.
  - 예시: `OnCurrentHealthChanged`, `OnMoveRight`

- `Try~`
  - 조건을 확인한 뒤 어떤 동작을 시도하는 함수에 사용한다.
  - 예시: `TryDrill`

- `Apply~`
  - 상태, 효과, 렌더링 설정처럼 어떤 것을 적용하는 함수에 사용한다.
  - 예시: `ApplyOxygenDrainEffect`, `ApplyDamageInvincibility`

- `Clear~`
  - 적용된 상태나 효과를 해제할 때 사용한다.
  - 예시: `ClearOxygenDrainEffect`

- `Update~`
  - 현재 상태를 바탕으로 값을 즉시 갱신할 때 사용한다.
  - 예시: `UpdateOxygen`, `UpdateHorizontalMovement`, `UpdateVisuals`

- `Refresh~`
  - UI 처럼 다시 표시하거나 다시 채워 넣는 성격일 때 사용한다.
  - 예시: `RefreshCoinText`

- `Find~`
  - 검색 또는 탐색이 들어갈 때 사용한다.
  - 예시: `FindSpriteComponentByName`

- `Get~`
  - 이미 들고 있는 참조, 캐시, 값, 설정을 반환할 때 사용한다.
  - 예시: `GetGameSettings`, `GetCharacterData`

- `Calculate~`
  - 계산 과정을 거쳐 값을 만들어내는 함수에 사용한다.
  - `Resolve~` 대신 이 표현을 우선한다.
  - 예시: `CalculateCurrentAnimationType`

- `Is~`, `Can~`, `Has~`
  - 조건 판단 함수에 사용한다.

## 7. 클래스 책임 분리

### 7.1 Manager

- `Manager` 는 공용 데이터 접근, 전역 설정, 자산 조회 같은 서비스 책임을 가진다.
- 특정 월드 진행이나 액터 흐름을 직접 제어하지 않는다.

### 7.2 Subsystem

- `Subsystem` 은 월드 생명주기, 월드 흐름, 여러 액터를 묶는 진행 제어를 담당한다.
- 월드 의존 로직은 `Manager` 보다 `Subsystem` 에 우선 배치한다.

### 7.3 Actor 와 Component

- `Actor` 는 조합과 상위 흐름 제어를 담당한다.
- `Component` 는 재사용 가능하거나, 본체 클래스 안에서 책임이 많이 얽힌 경우에만 분리한다.
- 작은 helper 몇 개만 있다고 바로 컴포넌트로 분리하지 않는다.
- 먼저 private helper, 함수 묶음, 섹션 재배치로 정리해보고 그래도 복잡하면 그때 분리한다.

### 7.4 Character 상태 관리

- 체력, 산소, 이동속도 같은 실제 수치는 GAS `AttributeSet` 이 기준이다.
- 별도 컴포넌트를 만들더라도 Attribute 값을 복제해서 소유하지 않는다.
- 상태 처리 컴포넌트가 필요하면 수치 저장보다 상태 반응과 규칙 적용을 맡긴다.

### 7.5 AbilitySystemComponent 확장

- 공용 GAS helper, 공용 태그 처리, 공용 ability/effect 유틸은 ASC 서브클래스로 넣을 수 있다.
- 캐릭터 고유 입력 정책이나 캐릭터 전용 흐름은 ASC 서브클래스에 넣지 않는다.

## 8. Widget 구조

- 모든 위젯 클래스는 `UMKUserWidget` 계열이어야 한다.
- `UHUDWidget` 도 예외가 아니라 `UMKUserWidget` 계열 위젯이다.
- `UHUDWidget` 를 상속하는 별도 위젯 계층은 만들지 않는다.
- Controller, Character, ASC 공통 접근은 `UMKUserWidget` 의 공통 접근 지점을 통해 처리한다.

## 9. 데이터 접근

- DataTable, DataAsset, 전역 설정값 접근은 `UDataManager` 경유를 원칙으로 한다.
- 게임 로직 클래스가 DataTable 을 직접 `FindRow` 하는 구조는 지양한다.
- 게임 전반 규칙값은 `GameSettingDataAsset` 이동 여부를 먼저 검토한다.
- UI 표시용 세부 조정값은 BP 노출 프로퍼티로 두는 것을 우선한다.

## 10. UPROPERTY / UFUNCTION

- UObject 멤버 참조는 기본적으로 `UPROPERTY` + `TObjectPtr` 를 사용한다.
- 생명주기를 추적하지 않는 캐시 참조만 `TWeakObjectPtr` 를 검토한다.
- `EditAnywhere` 보다 더 좁힐 수 있으면 `EditDefaultsOnly` 또는 `VisibleAnywhere` 를 사용한다.
- `BlueprintReadWrite` 는 꼭 필요한 경우에만 사용하고, 기본은 `BlueprintReadOnly` 로 본다.
- `BlueprintCallable` 은 실제로 블루프린트 호출이 필요한 함수에만 붙인다.

## 11. Tick 과 이벤트 바인딩

- `Tick` 은 기본 선택지가 아니라 예외적인 선택지다.
- delegate, timer, overlap, GAS attribute change, 명시적 갱신 호출로 대체 가능한지 먼저 본다.
- Widget 은 가능하면 이벤트 기반 갱신을 우선한다.
- 이벤트 바인딩은 생명주기에 맞춰 짝으로 관리한다.
- 공통 관례는 `BindEvents()` / `UnbindEvents()` 다.

## 12. 헤더 선언 순서

헤더의 함수 선언 순서는 항상 아래 기준을 따른다.

1. 생성자 / 소멸자
2. 엔진 override
3. `BindEvents()` / `UnbindEvents()` 계열
4. 외부 상호작용 함수
5. 내부 동작 함수
6. 조회 / 계산 / 설정 / 유틸 함수
7. `On~` 콜백 함수
8. delegate 선언
9. 멤버 변수

추가 기준은 아래와 같다.

- override 는 클래스의 진입점이므로 가장 위에 둔다.
- `BindEvents()` / `UnbindEvents()` 는 override 바로 아래 고정이다.
- 외부 상호작용 함수는 다른 클래스가 호출하는 공개 동작이다.
- 내부 동작 함수는 실제 처리 로직이다.
- 조회/계산/유틸은 `Get~`, `Calculate~`, `Find~`, `Is~`, `Can~`, `Has~`, `Set~` 계열이다.
- `On~` 함수는 바인딩용 콜백이므로 한 구역에 모은다.

## 13. cpp 정의 순서

- `.cpp` 의 함수 정의 순서는 헤더 선언 순서와 반드시 맞춘다.
- 헤더에서 위에 선언된 함수가 `.cpp` 에서도 먼저 구현되어야 한다.
- 리팩토링 시 함수 순서를 다시 정리할 때는 선언 순서와 구현 순서를 함께 맞춘다.
- 새 함수를 추가할 때도 헤더와 `.cpp` 양쪽에 같은 묶음과 같은 위치에 넣는다.

## 14. 함수 묶음 규칙

- 함수는 이름순이 아니라 역할 순으로 정렬한다.
- 같은 역할의 함수는 한 묶음으로 배치하고, 묶음 사이에는 한 줄 개행을 둔다.
- 같은 묶음 안에서는 접두어와 의미를 통일한다.

예시 묶음:

- Input
  - `OnLookRight`
  - `OnLookUp`
  - `OnMoveRight`
  - `OnFly`
  - `OnFinishFly`

- Movement Update
  - `UpdateHorizontalMovement`
  - `UpdateFlyingVerticalVelocity`

- Status Apply / Clear / Update
  - `ApplyDamageInvincibility`
  - `ApplyOxygenDrainEffect`
  - `ClearOxygenDrainEffect`
  - `RestoreOxygenToMax`
  - `UpdateOxygen`

- Rendering Apply
  - `Apply2DCameraOverrides`
  - `ApplyTextureRenderingOverrides`
  - `ApplySpriteRenderingOverrides`

- Query / Calculate
  - `GetCurrentBlockDepth`
  - `GetGameSettings`
  - `GetCharacterData`
  - `CalculateCurrentAnimationType`
  - `CalculateCurrentBaseFrameSprite`
  - `CalculateCurrentBasePixelsPerUnrealUnit`

- Event Callback
  - `OnItemCollectRangeChanged`
  - `OnCurrentHealthChanged`
  - `OnCurrentOxygenChanged`

## 15. 레거시 / 호환 코드

- 호환용 코드는 기본적으로 남겨두지 않는다.
- 남겨야 한다면 왜 남아있는지, 어떤 자산이나 데이터 때문인지, 언제 제거 가능한지 주석으로 남긴다.
- 임시 호환 코드가 구조를 흐리지 않도록 지속 정리한다.

## 16. 주석과 로그

- 주석은 왜 필요한지 설명할 때만 짧게 남긴다.
- 당연한 코드 설명 주석은 지양한다.
- TODO 는 `// TODO: ...` 형식으로 작성한다.
- 반복 Tick 로그나 의미 없는 로그는 남기지 않는다.

## 17. 리팩토링 마감 전 확인

- 호환용 코드가 정말 필요한지 다시 확인한다.
- DataTable, DataAsset, Widget 접근이 직접 참조로 되돌아가지 않았는지 확인한다.
- Tick, delegate, widget binding 이 불필요하게 늘어나지 않았는지 확인한다.
- Blueprint 노출 범위가 과하게 넓어지지 않았는지 확인한다.
- 가능하면 빌드 또는 최소 동작 확인까지 하고 마감한다.
