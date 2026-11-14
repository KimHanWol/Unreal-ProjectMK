# 보관 상자 V2

기존 `COMMON_PIXEL_ART_PROMPT.md`와 `MINE_DECOR_V1.md`의 나무·철·황동 팔레트를 따른다. 이전의 열린 석탄 보관함 아이콘 대신 닫힌 뚜껑과 잠금쇠가 읽히는 보관 상자를 사용한다. 캐릭터는 포함하지 않는다.

## 자원·재적용

- 내장 imagegen으로 전용 상자 한 개를 생성했다. 투명 배경 원본은 `Content/Asset/StorageChestV2/Source/chest-original.png`다.
- `AGENT/Scripts/PrepareStorageChestV2.ps1`은 원본을 hard alpha·Nearest 방식으로 정리해 `Source/StorageChest.png` 32×32에 배치한다. 실제 상자 폭 28px, 바닥 여백 2px.
- UE에서 `AGENT/Scripts/ApplyStorageChestData.py`로 `/Game/Asset/StorageChestV2/T_StorageChest`, `S_StorageChest`를 임포트한다.
- 텍스처: Nearest, NoMipmaps, Pixels2D, EditorIcon. 스프라이트: 2px/월드 단위, MaskedUnlit, SourceBoundingBox, 자체 Paper2D 충돌 없음.
- ItemDataTable.StorageChest, BlockDataTable.StorageChest, VisualStyle.IconStorage에서 공통 사용한다. 필드 충돌과 낙하는 별도 블록 몸체가 담당한다.
- 제작 시 기존 다른 에셋·캐릭터 이미지를 변경하지 않았다.

## 생성 프롬프트

```text
Use case: stylized-concept. Production sprite for ProjectMK, a 2D SIDE VIEW mining game. One CLOSED front-facing orthographic wooden storage chest. 32x32 logical pixel grid at crisp integer enlargement, one isolated chest on transparent background, tight square crop. About 28 pixels wide and 24 pixels high, centered, bottom margin 2 logical pixels. Closed timber lid with clear seam, two dark iron straps, one small brass latch, dark walnut irregular grain, integrated small feet. Muted brown, slate iron and desaturated brass. Three to five stepped shades with modest upper-left light. Reusable storage chest, not a crate pile, coal bin, cart, open chest or gold treasure pile. Hard alpha and deliberate crisp pixel clusters. No antialiasing, blur, bloom, smooth gradient, glossy 3D, microdetail, white highlights, black halo, isometric perspective, scene, text, characters, floor or cast shadow. Match the pixel density of 32x32 terrain blocks.
```

32×32로 정리한 뒤 윤곽·알파·잠금쇠의 가독성을 확인하고 엔진에 적용했다.
