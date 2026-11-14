# 광산 지지대 — 열린 바닥과 X 보강재

기존 갱구와 보급소를 참고하여 내장 imagegen으로 다시 생성했다. 처음 생성한 바닥 가로대가 있는 사각 프레임은 사용하지 않는다. 상단 들보, 좌우 기둥, 내부의 얇은 X 보강재만 남긴다. 투명한 공간을 바닥판으로 채우지 않는다.

- 스타일 참고: `Content/Asset/MineDecorV1/Source/MineEntrance.png`, `Content/Asset/WorldArtV3/Source/Shop.png`.
- 생성 원본: `Content/Asset/SupportClimbV1/Source/support-original.png`.
- 게임 텍스처: `Content/Asset/SupportClimbV1/Source/SupportBeam.png`, 32×32px.
- 런타임: `/Game/Asset/SupportClimbV1/S_SupportBeam`.
- 재현: `PrepareSupportFrame.ps1`로 크기·알파를 정리하고 `ApplyTownArt.py`로 텍스처·스프라이트·아이템·블록 데이터를 등록한다.
- 처리: Nearest, NoMipmaps, hard alpha, MaskedUnlit, 2px/월드 단위. 원본의 반투명 가장자리를 제거하고 본체를 바닥에 정렬한다.
- 렌더 순서: 지지대 → 드랍 아이템 → 캐릭터. 물리 윗면은 상단 들보에 맞춘 별도 단방향 충돌이다. 기둥 내부는 지나갈 수 있다.

## 생성 프롬프트 핵심

```text
One mining support sprite, using the supplied mine entrance and supply shop as style references. Match their muted weathered brown wood, dark desaturated iron, restrained upper-left lighting, and coarse hand-made pixel clusters. A square logical 32x32 canvas: an inverted-U timber frame with a top horizontal beam and two vertical posts. Thin diagonal wooden braces form an X between the posts. NO bottom horizontal beam, no floor, no platform, no closed rectangular base. Leave all openings transparent. Modest worn iron plates at the upper joints, sparse rivets, no shiny golden decoration. Front-facing orthographic side view, crisp hard-alpha edges. No characters, scene, ground, shadow, lettering, gradients, antialiasing, glossy 3D or glow.
```

원본 형태를 그대로 고집하기보다 현재 게임의 목재·철물 색과 픽셀 밀도에 맞추는 것을 우선한다. 바닥 가로대가 없는 구조는 사용자 요청으로 유지한다.
