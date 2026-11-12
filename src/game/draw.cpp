#include "draw.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

static C2D_TextBuf g_tbuf = nullptr;
static C2D_TextBuf g_mbuf = nullptr;   // scratch buffer for measuring text
static const float Z = 0.5f;

RGB shade(const RGB& c, float f) {
    auto ch = [f](int v) {
        float r = f <= 1.f ? v * f : v + (255 - v) * (f - 1.f);
        return (uint8_t)clampv((int)r, 0, 255);
    };
    return RGB{ch(c.r), ch(c.g), ch(c.b)};
}
RGB mix(const RGB& a, const RGB& b, float t) {
    return RGB{(uint8_t)(a.r + (b.r - a.r) * t), (uint8_t)(a.g + (b.g - a.g) * t), (uint8_t)(a.b + (b.b - a.b) * t)};
}

static void pixInit();
// ---- Flip Phone bitmap font: a 12px non-antialiased font in a nearest-filtered texture atlas
#include "pxfont.h"
static inline bool gpuRoom();
static C3D_Tex g_pxTex;
static bool g_pxOk = false;
static Tex3DS_SubTexture g_pxSub[95];
static void pxFontInit() {
    if (!C3D_TexInit(&g_pxTex, 256, 128, GPU_RGBA8)) return;
    C3D_TexSetFilter(&g_pxTex, GPU_NEAREST, GPU_NEAREST);
    u32* px = (u32*)g_pxTex.data;
    memset(px, 0, 256 * 128 * 4);
    for (int gi = 0; gi < 95; gi++) {
        int cx = (gi % 16) * 14, cy = (gi / 16) * 14;
        for (int y = 0; y < 12; y++) for (int x = 0; x < 12; x++) {
            if (!(PXF_ROWS[gi][y] >> x & 1)) continue;
            int X = cx + x, Y = cy + y;   // 8x8 morton tiles, row 0 = top
            u32 off = ((Y >> 3) * (256 >> 3) + (X >> 3)) * 64;
            u32 m = (X & 1) | ((Y & 1) << 1) | ((X & 2) << 1) | ((Y & 2) << 2) | ((X & 4) << 2) | ((Y & 4) << 3);
            px[off + m] = 0xFFFFFFFF;
        }
        int w = PXF_W[gi];
        g_pxSub[gi] = {(u16)w, 12, cx / 256.f, 1.f - cy / 128.f, (cx + w) / 256.f, 1.f - (cy + 12) / 128.f};
    }
    GSPGPU_FlushDataCache(g_pxTex.data, g_pxTex.size);
    g_pxOk = true;
}
extern int g_artStyle;
static inline bool pxText() { return false; }   // the tiny bitmap font was too hard to read: always use the system font
static inline int pxDot(float sc) { return sc >= 0.62f ? 2 : 1; }
static float pxWidth(const char* s, int dot) { float w = 0; for (; *s; s++) { unsigned char ch = (unsigned char)*s; w += (ch >= 32 && ch < 127 ? PXF_W[ch - 32] : PXF_W[0]) * dot; } return w; }
static void pxDraw(const char* s, float x, float y, float sc, u32 c, int align) {
    const int dot = pxDot(sc);
    float w = pxWidth(s, dot);
    if (align == 1) x -= w / 2; else if (align == 2) x -= w;
    x = std::floor(x); y = std::floor(y + 15 * sc - 6 * dot);
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, c, 1.0f);
    for (; *s; s++) {
        if (!gpuRoom()) return;
        unsigned char ch = (unsigned char)*s;
        int gi = (ch >= 32 && ch < 127) ? ch - 32 : 0;
        if (gi) { C2D_Image img = {&g_pxTex, &g_pxSub[gi]}; C2D_DrawImageAt(img, x, y, Z, &tint, (float)dot, (float)dot); }
        x += PXF_W[gi] * dot;
    }
}
void drawInit() { g_tbuf = C2D_TextBufNew(8192); g_mbuf = C2D_TextBufNew(512); pixInit(); pxFontInit(); }
void drawExit() { if (g_tbuf) C2D_TextBufDelete(g_tbuf); if (g_mbuf) C2D_TextBufDelete(g_mbuf); }
// Each layer texture is cleared once per frame (clearing a texture that was already
// drawn from earlier in the same frame crashes the GPU queue). A layer first used on
// one screen isn't reused on the other screen that frame - the sprite is drawn directly.
static int g_ssClearedOn[2] = {0, 0}, g_pixClearedOn[2] = {0, 0}, g_screenNo = 0;
void drawFrameBegin() { C2D_TextBufClear(g_tbuf); g_ssClearedOn[0] = g_ssClearedOn[1] = g_pixClearedOn[0] = g_pixClearedOn[1] = 0; g_screenNo = 0; }

