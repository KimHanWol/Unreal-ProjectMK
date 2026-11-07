// Full 32px cells only. All operations stay inside the outer four pixels.
float2 p = min(floor(saturate(UV) * 32.0), 31.0);
float4 d = float4(p.x, 31.0 - p.x, p.y, 31.0 - p.y);
// Keep short, uneven teeth; bevel only their immediate shoulders instead of smoothing whole lobes.
float4 sideSeed = EdgeSeed + float4(5, 13, 29, 41);
float4 phase = floor(frac(sin(sideSeed * 2.319) * 22578.1459) * 8.0);
float4 along = float4(p.y, p.y, p.x, p.x) + phase;
float4 profile = 0.0;
[unroll]
for (int offset = -1; offset <= 1; ++offset) {
    float4 samplePosition = along + float(offset);
    float4 group = floor(samplePosition / 8.0);
    float4 split = 2.0 + floor(frac(sin(group * 7.137 + sideSeed) * 22578.1459) * 4.0);
    float4 segment = group * 2.0 + step(split, samplePosition - group * 8.0);
    float4 height = 1.0 + floor(frac(sin(segment * 12.9898 + sideSeed) * 43758.5453) * 4.0);
    profile += height * (offset == 0 ? 0.4 : 0.3);
}
float4 inset = floor(profile + 0.5);
float4 edges = float4(Left, Right, Top, Bottom);
float4 fills = float4(FillLeft, FillRight, FillTop, FillBottom);
float4 cut = edges * (1.0 - step(inset, d));
float4 emptyCut = cut * (1.0 - fills);
float coverage = 1.0 - max(max(cut.x, cut.y), max(cut.z, cut.w));
float emptyCoverage = 1.0 - max(max(emptyCut.x, emptyCut.y), max(emptyCut.z, emptyCut.w));
float4 color = Source;
float nearest = 100.0;
bool filled = false;
if (cut.x * fills.x > 0.5 && d.x < nearest) {
    color = Texture2DSample(LeftTexture, LeftTextureSampler, float2(1.0 - UV.x, UV.y));
    nearest = d.x; filled = true;
}
if (cut.y * fills.y > 0.5 && d.y < nearest) {
    color = Texture2DSample(RightTexture, RightTextureSampler, float2(1.0 - UV.x, UV.y));
    nearest = d.y; filled = true;
}
if (cut.z * fills.z > 0.5 && d.z < nearest) {
    color = Texture2DSample(TopTexture, TopTextureSampler, float2(UV.x, 1.0 - UV.y));
    nearest = d.z; filled = true;
}
if (cut.w * fills.w > 0.5 && d.w < nearest) {
    color = Texture2DSample(BottomTexture, BottomTextureSampler, float2(UV.x, 1.0 - UV.y));
    filled = true;
}
float4 rim = edges * (1.0 - fills) * (1.0 - step(inset + 1.0, d)) * (1.0 - step(4.0, d));
float shade = 1.0 - 0.06 * max(max(rim.x, rim.y), max(rim.z, rim.w));
return float4(color.rgb * Tint.rgb * shade, color.a * TintAlpha * (filled ? emptyCoverage : coverage));
