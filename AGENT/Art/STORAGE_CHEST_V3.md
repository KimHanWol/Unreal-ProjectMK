# 보관 상자 V3

기존 광산 상자 더미와 광차 이미지를 내장 imagegen의 스타일 참고로 전달해 새 닫힌 상자를 생성했다. V2 원본은 보존한다.

- 참고: `Content/Asset/MineDecorV1/Source/SupplyCrates.png`, `Minecart.png`.
- 자원: `Content/Asset/StorageChestV3/Source/chest-original.png`, `StorageChest.png`.
- 네이티브 32×32, 본체 최대 28×22px, 바닥을 마지막 행에 정렬. 투명 바닥 여백 없음.
- Nearest, NoMipmaps, hard alpha, MaskedUnlit, 2px/월드 단위.
- 재적용: `PrepareStorageChestV3.ps1`, `ApplyStorageChestData.py`.
- 런타임 렌더는 실제 캐릭터보다 뒤쪽에 두며 충돌·상자 위치·낙하 좌표는 변경하지 않는다.

## 생성 프롬프트

```text
Use case: stylized-concept. ONE closed wooden storage chest sprite for a side-view mining game. Match the supplied crate pile and minecart references: muted weathered tan-brown planks, slate-gray iron, sparse desaturated brass, restrained contrast and handmade irregular pixel clusters. Compact practical rectangular chest, closed flat lid and subtle seam, two thin worn iron straps, one small latch, softly worn corners. Front-facing orthographic side view with only a tiny hint of lid thickness. 32x32 logical canvas, body about28x22px, continuous solid base, no feet. Hard alpha transparent background and tight crop. Three to five stepped shades and modest upper-left lighting. No shiny gold bands, orange-red wood, glow, smooth3D, antialiasing, blur, gradients, white highlights, black halo, scene, ground, shadow, text, icons or characters.
```
