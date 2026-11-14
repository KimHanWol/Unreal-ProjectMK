# 광산 장식 V1

기존 COMMON_PIXEL_ART_PROMPT.md와 WORLD_SHOP_V3.md의 나무·철·황동 색감에 맞춘 비충돌 배경 장식이다. 캐릭터는 포함하지 않는다.

## 자원과 적용

- 생성 방식: 내장 imagegen, 투명 배경, 6종을 한 아틀라스로 생성.
- 원본: `Content/Asset/MineDecorV1/Source/atlas-original.png` (1536×1024).
- 개별 PNG와 규격: 같은 Source 폴더의 `manifest.json`.
- MineEntrance 96×96, Minecart 64×48, SupplyCrates 64×48, LanternPost 32×80, ToolRack 64×64, RailSegment 96×32.
- 엔진 텍스처·스프라이트: `/Game/Asset/MineDecorV1/T_<역할>`, `S_<역할>`.
- 공통 참조: `/Game/Asset/ArtV1/DA_VisualStyle.MineDecorations`.
- 지형과 동일하게 2px/월드 단위. Nearest, NoMipmaps, Pixels2D, hard alpha, MaskedUnlit, 충돌 없음.
- Windows PowerShell Desktop에서 `AGENT/Scripts/PrepareMineDecor.ps1`을 실행하면 원본 아틀라스에서 알파 경계 제거·크롭·정수 픽셀 자원을 재구성한다. 에디터에서 `ApplyMineDecor.py`를 실행해 임포트와 공통 참조를 갱신한다.
- 런타임 장식은 바닥 블록에 부착되고 플레이어보다 뒤에 표시한다. 지형 생성 설정과 검증은 `AGENT/Design/ProceduralMine.md`를 따른다.

## 생성 프롬프트

```text
Use case: stylized-concept. Asset type: one production sprite atlas for ProjectMK, a side-view 2D pixel-art mining game. Create a single transparent sprite sheet, landscape aspect ratio 3:2, organized in EXACTLY 3 columns and 2 rows of equal 128x128 LOGICAL pixel cells (384x256 logical total), shown at crisp integer enlargement if needed. No visible grid, labels or cell dividers. All six props isolated and fully separated by transparent gutters. Each cell has a consistent bottom baseline twelve logical pixels above its bottom. Row 1 from left to right: (1) an open wooden mine-entrance support frame, 96x96 logical pixels, heavy aged timber posts and crossbeam with restrained iron brackets, empty transparent opening; (2) a low side-view minecart carrying matte broken gray stones, 64x48 logical pixels, two readable iron wheels; (3) a group of two weathered wooden supply crates and one small plain barrel, 64x48 logical pixels. Row 2 from left to right: (4) a slim timber lantern post with one modest small amber lantern hanging from its arm, 32x80 logical pixels, no large glow; (5) a small wooden tool stand with a pickaxe, shovel and coil of rope, 64x64 logical pixels; (6) a short side-view floor rail segment, 96x32 logical pixels, worn sleepers and dark iron rail, no perspective vanishing. Use flat front-facing orthographic SIDE VIEW suitable for a platform mining game, absolutely no isometric or top-down view. Art direction: match handmade restrained pixel art, irregular intentional pixel clusters, sturdy practical silhouettes, muted dark brown timber, charcoal/slate-gray metal, dusty beige rope, small desaturated brass fittings, very sparse muted teal details. Materials have 3-5 stepped shades and modest upper-left lighting. Avoid pure-black outline halos, polished highlights, glossy 3D, smooth gradients, blur, antialiasing and subpixel detail. Pixel density reference: a terrain block is 32x32 logical pixels, one block wide in game, so the minecart is two blocks wide and the mine entrance is three blocks wide. Preserve native logical pixel density across prop sizes. No ground plane under any prop, no soil texture, no background, no scenic composition, no text, letters, numbers, logos, signs, people, animals or characters. Transparent alpha everywhere outside each prop and inside the entrance opening. Fill only the specified bounded sprite areas within their cells, leaving generous transparent separation.
```

2026-11-15부터 SupplyCrates의 런타임 배치 지점은 실제 보관 상자로 대체한다. 원본 아틀라스는 유지하지만 이 장식 스프라이트를 보관함처럼 배치하지 않는다. 전용 상자 자원은 [STORAGE_CHEST_V2.md](STORAGE_CHEST_V2.md)를 따른다.