// Ink mode: every opaque shape is drawn in one flat colour (used to build crisp
// character outlines); translucent shapes (blush, shine...) are skipped.
u32 g_ink = 0;
// Safety valve: if the GPU command buffer is nearly full this frame, skip extra
// shapes instead of letting the console panic. (A crowded scene just loses a few details.)
static inline bool gpuRoom() { return !gpuCmdBuf || gpuCmdBufOffset + 2048 < gpuCmdBufSize; }
static inline bool ink(u32& c) { if (!gpuRoom()) return false; if (!g_ink) return true; if ((c >> 24) < 170) return false; c = g_ink; return true; }

// ---- pixel-art pass ---------------------------------------------------------
// The scene is drawn at half resolution into an offscreen texture (view scaled
// by 0.5, so callers keep using 400x240 coordinates), then blown up 2x with
// nearest-neighbour filtering: every shape lands on a crisp 2x2 pixel grid.
// While it's active, hairline details are fattened to at least one pixel.
bool g_pix = false;
static C3D_Tex g_pixTex[2];
static C3D_RenderTarget* g_pixTgt[2] = {nullptr, nullptr};
static C3D_RenderTarget* g_screen = nullptr;
static int g_pixSlot = 0;
void drawSetScreen(C3D_RenderTarget* t) { g_screen = t; g_screenNo++; }
// ---- smooth (anti-aliased) layer: drawn at 2x into a 1024x512 texture, then
// shrunk back with bilinear filtering, which averages each 2x2 block = 4x SSAA.
bool g_layer = false;
// two layers: slot 0 = the whole scene, slot 1 = a sprite/portrait drawn later in the same
// frame (re-using one texture twice per frame is what crashed travelling into town)
static C3D_Tex g_ssTex[2];
static C3D_RenderTarget* g_ssTgt[2] = {nullptr, nullptr};
static bool g_ss = false;
static int g_ssSlot = 0;
static void ssInit() {
    for (int i = 0; i < 2; i++) {
        if (!C3D_TexInitVRAM(&g_ssTex[i], 1024, 512, GPU_RGBA8)) continue;
        C3D_TexSetFilter(&g_ssTex[i], GPU_LINEAR, GPU_LINEAR);
        g_ssTgt[i] = C3D_RenderTargetCreateFromTex(&g_ssTex[i], GPU_TEXFACE_2D, 0, (GPU_DEPTHBUF)-1);
    }
}
static void pixInit() {
    ssInit();
    for (int i = 0; i < 2; i++) {
        if (!C3D_TexInitVRAM(&g_pixTex[i], 256, 128, GPU_RGBA8)) continue;
        C3D_TexSetFilter(&g_pixTex[i], GPU_NEAREST, GPU_NEAREST);
        g_pixTgt[i] = C3D_RenderTargetCreateFromTex(&g_pixTex[i], GPU_TEXFACE_2D, 0, (GPU_DEPTHBUF)-1);
    }
}
static bool ssBegin(int slot) {
    if (slot < 0 || slot > 1 || !g_ssTgt[slot] || !g_screen) return false;
    g_ssSlot = slot;
    if (g_ssClearedOn[slot] && g_ssClearedOn[slot] != g_screenNo) return false;
    if (!g_ssClearedOn[slot]) { C2D_TargetClear(g_ssTgt[slot], 0); g_ssClearedOn[slot] = g_screenNo; }
    C2D_SceneBegin(g_ssTgt[slot]);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    C2D_ViewReset();
    C2D_ViewScale(2.f, 2.f);
    g_ss = true; g_layer = true;
    return true;
}
static void ssEnd(float x0, float y0, float x1, float y1) {
    if (!g_ss) return;
    g_ss = false; g_layer = false;
    C2D_SceneBegin(g_screen);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
    C2D_ViewReset();
    int px0 = clampv((int)std::floor(x0), 0, 400), py0 = clampv((int)std::floor(y0), 0, 240);
    int px1 = clampv((int)std::ceil(x1), 0, 400), py1 = clampv((int)std::ceil(y1), 0, 240);
    if (px1 <= px0 || py1 <= py0) return;
    Tex3DS_SubTexture sub = {(u16)((px1 - px0) * 2), (u16)((py1 - py0) * 2), px0 * 2 / 1024.f, 1.f - py0 * 2 / 512.f, px1 * 2 / 1024.f, 1.f - py1 * 2 / 512.f};
    C2D_Image img = {&g_ssTex[g_ssSlot], &sub};
    if (g_ssSlot == 0) {
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
        C2D_DrawImageAt(img, (float)px0, (float)py0, Z, nullptr, 0.5f, 0.5f);
        C2D_Flush();
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
    } else C2D_DrawImageAt(img, (float)px0, (float)py0, Z, nullptr, 0.5f, 0.5f);
}
bool artLayerBegin(int slot, int style) {
    if (g_layer) return false;
    if (style == 3 || style == 5 || style == 6) return pixBegin(slot);
    if (style == 11) return slot == 0 ? pixBegin(slot) : false;   // Pocket City: the whole scene is chunky pixels, figures are pixel-exact
    if (style == 4 || style >= 7) return ssBegin(slot);   // HD Glam & the stick styles: smooth anti-aliased lines
    return false;
}
void artLayerEnd(float x0, float y0, float x1, float y1) {
    if (g_ss) ssEnd(x0, y0, x1, y1);
    else if (g_pix) pixEnd(x0, y0, x1, y1);
}

