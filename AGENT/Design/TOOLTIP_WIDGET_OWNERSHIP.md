# 레시피 툴팁의 위젯 이름 충돌

사용자가 보고한 크래시와 스트레스 검사에서 재현한 크래시는 모두 Slate의 ImagePaint, 레시피 툴팁 재료 행의 SImage 경로에 있었다. 원래 기록은 장시간 실행 후 발생했고, 새 검사에서는 레시피·아이템 툴팁을 바꾸며 강제 GC를 실행해 재현했다.

이미지 리소스를 스프라이트에서 텍스처로 바꾸는 비교도 같은 경로에서 실패했다. 해당 가설의 변경은 되돌렸다.

실제 문제는 공통 `BuildItemIconFrame`이 같은 WidgetTree 아래에 `Slot`이라는 명시적 UObject 이름을 반복 사용한 것이었다. `ConstructWidget`은 NewObject를 호출하며, 같은 Outer·이름은 기존 객체를 재구성한다. 실행 검사에서 서로 다른 헤더·재료 행이 동일한 Slot·SizeBox·Image 주소를 참조하는 것을 확인했다. 이 상태에서 GC 후 이전 Slate 이미지가 수명이 끝난 Brush를 그리면서 접근 위반이 발생했다.

프레임의 스타일 키와 UObject 이름을 분리하고 `MakeUniqueObjectName`으로 생성한다. 위젯 트리 중복 참조 검사와 레시피/아이템 변경·강제 GC 120회 검사를 회귀 테스트에 포함한다.

증거는 Saved/AgentLogs의 `Expansion-first-crash.log`, `Expansion-tooltip-texture-comparison.log`, `Tooltip-widget-alias-reproduction.log`에 보존했다. 원본 CrashContext·덤프는 Saved/Crashes에 남겼으며 계정 식별자는 문서에 복사하지 않았다.


## 검증

명시적 이름 충돌을 검출하는 검사는 수정 전 실패했고, 고유 이름 적용 후 위젯 중복 참조 검사와 툴팁 변경·강제 GC120회가 통과했다. `Expansion-after-tooltip-stress.log`에 기록했다. 같은 연속 실행의 다음 PIE 시작에서 D3D12RHI 셰이더 파라미터 처리 오류가 별도로 발생하여 후속 회귀 실행에서 분리 조사한다. 이 오류는 이전 Slate ImagePaint 호출 스택과 다르다.

후속 검사에서는 강제 GC용 임시 툴팁을 제거한 후3프레임 동안 UObject를 유지해 Slate의 제거 갱신이 진행된 뒤 PIE 정리를 요청했다. GC120회와 다음 PIE 회수·승강기·카메라 검사가 모두 성공했으며 이후 수동 PIE 전환에서도 같은 RHI 오류는 재현되지 않았다. 이는 테스트 정리 순서를 보완한 것으로, 프로젝트 렌더러의 수정이나 D3D12 오류의 원인 확정을 의미하지 않는다. 최초 Slate 위젯 이름 충돌과 분리해서 기록한다.
