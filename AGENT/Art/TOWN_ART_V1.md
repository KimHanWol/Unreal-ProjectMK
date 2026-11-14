# 지상 시설·철도 프로토타입 아트

내장 imagegen으로 3×3 아틀라스를 생성했다. 캐릭터와 NPC 인물 이미지는 만들지 않았다. 기존 보급소와 광차를 스타일 참고로 사용했다.

- 참고: `Content/Asset/WorldArtV3/Source/Shop.png`, `Content/Asset/MineDecorV1/Source/Minecart.png`.
- 원본: `Content/Asset/TownArtV1/Source/atlas-original.png`.
- 분할·배치 규격: 같은 폴더의 `manifest.json`.
- 가공: `AGENT/Scripts/PrepareTownArt.ps1`.
- 임포트·등록: `AGENT/Scripts/ApplyTownArt.py`. VisualStyle의 TownSprites 맵과 Item/Block/ShopRecipe DataTable을 함께 갱신한다.

| 자원 | 네이티브 해상도 | 용도 |
|---|---|---|
| Elevator | 64×96 | 지상·지하 승강기 정거장 |
| RailFactory | 128×96 | 철도 공장 |
| HouseA / HouseB | 각 96×96 | 광부·정비공 주택 |
| MineCart / AutoMineCart | 각 48×32 | 수동·전자동 광차 |
| Rail | 32×8 | 수평·대각선 선로 |
| RailEnd | 8×8 | 연결되지 않는 선로 끝 |
| SteadyBelt | 24×24 | 광부의 중심추 유물 배지 |

건물·광차는 크기에 맞는 해상도를 사용하며 모든 자원을 32×32로 강제하지 않는다. 공통 2px/월드 단위, Nearest, NoMipmaps, MaskedUnlit을 사용한다. 투명한 바닥 여백을 제거하고 크기가 작은 자원은 캔버스 하단에 정렬한다. 별도 곡선 철도 전환 스프라이트는 이번 프로토타입에 포함하지 않는다.

## 생성 프롬프트 핵심

```text
A transparent 3x3 sprite atlas for a side-view pixel-art mining game. Match the supplied weathered wooden shop and minecart: muted tan-brown timber, slate iron, sparse dull brass and teal details, coarse readable pixel clusters, restrained contrast. Each cell contains one isolated front-facing orthographic sprite, with generous transparent gutters and a solid bottom-aligned base. Row 1: a narrow timber mining elevator, a wide railway workshop, a small miner's house. Row 2: a different mechanic's house, a heavy manual minecart, a matching motorized minecart with a compact engine. Row 3: a short horizontal rail, a separate rail end stop, a compact miner's stabilizing belt relic icon. No people, characters, labels, letters, scene, ground shadows, glossy 3D, gradients, glow or antialiased outlines. Buildings have larger logical pixel canvases than small icons while retaining a similar pixel scale.
```