bool pixBegin(int slot) {
    if (slot < 0 || slot > 1 || !g_pixTgt[slot] || !g_screen) return false;
    g_pixSlot = slot;
    if (g_pixClearedOn[slot] && g_pixClearedOn[slot] != g_screenNo) return false;
    if (!g_pixClearedOn[slot]) { C2D_TargetClear(g_pixTgt[slot], 0); g_pixClearedOn[slot] = g_screenNo; }
    C2D_SceneBegin(g_pixTgt[slot]);
    // keep the layer's alpha solid where translucent shapes land on opaque ones
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);   // the layer has no depth buffer
    C2D_ViewReset();
    C2D_ViewScale(0.5f, 0.5f);
    g_pix = true; g_layer = true;
    return true;
}
void pixEnd(float x0, float y0, float x1, float y1) {
    if (!g_pix) return;
    g_pix = false; g_layer = false;
    C2D_SceneBegin(g_screen);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
    C2D_ViewReset();
    int tx0 = clampv((int)std::floor(x0 / 2), 0, 200), ty0 = clampv((int)std::floor(y0 / 2), 0, 120);
    int tx1 = clampv((int)std::ceil(x1 / 2), 0, 200), ty1 = clampv((int)std::ceil(y1 / 2), 0, 120);
    if (tx1 <= tx0 || ty1 <= ty0) return;
    Tex3DS_SubTexture sub = {(u16)(tx1 - tx0), (u16)(ty1 - ty0), tx0 / 256.f, 1.f - ty0 / 128.f, tx1 / 256.f, 1.f - ty1 / 128.f};
    C2D_Image img = {&g_pixTex[g_pixSlot], &sub};
    if (g_pixSlot == 0) {   // a full scene: stamp it opaque so stray alpha can't punch holes
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
        C2D_DrawImageAt(img, tx0 * 2.f, ty0 * 2.f, Z, nullptr, 2.f, 2.f);
        C2D_Flush();
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
    } else C2D_DrawImageAt(img, tx0 * 2.f, ty0 * 2.f, Z, nullptr, 2.f, 2.f);
}

