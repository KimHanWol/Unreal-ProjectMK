# Forestry and bounty board polish assets

Built-in image_gen was used. References: `Content/Asset/TownArtV2/Source/HouseA.png`, `Content/Asset/WorldArtV3/Source/Shop.png` and `AGENT/Art/COMMON_PIXEL_ART_PROMPT.md`.

Separate `BountyBoard` (768×432) and `BountyPaper` (320×192) are composed by widgets. The paper is opaque; only its exterior white matte is keyed out. Board and paper use nine-slice margins. A single low enclosure (768×20) is grounded without burying it; two lamp state sprites (16×20) are independent of the fence. SupportBeam is cropped from its existing 2-pixel top padding and fitted to 32×32. Existing tree textures use the terrain neighbor silhouette, preserving connected interiors and item overlays. All world sprites use 2 pixels per unit.

Final prompts:

## boardPrompt

```text
Use case: stylized-concept. Asset type: a single empty bounty notice BOARD background for a 2D mining game UI, landscape aspect ratio 1.8:1. Reference images are style references only: match muted honey-brown timber, dark iron corner brackets, crisp pixel clusters of the supplied house and shop. Create ONLY the bare wide rectangular wooden pinboard, close front orthographic view, tightly cropped. A slim sturdy wood frame with small dull iron corner brackets surrounds a broad EMPTY medium warm wood plank surface. Plenty of quiet usable interior for four separate parchment widgets to be overlaid by code later. NO paper, no notices, no pins inside, no text, no characters, no icons, no scenery, no stand or legs. Native logical art size 768 by 432, keep pixel density appropriate to this large object, hard edges stepped clusters 3-5 shades per material, modest upper left light, not glossy, no blur or antialiasing, no black halo. Transparent background outside the board. Do not copy reference architecture.
```

## paperOpaquePrompt

```text
Create ONE solid opaque rectangular sheet of blank cream parchment on a PURE WHITE (#ffffff) background. This is a blank bounty notice sprite for a 2D pixel art mining game. Wide landscape aspect 1.65:1. Light ivory tan paper, entire center and entire sheet are solid opaque pale cream, no holes, no windows, no transparent interior. Small pixel ragged outer contour, subtle curled bottom corners, small gray iron tacks top two corners. The center is completely empty flat light cream for black dynamic text. Do NOT depict anything inside it. No gradient, no shadows across the center, no vignette, no text. Native logical art320x192, crisp chunky pixel clusters, 3-5 shades only, subtle darker edge, match muted earthy fantasy mining shop textures. White backdrop only outside the paper. It is a PAPER SHEET not a picture frame.
```

## fencePrompt

```text
Use case: stylized-concept. Asset type: ONE long low wooden forestry plot fence, side view sprite for a pixel mining game. Use supplied existing mining house/shop as STYLE ONLY. A single uninterrupted horizontal fence spanning three adjoining tree plots, width to height about 24:1, logical sprite 768x32 pixels. LOW slender waist-height open railing, two thin horizontal honey-brown timber rails and 7 evenly spaced slim posts with small dark steel bolts. Feet terminate at same straight bottom ground line. No tall signposts, lamps, trees, buildings, platform, deep solid skirt, underground foundation or contents. Air visible between rails. This is foreground fence which should only cover a character's lower legs. Crisp pixel clusters and muted earthy stepped timber shades matching reference. No glossy render, no blur, no text, transparent outside rails. Long slim horizontal composition.
```

## lampPrompt

```text
Use case: stylized-concept. Asset type: a TWO-STATE sprite atlas of the SAME compact forestry status indicator lamp, two sprites separated widely, left GREEN lit glass, right RED lit glass. Identical size/geometry in both. Each logical sprite 16x20 pixels. Short sturdy bronze/dark iron bolted mining instrument housing, an octagonal jewel-like thick glass lens in center, beveled rim, tiny upper-left glass glint, small foot mounting bracket. Actual faceted ROUND/OCTAGONAL glass lamp, NOT a plain colored square. The green lens uses mossy bright green and the red muted ruby red, legible at tiny world size; restrained highlight, no bloom. Front orthographic view, two separate sprites side by side with clear transparent gap; no pole, no fence, no captions, no labels, no text or symbols. Match supplied mining house's warm wood and gray iron pixel art, crisp chunky hard pixel shapes with 3-5 stepped shades, no blur. Transparent background.
```

Raw originals and prepared sources: `Content/Asset/ForestryBoardV2/Source/`.
Preparation: `AGENT/Scripts/PrepareForestryBoardV2.ps1`; editor import: `AGENT/Scripts/ApplyForestryBoardV2.py`.
