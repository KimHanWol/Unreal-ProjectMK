# 월드 상점 V3

## 디자인 기준

`COMMON_PIXEL_ART_PROMPT.md`, `ART_V2_APPLICATION.md`, `WIDGET_ART_V1.md`와 기존 지형·소품을 참고했다. 새 상점에는 캐릭터, 상점 주인, 얼굴을 넣지 않는다. 짙은 목재, 낮은 채도의 청록·크림색 차양, 작은 황동 장식만 사용한다.

- 실제 게임 소스: `Content/Asset/WorldArtV3/Source/Shop.png`, 128×96.
- 생성 원본: 같은 폴더의 `Shop-original.png`.
- Unreal 참조: `T_Shop`, `S_Shop`, `BlockDataTable.Shop.TileSprite`.
- 월드 크기: 4×3블록, 2px/Unreal unit. 기존 32×32 상점 확대 참조를 교체한다.
- 지면 트레이스로 바닥을 맞추고 플레이어 뒤에 표시한다. 충돌로 이동·채굴을 막지 않는다.
- 상호작용은 상점 그림의 영역 안에서만 가능하며, F 안내는 플레이어 머리 가까이에 표시한다.

## 생성 지시

내장 `image_gen`을 사용했다. 최종 생성 지시는 다음 사양으로 구성했다.

```text
Create one character-free 2D pixel-art mining supply stall for ProjectMK.
Match the existing terrain and prop references and COMMON_PIXEL_ART_PROMPT.md:
muted earthy colors, dark brown timber, desaturated teal and cream striped
awning, restrained brass fittings, 3–5 stepped shades per material, modest
upper-left lighting and crisp intentional pixel clusters. Front-facing side-view
game prop, simple sturdy silhouette, irregular but carefully placed wear.
The native game asset is 128x96 pixels, four blocks wide and three blocks tall,
with the same logical pixel density as the existing 32x32 terrain tiles.
Keep details readable at that size. No people, merchant, character, face,
text, letters, sign lettering, logo, glossy 3D rendering, smooth gradients,
blur, bloom, excessive tiny ornaments, or presentation background.
Transparent background, isolated stall, grounded base.
```

`PrepareShopArt.ps1`가 투명 영역을 정리해 게임 해상도로 변환한다. `ApplyUIRepairArt.py`가 픽셀 필터·충돌·스프라이트와 데이터 참조를 적용한다. PIE 화면에서 지면 접촉과 캐릭터 제외를 확인했다.