void rect(float x, float y, float w, float h, u32 c) { if (w > 0 && h > 0 && ink(c)) C2D_DrawRectSolid(x, y, Z, w, h, c); }
void grad(float x, float y, float w, float h, u32 top, u32 bottom) { if (!gpuRoom()) return; C2D_DrawRectangle(x, y, Z, w, h, top, top, bottom, bottom); }
void circle(float cx, float cy, float r, u32 c) { if (r > 0 && ink(c)) { if (g_pix && r < 1.f) r = 1.f; C2D_DrawCircleSolid(cx, cy, Z, r, c); } }
void ellipse(float cx, float cy, float w, float h, u32 c) {
    if (w <= 0 || h <= 0 || !ink(c)) return;
    if (g_pix) { if (w < 2.f) w = 2.f; if (h < 2.f) h = 2.f; }
    C2D_DrawEllipseSolid(cx - w / 2, cy - h / 2, Z, w, h, c);
}
void tri(float x0, float y0, float x1, float y1, float x2, float y2, u32 c) {
    if (!ink(c)) return;
    C2D_DrawTriangle(x0, y0, c, x1, y1, c, x2, y2, c, Z);
}
void quad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, u32 c) {
    tri(x0, y0, x1, y1, x2, y2, c);
    tri(x0, y0, x2, y2, x3, y3, c);
}
void line(float x0, float y0, float x1, float y1, float th, u32 c) { if (ink(c)) C2D_DrawLine(x0, y0, c, x1, y1, c, (g_pix && th < 2.f) ? 2.f : th, Z); }
void rrect(float x, float y, float w, float h, float r, u32 c) {
    r = std::fmin(r, std::fmin(w, h) / 2);
    if (w <= 0 || h <= 0) return;
    if (r < 1.f) { rect(x, y, w, h, c); return; }
    if ((c >> 24) == 255) {   // opaque: overlapping pieces are fine (and cheaper)
        rect(x + r, y, w - 2 * r, h, c);
        rect(x, y + r, r, h - 2 * r, c);
        rect(x + w - r, y + r, r, h - 2 * r, c);
        circle(x + r, y + r, r, c);
        circle(x + w - r, y + r, r, c);
        circle(x + r, y + h - r, r, c);
        circle(x + w - r, y + h - r, r, c);
        return;
    }
    // translucent: a non-overlapping triangle fan so there are no dark blobs
    const int SEG = 4;
    float cx = x + w / 2, cy = y + h / 2;
    float px[4 * (SEG + 1)], py[4 * (SEG + 1)];
    const float ccx[4] = {x + w - r, x + r, x + r, x + w - r}, ccy[4] = {y + h - r, y + h - r, y + r, y + r};
    int n = 0;
    for (int k = 0; k < 4; k++)
        for (int i = 0; i <= SEG; i++) {
            float a = (k * 90.f + i * 90.f / SEG) * 3.14159265f / 180.f;
            px[n] = ccx[k] + std::cos(a) * r;
            py[n] = ccy[k] + std::sin(a) * r;
            n++;
        }
    for (int i = 0; i < n; i++) tri(cx, cy, px[i], py[i], px[(i + 1) % n], py[(i + 1) % n], c);
}
void outline(float x, float y, float w, float h, float th, u32 c) {
    rect(x, y, w, th, c); rect(x, y + h - th, w, th, c);
    rect(x, y, th, h, c); rect(x + w - th, y, th, h, c);
}

void sparkle(float x, float y, float s, u32 c) {
    float t = s * 0.22f;
    quad(x, y - s, x + t, y, x, y + s, x - t, y, c);
    quad(x - s, y, x, y - t, x + s, y, x, y + t, c);
    circle(x, y, t * 1.1f, c);
}

void swirl(float cx, float cy, float r, float turns, float th, u32 c, bool flip) {
    const int N = std::max(6, std::min((int)(18 * turns) + 4, (int)(r * turns * 2.5f)));
    float px = cx, py = cy;
    for (int i = 1; i <= N; i++) {
        float k = (float)i / N;
        float a = k * turns * 6.2831853f;
        float rr = r * k;
        float x = cx + (flip ? -1 : 1) * std::cos(a) * rr;
        float y = cy + std::sin(a) * rr;
        line(px, py, x, y, th * (0.4f + 0.6f * k), c);
        px = x; py = y;
    }
}

