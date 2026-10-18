# 캐릭터 충돌과 스프라이트 확인

## 한 칸 통로에서 낙하가 멈추는 현상

- 재현 위치: 플레이어 X 1168.31 / Z -231.09. 16 단위 블록 사이 통로 중심은 X 1170이었다.
- 기존 캡슐 반경과 반높이는 각각 약 6.89였다. CharacterMovement가 왼쪽 블록 모서리 X 1162를 바닥으로 판정해 Walking 상태를 유지했다.
- BP_Player의 캡슐 반경을 5로 줄이고 반높이는 유지했다. CharacterMovement의 PerchRadiusThreshold는 3.5, PerchAdditionalHeight는 0으로 설정했다. 통로에서 모서리만으로 서 있는 상태를 줄인다.
- 몬스터도 반경 5에 같은 perch 설정을 적용했다.
- 실제 걸린 위치에 설정을 적용했을 때 Z -274.96까지 즉시 낙하했다.
- 에디터 재시작 후 `ProjectMK.PIE.Character.OneCellShaftCollision` 자동 테스트를 통과했다. 통로 중심에서 1.7 단위 벗어난 위치에서 48 단위 낙하했고, 아래 바닥 충돌도 유지됐다.

## 블록을 가리는 검은 사각형

- 원본의 발밑 그림자라는 최초 진단과 머티리얼 재컴파일로 해결됐다는 후속 판단은 잘못됐다. 작은 전체 화면만으로 완료 판정을 내려 사각형 가림을 놓쳤다.
- 같은 위치에서 캐릭터의 MainPass/DepthPass를 모두 끄면 가림이 없어져 CharacterVisualComponent로 범위를 좁혔다. DepthPass만 꺼도 기존 Masked 머티리얼에서는 재현됐고, Translucent 진단 머티리얼에서는 사라졌다.
- 실행 설정 `r.EarlyZPassOnlyMaterialMasking=1`을 확인했다. UE 5.8의 RendererScene.cpp 기본값도 1이다. RenderUtils.cpp의 MaskedInEarlyPass가 이 설정을 셰이더에 반영하며, BasePassPixelShader.usf는 이때 일반 그리기 단계의 GetMaterialCoverageAndClipping 호출을 생략한다.
- Paper2D의 스프라이트 그리기 경로에서 이 설정을 사용하면 투명 부분까지 사각형 지오메트리가 그려지는 것을 재현했다. Config/DefaultEngine.ini의 RendererSettings에서 `r.EarlyZPassOnlyMaterialMasking=False`로 지정해 일반 그리기 단계에서도 알파 마스크를 검사하도록 수정했다.
- 설정은 에디터 재시작이 필요하다. 재시작 후 콘솔 변수 0을 읽어 확인했고, 원래 MI_Character(Masked), MainPass=true, DepthPass=true 상태로 정면/왼쪽/오른쪽 프레임 모두 블록이 정상적으로 보이는 것을 확대 화면에서 확인했다.
- 원본 텍스처, 스프라이트 자산과 C++는 수정하지 않았다. 진단용 머티리얼 그래프·Translucent 변경 및 이전 재컴파일로 생긴 자산 변경도 원래 파일로 복구했다. 영구 수정은 프로젝트 렌더링 설정 한 항목이다.
- 화면 기록: `output/playtest/character-transparency-verified.png`, 확대 기록: `output/playtest/character-transparency-verified-detail.png`. 이전 character-transparency-fixed.png는 부정확한 검증 기록이므로 완료 근거로 사용하지 않는다.
- 진단 자산 파일까지 원본으로 복구하고 다시 에디터/PIE를 시작한 뒤에도 콘솔 변수 0과 사각형 제거를 재확인했다. 최종 확대 기록: `output/playtest/character-transparency-proof.png`.
