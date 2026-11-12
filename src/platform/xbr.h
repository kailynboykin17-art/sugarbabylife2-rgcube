// Pixel-art smoothing for the RG Cube port: an xBR-style 2x scaler (run twice for 4x).
// Finds the stair-step edges in a sprite and fills them in along the edge direction,
// so outlines become clean curves and diagonals while flat colours stay untouched.
#pragma once
#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>

namespace xbr {
// pixels are 0xAABBGGRR (R in the low byte)
inline float diff(uint32_t a, uint32_t b) {
    const int ar = a & 255, ag = a >> 8 & 255, ab = a >> 16 & 255, aa = a >> 24;
    const int br = b & 255, bg = b >> 8 & 255, bb = b >> 16 & 255, ba = b >> 24;
    const float y = 0.299f * (ar - br) + 0.587f * (ag - bg) + 0.114f * (ab - bb);
    const float u = -0.169f * (ar - br) - 0.331f * (ag - bg) + 0.5f * (ab - bb);
    const float v = 0.5f * (ar - br) - 0.419f * (ag - bg) - 0.081f * (ab - bb);
    return 48 * std::fabs(y) + 7 * std::fabs(u) + 6 * std::fabs(v) + 40 * std::fabs((float)(aa - ba));
}
inline bool same(uint32_t a, uint32_t b) { return diff(a, b) < 155; }
inline uint32_t blend(uint32_t a, uint32_t b, float t) {   // t = amount of b
    uint32_t r = 0;
    for (int s = 0; s < 32; s += 8) {
        const float x = (a >> s & 255) * (1 - t) + (b >> s & 255) * t;
        r |= (uint32_t)std::clamp((int)(x + 0.5f), 0, 255) << s;
    }
    return r;
}
// 2x upscale, w x h -> 2w x 2h
inline std::vector<uint32_t> scale2(const std::vector<uint32_t>& src, int w, int h) {
    auto P = [&](int x, int y) { x = std::clamp(x, 0, w - 1); y = std::clamp(y, 0, h - 1); return src[(size_t)y * w + x]; };
    std::vector<uint32_t> out((size_t)w * h * 4);
    const int W2 = w * 2;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        uint32_t o[4]; for (uint32_t& q : o) q = P(x, y);   // 0 TL, 1 TR, 2 BL, 3 BR
        // the four corners are the same rule rotated: (dx,dy) points at the corner
        for (int c = 0; c < 4; c++) {
            const int dx = (c & 1) ? 1 : -1, dy = (c & 2) ? 1 : -1;
            // local frame: "F" = horizontal neighbour toward the corner, "H" = vertical one, "I" = the diagonal
            auto L = [&](int i, int j) { return P(x + i * dx, y + j * dy); };   // i along x toward corner, j along y toward corner
            const uint32_t E = L(0, 0), F = L(1, 0), H = L(0, 1), I = L(1, 1);
            const uint32_t B = L(0, -1), D = L(-1, 0), C = L(1, -1), G = L(-1, 1);
            const uint32_t F4 = L(2, 0), H5 = L(0, 2), I4 = L(2, 1), I5 = L(1, 2);
            if (E == F || E == H) continue;
            const float e = diff(E, C) + diff(E, G) + diff(I, F4) + diff(I, H5) + 4 * diff(H, F);
            const float i = diff(H, D) + diff(H, I5) + diff(F, I4) + diff(F, B) + 4 * diff(E, I);
            if (!(e < i)) continue;
            const uint32_t px = diff(E, F) <= diff(E, H) ? F : H;
            const float ke = diff(F, G), ki = diff(H, C);
            const bool shallow = ke * 2 <= ki && E != G && D != G;
            const bool steep = ki * 2 <= ke && E != C && B != C;
            // subpixels in local frame: corner = (1,1); along x = (1,0)->"side", along y = (0,1)
            const int corner = c, sideX = c ^ 1, sideY = c ^ 2;   // sideX: same row, other half; sideY: same column, other half
            if (shallow && steep) { o[corner] = blend(o[corner], px, 0.875f); o[sideX] = blend(o[sideX], px, 0.375f); o[sideY] = blend(o[sideY], px, 0.375f); }
            else if (shallow) { o[corner] = blend(o[corner], px, 0.75f); o[sideX] = blend(o[sideX], px, 0.25f); }
            else if (steep) { o[corner] = blend(o[corner], px, 0.75f); o[sideY] = blend(o[sideY], px, 0.25f); }
            else o[corner] = blend(o[corner], px, 0.5f);
        }
        uint32_t* r0 = &out[(size_t)(y * 2) * W2 + x * 2]; uint32_t* r1 = r0 + W2;
        r0[0] = o[0]; r0[1] = o[1]; r1[0] = o[2]; r1[1] = o[3];
    }
    return out;
}
inline std::vector<uint32_t> scale4(const std::vector<uint32_t>& src, int w, int h) { return scale2(scale2(src, w, h), w * 2, h * 2); }
}   // namespace xbr