void heart(float cx, float cy, float s, u32 c) {
    if ((c >> 24) != 255) {   // translucent: non-overlapping fan from the classic heart curve
        const int N = 28;
        float k = s / 15.f, ox = cx, oy = cy + s * 0.15f;
        float px0 = 0, py0 = 0;
        for (int i = 0; i <= N; i++) {
            float t = i * 6.2831853f / N;
            float st = std::sin(t);
            float x = 16 * st * st * st, y = 13 * std::cos(t) - 5 * std::cos(2 * t) - 2 * std::cos(3 * t) - std::cos(4 * t);
            float px = ox + x * k, py = oy - y * k;
            if (i) tri(ox, oy, px0, py0, px, py, c);
            px0 = px; py0 = py;
        }
        return;
    }
    circle(cx - s * 0.5f, cy - s * 0.2f, s * 0.55f, c);
    circle(cx + s * 0.5f, cy - s * 0.2f, s * 0.55f, c);
    tri(cx - s * 1.03f, cy, cx + s * 1.03f, cy, cx, cy + s * 1.1f, c);
}

void zebraRect(float x, float y, float w, float h, u32 base, u32 stripe, int seed) {
    rect(x, y, w, h, base);
    float sx = x + 1 + (seed % 3);
    int k = seed;
    while (sx < x + w - 2) {
        float wt = 1.6f + (k * 7 % 5) * 0.5f;
        float sk = ((k % 2) ? 3.f : -2.f);
        float a = sx, b = sx + sk;
        if (a < x) a = x;
        if (b < x) { b = x; }
        if (a + wt > x + w) break;
        if (b + wt > x + w) b = x + w - wt;
        quad(a, y, a + wt, y, b + wt * 0.6f, y + h, b, y + h, stripe);
        sx += wt + 3.5f + (k * 3 % 4);
        k++;
    }
}

void leopardRect(float x, float y, float w, float h, int seed) {
    rect(x, y, w, h, C(214, 166, 98));
    u32 ring = C(70, 42, 28), core = C(176, 118, 60);
    unsigned st = 1234u + seed * 77u;
    int n = (int)(w * h / 60.f) + 1;
    for (int i = 0; i < n; i++) {
        st = st * 1103515245u + 12345u;
        float fx = (float)((st >> 8) % 1000) / 1000.f;
        st = st * 1103515245u + 12345u;
        float fy = (float)((st >> 8) % 1000) / 1000.f;
        float r = std::fmin(2.6f, std::fmin(w, h) * 0.18f);
        if (r < 0.8f) return;
        float px = x + r + fx * (w - 2 * r), py = y + r + fy * (h - 2 * r);
        circle(px, py, r, ring);
        circle(px + r * 0.15f, py - r * 0.1f, r * 0.55f, core);
    }
}

void Pen::F(float x, float y, float w, float h, int palIdx, u32 solid, int seed) const {
    float px = flip ? ox + (W - x - w) * s : ox + x * s;
    float py = Y(y);
    if (palIdx == 10) zebraRect(px, py, w * s, h * s, C(244, 240, 234), C(col::BLACK), seed);
    else if (palIdx == 11) leopardRect(px, py, w * s, h * s, seed);
    else rect(px, py, w * s, h * s, solid);
}

