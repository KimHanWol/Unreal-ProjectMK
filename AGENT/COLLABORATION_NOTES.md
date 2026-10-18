# Collaboration Notes

## User Decision Rule

- 구현 방식에 선택지가 생기면 에이전트가 임의로 결정하지 않는다.
- 기존 동작을 바꾸는 수정, 체감 변화가 생길 수 있는 튜닝, 요구하지 않은 추가 판단이 필요한 경우 먼저 사용자에게 확인한다.
- 사용자가 명시적으로 요청한 범위만 수정한다.
- 사용자가 "기존과 동일하게", "원래처럼", "그대로"라고 말한 항목은 추정으로 바꾸지 않는다.

## Build Rule

- Unreal Editor가 열린 상태에서 외부 빌드를 실행하면 Live Coding 상태와 DLL이 충돌해 라이브 컴파일 환경을 망가뜨릴 수 있다.
- 에이전트는 빌드와 에디터 재시작을 `AGENT/Scripts/` 스크립트로만 수행한다. 직접 UBT/MSBuild를 호출하지 않는다.
  - `StopEditor.ps1` → `BuildEditor.ps1` → `StartEditor.ps1` 순서로 실행한다.
  - `BuildEditor.ps1`은 에디터가 실행 중이면 빌드하지 않고 종료한다.
  - `StartEditor.ps1`은 `-ModelContextProtocolStartServer`로 에디터를 띄우고 MCP 포트(8000)가 열릴 때까지 기다린다.
- 사용자가 에디터에서 작업 중일 수 있으므로, 에디터를 끄기 전에 저장되지 않은 변경이 없는지 확인한다.
