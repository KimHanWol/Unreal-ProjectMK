# 광산 시설·운송 아트 개정

내장 imagegen으로 기존 `TownArtV2/Source/HouseA.png`와 공통 픽셀 아트 지침을 참고하여 제작했다. 캐릭터·새 몬스터 디자인은 추가하지 않는다. `PrepareMineRevision.ps1`는 원본 보존, 분리, Nearest 축소, hard alpha만 처리하며 `ApplyMineRevision.py`가 재사용 스프라이트·블록·아이템·UI에 등록한다.

## 원본과 적용 위치

모든 원본과 게임 PNG는 `Content/Asset/MineRevisionV1/Source/`에 둔다. 런타임 자산은 `/Game/Asset/MineRevisionV1/`이다.

| 파일 | 크기 | 용도 |
|---|---|---|
| SupportBeam.png | 32×32 | 벽에 밀착해 이어지는 밝은 X형 버팀목, 열린 바닥 |
| Rail.png | 32×12 | 한 칸 철도, 연결 경계까지 렌더 |
| RailTrestleDown/Up.png | 각각 32×32 | 경사 철도 아래 목재 받침. 실제 버팀목이 같은 칸이면 중복 표시하지 않음 |
| MineCart/AutoMineCart.png | 각각 48×32 | 일반·전자동 수레, 철도와 다른 깊이 |
| LumberyardEnclosure.png | 768×80 | 세 나무를 포함하는 하나의 긴 전경 울타리·재배판 |
| TreeLeaves.png | 32×32 | 동일 이미지가 반복되는 사각 잎 블록. 기존 영역 키도 이 그림을 공유 |
| BountyBoard.png | 512×384 | 2×2 의뢰지가 붙은 전용 팝업 벽보. 내용은 실제 위젯으로 표시 |
| ForestBackdrop.png | 768×256 | 벌목장 뒤 나무 군락 |

원본 파일은 transport/yard/leaves/board/forest/trestle-original.png이다. `S_TreeStatus`는 재배 시설 텍스처의 상태등 렌즈 영역(141,10에서 8×10px)을 그대로 재사용하고 녹색·적색·황색으로 표시한다. 드릴 길이 HUD는 기존 DrillRange 아이콘과 공통 프레임을 재사용한다.

## 최종 생성 프롬프트 세트

공통: Create cohesive orthographic side-view 2D pixel-art mining game assets matching the supplied HouseA reference. Warm medium oak, muted desaturated iron, earthy stepped shades, crisp intentional pixel clusters, readable brighter timber. No perspective, characters, lettering, watermarks, blur, antialiasing, smooth glossy rendering or bright outline halos. Transparent exterior PNG except the full-coverage leaf texture. Keep native pixel density: one block is 32 pixels at two pixels per world unit.

- transport: Four isolated atlas objects in a 2×2 grid: upper left square inverted-U timber support with top beam, side posts and thin X braces, NO bottom beam; upper right one-block straight railway without ground; lower left empty manual wooden mine cart; lower right matching automatic cart with a small muted teal motor. Separate rails and carts. Native sizes 32×32, 32×12, 48×32, 48×32.
- yard: One continuous long low timber planting enclosure, three evenly spaced open bays for trees to grow behind the foreground rail, one small neutral status-lamp housing beside each bay. Connected trough, iron hardware, large transparent space above the low fence. No trees, building, ground or characters. Logical 768×80.
- leaves: One seamless repeatable 32×32 square leaf block, full opaque coverage to all four boundaries, matching olive green clustered leaves. No rounded bush silhouette, transparent corners, border, perimeter rim, bevel, gaps or branches. Opposite edges should tile horizontally and vertically.
- board: One flat front wooden quest-board UI with FOUR blank cream parchment notices in exactly two columns and two rows. Narrow oak/iron outer frame, comfortable gutters, large blank text interiors, small blank portrait reserve at upper left of each paper. No generated writing, title banner, icons or characters. Logical 512×384.
- forest: One wide quiet background grove, closely grouped broadleaf and conifer trees, varied heights, desaturated blue olive green, lower contrast than the foreground, transparent sky, sparse fence and stacked logs near the bottom. No ground rectangle, buildings or characters. Logical 768×256.
- trestle: A matching pair of side-view timber frameworks, top edges descending right and descending left, tall outer post and shorter inner post, thin X bracing and open bottom. Match the support atlas wood and iron. No rails or ground. Each fits one 32×32 block.

생성 결과는 기본 생성 폴더에서 프로젝트로 복사하여 보존했다. 반투명 외곽과 흰색 매트가 남지 않도록 alpha threshold와 밝은 매트 제거를 적용했다. UI 벽보는 비율을 유지해 표시하며 글씨와 버튼을 이미지에 굽지 않는다.