void text(const char* s, float x, float y, float sc, u32 c, int align) {
    if (!s || !*s || !gpuRoom()) return;
    if (pxText()) { pxDraw(s, x, y, sc, c, align); return; }
    C2D_Text t;
    C2D_TextParse(&t, g_tbuf, s);
    C2D_TextOptimize(&t);
    u32 flags = C2D_WithColor;
    if (align == 1) flags |= C2D_AlignCenter;
    else if (align == 2) flags |= C2D_AlignRight;
    C2D_DrawText(&t, flags, x, y, Z, sc, sc, c);
}
void textShadow(const char* s, float x, float y, float sc, u32 c, u32 shadow, int align) {
    text(s, x + 1, y + 1, sc, shadow, align);
    text(s, x, y, sc, c, align);
}
// chunky Flash-game lettering: the text is parsed once and stamped 8 times in the outline colour
void textOutline(const char* s, float x, float y, float sc, u32 c, u32 ol, int align, float th) {
    if (!s || !*s || !gpuRoom()) return;
    if (pxText()) {   // crisp 1-dot pixel outline
        const int d = pxDot(sc);
        (void)th;
        pxDraw(s, x - d, y, sc, ol, align); pxDraw(s, x + d, y, sc, ol, align); pxDraw(s, x, y - d, sc, ol, align); pxDraw(s, x, y + d, sc, ol, align);
        pxDraw(s, x, y, sc, c, align);
        return;
    }
    C2D_Text t;
    C2D_TextParse(&t, g_tbuf, s);
    C2D_TextOptimize(&t);
    u32 flags = C2D_WithColor;
    if (align == 1) flags |= C2D_AlignCenter;
    else if (align == 2) flags |= C2D_AlignRight;
    static const float O[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-0.7f, -0.7f}, {0.7f, -0.7f}, {-0.7f, 0.7f}, {0.7f, 0.7f}};
    for (const auto& o : O) C2D_DrawText(&t, flags, x + o[0] * th, y + o[1] * th + 0.6f, Z, sc, sc, ol);
    C2D_DrawText(&t, flags, x, y, Z, sc, sc, c);
}
float textW(const char* s, float sc) {
    if (!s || !*s) return 0;
    if (pxText()) return pxWidth(s, pxDot(sc));
    C2D_Text t;
    C2D_TextBufClear(g_mbuf);
    C2D_TextParse(&t, g_mbuf, s);
    float w = 0, h = 0;
    C2D_TextGetDimensions(&t, sc, sc, &w, &h);
    return w;
}

std::vector<std::string> wrapText(const std::string& s, float sc, float maxW) {
    std::vector<std::string> out;
    std::string lineS, word;
    auto flushWord = [&]() {
        if (word.empty()) return;
        std::string trial = lineS.empty() ? word : lineS + " " + word;
        if (!lineS.empty() && textW(trial.c_str(), sc) > maxW) { out.push_back(lineS); lineS = word; }
        else lineS = trial;
        word.clear();
    };
    for (char ch : s) {
        if (ch == '\n') { flushWord(); out.push_back(lineS); lineS.clear(); }
        else if (ch == ' ') flushWord();
        else word += ch;
    }
    flushWord();
    if (!lineS.empty()) out.push_back(lineS);
    return out;
}

// ---- Flash-era UI: hot pink & turquoise, chunky dark outlines, glossy tops
const RGB UI_INK{34, 22, 48};
static RGB styleCol(int style, bool on) {
    switch (style) {
    case 1: return on ? RGB{255, 66, 160} : RGB{40, 192, 210};    // turquoise
    case 2: return on ? RGB{255, 66, 160} : RGB{255, 168, 36};    // orange
    case 3: return on ? RGB{255, 66, 160} : RGB{150, 96, 230};    // purple
    default: return on ? RGB{255, 120, 196} : RGB{255, 46, 140};  // hot pink
    }
}
extern int g_artStyle;
static bool flipUI() { return false; }   // Flip Phone: glossy navy panels, white boxes, orange highlights
static const RGB NAVY = {28, 44, 112}, NAVY_D = {16, 24, 70}, NAVY_L = {70, 104, 196}, ORANGE = {250, 128, 30};
void flashPanel(float x, float y, float w, float h, RGB fill, float r, uint8_t a) {
    if (flipUI()) {
        x = std::floor(x); y = std::floor(y);
        const bool box = fill.r + fill.g + fill.b > 600;   // light fills become the white inner boxes
        rrect(x - 2, y - 2, w + 4, h + 4, 5, C(NAVY_D, a));
        if (box) { rrect(x, y, w, h, 4, C(255, 255, 255, a)); rect(x + 2, y + h - 2, w - 4, 2, C(196, 206, 230, a)); return; }
        rrect(x, y, w, h, 4, C(NAVY, a));
        rrect(x + 1, y + 1, w - 2, std::fmin(h * 0.45f, 9.f), 3, C(NAVY_L, a));   // glossy bevel
        rect(x + 3, y + 2, w - 6, 1, C(160, 190, 255, (uint8_t)(a * 0.7f)));
        return;
    }
    rrect(x - 1.5f, y - 1.5f, w + 3, h + 3, r + 1.5f, C(UI_INK, a));
    rrect(x, y, w, h, r, C(fill, a));
    rrect(x + 2, y + 1.5f, w - 4, std::fmin(h * 0.4f, 10.f), std::fmax(1.f, r - 2), C(255, 255, 255, (uint8_t)(a / 5)));
}
// fit a label inside a box: shrink it, then wrap to two lines (never spills into neighbours)
static void fitLabel(const char* label, float cx, float cy, float maxW, float maxH, float sc, u32 fill, u32 ink) {
    float s = sc, w = textW(label, s);
    if (w <= maxW) { textOutline(label, cx, cy - 15 * s - 0.5f, s, fill, ink, 1, 1.0f); return; }
    s = std::max(0.3f, sc * maxW / w);
    if ((textW(label, s) <= maxW && s >= 0.34f) || maxH < 20) { s = std::max(0.26f, sc * maxW / w); textOutline(label, cx, cy - 15 * s - 0.5f, s, fill, ink, 1, 1.0f); return; }
    std::string L(label); size_t best = std::string::npos; const size_t mid = L.size() / 2;
    for (size_t i = 0; i < L.size(); i++) if (L[i] == ' ' && (best == std::string::npos || (i > mid ? i - mid : mid - i) < (best > mid ? best - mid : mid - best))) best = i;
    if (best == std::string::npos) { s = std::max(0.24f, sc * maxW / w); textOutline(label, cx, cy - 15 * s - 0.5f, s, fill, ink, 1, 1.0f); return; }
    const std::string a = L.substr(0, best), b2 = L.substr(best + 1);
    s = std::min(sc, std::min(maxW / std::max(1.f, textW(a.c_str(), 1)), maxW / std::max(1.f, textW(b2.c_str(), 1))));
    s = std::min(s, maxH / 2 / 26.f); s = std::max(0.24f, s);
    const float lh = 26 * s;
    textOutline(a.c_str(), cx, cy - lh - 3 * s, s, fill, ink, 1, 1.0f);
    textOutline(b2.c_str(), cx, cy - 3 * s, s, fill, ink, 1, 1.0f);
}

void button(const Btn& b, const char* label, bool on, int style, float sc) {
    if (flipUI()) {
        const float x = std::floor(b.x), y = std::floor(b.y);
        RGB base = on ? ORANGE : NAVY, hi = on ? RGB{255, 190, 90} : NAVY_L;
        rrect(x - 2, y - 2, b.w + 4, b.h + 4, 5, C(on ? RGB{140, 60, 10} : NAVY_D));
        rrect(x, y, b.w, b.h, 4, C(base));
        rrect(x + 1, y + 1, b.w - 2, b.h * 0.48f, 3, C(hi));        // bevel highlight
        rect(x + 3, y + 2, b.w - 6, 1, C(255, 255, 255, 140));
        rect(x + 2, y + b.h - 3, b.w - 4, 2, C(shade(base, 0.7f)));
        (void)style;
        fitLabel(label, x + b.w / 2, y + b.h / 2, b.w - 8, b.h - 2, sc, C(255, 255, 255), C(on ? RGB{120, 50, 0} : NAVY_D));
        return;
    }
    RGB base = styleCol(style, on);
    rrect(b.x, b.y + 2.5f, b.w, b.h, 7, C(UI_INK, 120));                   // drop shadow
    rrect(b.x - 1.5f, b.y - 1.5f, b.w + 3, b.h + 3, 8.5f, C(UI_INK));        // the chunky outline
    rrect(b.x, b.y, b.w, b.h, 7, C(shade(base, 0.82f)));
    rrect(b.x, b.y, b.w, b.h - 2.5f, 7, C(base));
    rrect(b.x + 4, b.y + 2, b.w - 8, b.h * 0.34f, 4, C(255, 255, 255, 80)); // gloss
    fitLabel(label, b.x + b.w / 2, b.y + b.h / 2, b.w - 10, b.h - 2, sc, C(255, 255, 255), C(UI_INK));
    if (on) sparkle(b.x + b.w - 7, b.y + 6, 4, C(255, 255, 255, 230));
}

void panelBg(float x, float y, float w, float h) {
    if (flipUI()) { flashPanel(x, y, w, h, NAVY, 6); return; }
    rrect(x - 1.5f, y - 1.5f, w + 3, h + 3, 11, C(UI_INK));
    rrect(x, y, w, h, 10, C(36, 176, 196));
    rrect(x + 3, y + 3, w - 6, h - 6, 8, C(28, 140, 164));
}

void bottomBackdrop(float t) {
    if (flipUI()) {   // flat navy with a crisp 2px checker texture (no gradients)
        rect(0, 0, BOT_W, BOT_H, C(20, 30, 80));
        for (int y = 0; y < BOT_H; y += 8) for (int x = (y / 8 % 2) * 8; x < BOT_W; x += 16) rect(x, y, 8, 8, C(26, 38, 96));
        (void)t; return;
    }
    // Flash-game turquoise: bright gradient, halftone dots and drifting stars
    grad(0, 0, BOT_W, BOT_H, C(60, 206, 220), C(24, 128, 158));
    for (int r = 0; r < 12; r++) for (int k = 0; k < 16; k++) {
        float x = k * 20.f + (r % 2) * 10.f + 6, y = r * 20.f + 8;
        circle(x, y, 1.2f + r * 0.08f, C(255, 255, 255, 22));
    }
    for (int i = 0; i < 4; i++) {
        float a = std::sin(t * 2.f + i * 1.7f) * 0.5f + 0.5f;
        sparkle(30 + i * 85, 16 + (i * 53 % 190), 2.5f + a * 2.f, C(255, 255, 255, (uint8_t)(60 + a * 100)));
    }
}

// ---- Flash-game scenery helpers (title, cutscenes, beach)
void flashSun(float x, float y, float r, float t) {
    for (int i = 0; i < 12; i++) {
        float a = i * 0.5236f + t * 0.2f, a1 = a + 0.13f, a2 = a - 0.13f;
        tri(x + std::cos(a1) * r * 1.05f, y + std::sin(a1) * r * 1.05f, x + std::cos(a2) * r * 1.05f, y + std::sin(a2) * r * 1.05f, x + std::cos(a) * r * 1.55f, y + std::sin(a) * r * 1.55f, C(UI_INK));
        tri(x + std::cos(a1) * r * 1.0f, y + std::sin(a1) * r * 1.0f, x + std::cos(a2) * r * 1.0f, y + std::sin(a2) * r * 1.0f, x + std::cos(a) * r * 1.45f, y + std::sin(a) * r * 1.45f, C(255, 214, 40));
    }
    circle(x, y, r + 1.5f, C(UI_INK));
    circle(x, y, r, C(255, 214, 40));
    circle(x - r * 0.25f, y - r * 0.25f, r * 0.55f, C(255, 240, 140));
}
void flashCloud(float x, float y, float s) {
    const float P[5][3] = {{-1.1f, 0.15f, 0.55f}, {-0.45f, -0.2f, 0.75f}, {0.35f, -0.3f, 0.85f}, {1.05f, 0.1f, 0.6f}, {0, 0.25f, 0.7f}};
    for (const auto& p : P) circle(x + p[0] * 14 * s, y + p[1] * 14 * s, p[2] * 14 * s + 1.5f, C(UI_INK));
    for (const auto& p : P) circle(x + p[0] * 14 * s, y + p[1] * 14 * s, p[2] * 14 * s, C(255, 255, 255));
    ellipse(x - 2 * s, y - 6 * s, 16 * s, 5 * s, C(255, 255, 255));
}
void teenBadge(float x, float y, float s) {
    // an original "Teen 13+" age sticker
    circle(x, y, 17 * s + 2, C(UI_INK));
    circle(x, y, 17 * s, C(255, 46, 140));
    circle(x, y, 13.5f * s, C(255, 255, 255));
    textOutline("13+", x, y - 9 * s, 0.6f * s, C(255, 46, 140), C(UI_INK), 1, 0.9f);
    textOutline("TEEN", x, y + 2.5f * s, 0.32f * s, C(40, 192, 210), C(UI_INK), 1, 0.7f);
}
