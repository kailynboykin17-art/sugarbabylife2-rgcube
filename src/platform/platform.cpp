// Sugarbaby Life 2 - RG Cube / Android port: the 3DS system functions the game uses, rebuilt on SDL2.
// The game still "thinks" in 3DS screens (400x240 top, 320x240 bottom); everything is drawn
// at the real screen resolution (plus 2x supersampling) so shapes and text come out smooth.
#include <SDL.h>
#include "citro2d.h"
#include "fontw.h"
#include "xbr.h"
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

int sbl2_main();
extern "C" int SDL_main(int argc, char* argv[]);

static SDL_Window* g_win = nullptr;
static SDL_Renderer* g_ren = nullptr;
static bool g_paused = false, g_quit = false, g_saveTurn = false;
static int g_winW = 720, g_winH = 720;

// ============================================================ textures
struct TexImpl {
    int w = 0, h = 0;          // size the game sees
    float k = 1.f;             // real pixels per game pixel (render targets)
    bool target = false, dirty = false, nearest = false;
    SDL_Texture* st = nullptr;
    void* data = nullptr; u32 size = 0;
};
static std::vector<TexImpl*> g_texs;
u32* gpuCmdBuf = nullptr; u32 gpuCmdBufOffset = 0, gpuCmdBufSize = 1u << 30;

bool g_keepPixels = false;   // true = the chunky 3DS pixel look
// (older, simpler smoother - kept for reference; xbr.h is used)
static std::vector<u32> epx(const std::vector<u32>& a, int w, int h) {
    std::vector<u32> o((size_t)w * h * 4);
    auto P = [&](int x, int y) { x = std::clamp(x, 0, w - 1); y = std::clamp(y, 0, h - 1); return a[(size_t)y * w + x]; };
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        const u32 p = P(x, y), A = P(x, y - 1), B = P(x + 1, y), C = P(x - 1, y), D = P(x, y + 1);
        u32 e1 = p, e2 = p, e3 = p, e4 = p;
        if (C == A && C != D && A != B) e1 = A;
        if (A == B && A != C && B != D) e2 = B;
        if (D == C && D != B && C != A) e3 = C;
        if (B == D && B != A && D != C) e4 = D;
        u32* r0 = &o[(size_t)(y * 2) * w * 2 + x * 2]; u32* r1 = r0 + w * 2;
        r0[0] = e1; r0[1] = e2; r1[0] = e3; r1[1] = e4;
    }
    return o;
}
static void uploadTex(TexImpl* t) {
    if (!t->data || g_paused) return;
    const bool smooth = t->nearest && !g_keepPixels && t->w * t->h <= 512 * 512;
    const int up = smooth ? 4 : 1;
    if (!t->st) {
        t->st = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, t->w * up, t->h * up);
        if (!t->st) return;
        SDL_SetTextureBlendMode(t->st, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(t->st, (t->nearest && !smooth) ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
    }
    // 3DS textures are stored in 8x8 Morton-order tiles as 0xRRGGBBAA words
    std::vector<uint8_t> px((size_t)t->w * t->h * 4);
    const u32* src = (const u32*)t->data;
    for (int y = 0; y < t->h; y++) for (int x = 0; x < t->w; x++) {
        const u32 off = ((y >> 3) * (t->w >> 3) + (x >> 3)) * 64;
        const u32 m = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
        const u32 v = src[off + m];
        uint8_t* d = &px[((size_t)y * t->w + x) * 4];
        d[0] = v >> 24; d[1] = v >> 16; d[2] = v >> 8; d[3] = v;
    }
#ifndef __ANDROID__
    if (t->nearest && getenv("SBL2_DUMPTEX")) { static int n = 0; char fn[64]; snprintf(fn, sizeof fn, "tex_%03d_%dx%d.rgba", n++, t->w, t->h); if (FILE* f = fopen(fn, "wb")) { fwrite(px.data(), 1, px.size(), f); fclose(f); } }
#endif
    if (smooth) {
        std::vector<u32> a((size_t)t->w * t->h); memcpy(a.data(), px.data(), a.size() * 4);
        std::vector<u32> c = xbr::scale4(a, t->w, t->h);
        SDL_UpdateTexture(t->st, nullptr, c.data(), t->w * 4 * 4);
    } else SDL_UpdateTexture(t->st, nullptr, px.data(), t->w * 4);
    t->dirty = false;
}
static float g_targetK = 2.f;   // resolution boost for smooth (linear) offscreen layers
static void makeTarget(TexImpl* t) {
    if (t->st || g_paused) return;
    // RG Cube: even the "pixel" layer is drawn at full screen resolution with smoothing,
    // so dolls come out clean instead of blocky (set g_keepPixels to get the chunky 3DS look)
    const float k = (t->nearest && g_keepPixels) ? 1.f : t->nearest ? g_targetK * 4.f : g_targetK;
    t->k = k;
    t->st = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, (int)std::ceil(t->w * k), (int)std::ceil(t->h * k));
    if (t->st) { SDL_SetTextureBlendMode(t->st, SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(t->st, (t->nearest && g_keepPixels) ? SDL_ScaleModeNearest : SDL_ScaleModeLinear); }
}
static SDL_Texture* sdlTex(TexImpl* t) {
    if (!t) return nullptr;
    if (t->target) makeTarget(t);
    else if (!t->st || t->dirty) uploadTex(t);
    return t->st;
}
bool C3D_TexInit(C3D_Tex* tex, int w, int h, int) {
    TexImpl* t = new TexImpl; t->w = w; t->h = h; t->size = (u32)w * h * 4; t->data = calloc(1, t->size); t->dirty = true;
    g_texs.push_back(t);
    tex->w = w; tex->h = h; tex->data = t->data; tex->size = t->size; tex->impl = t;
    return t->data != nullptr;
}
bool C3D_TexInitVRAM(C3D_Tex* tex, int w, int h, int) {
    TexImpl* t = new TexImpl; t->w = w; t->h = h; t->target = true;
    g_texs.push_back(t);
    tex->w = w; tex->h = h; tex->data = nullptr; tex->size = (u32)w * h * 4; tex->impl = t;
    return true;
}
void C3D_TexSetFilter(C3D_Tex* tex, int mag, int) {
    if (!tex->impl) return;
    tex->impl->nearest = mag == GPU_NEAREST;
    if (tex->impl->st && !tex->impl->target) { SDL_DestroyTexture(tex->impl->st); tex->impl->st = nullptr; tex->impl->dirty = true; }
}
static void flushBatch();
void C3D_TexDelete(C3D_Tex* tex) {
    TexImpl* t = tex->impl; if (!t) return;
    flushBatch();
    g_texs.erase(std::remove(g_texs.begin(), g_texs.end(), t), g_texs.end());
    if (t->st) SDL_DestroyTexture(t->st);
    free(t->data); delete t;
    tex->impl = nullptr; tex->data = nullptr;
}
Result GSPGPU_FlushDataCache(const void* p, u32) {
    for (TexImpl* t : g_texs) if (t->data == p) { flushBatch(); t->dirty = true; }
    return 0;
}

// ============================================================ screens & layout
struct Screen { TexImpl impl; C3D_RenderTarget rt; int gw, gh; float q; SDL_Rect dst; };
static Screen g_scr[2];
static const int SS = 2;   // supersampling: draw at 2x, shrink with smoothing
static void layout() {
    SDL_GetRendererOutputSize(g_ren, &g_winW, &g_winH);
    const float W = (float)g_winW, H = (float)g_winH;
    float st, sb; int tx, ty, bx, by;
    if (W / H > 1.45f) {   // wide screen (phones sideways): side by side
        st = std::min(W * 0.55f / 400.f, H / 240.f); sb = std::min((W - 400 * st) / 320.f, H / 240.f);
        tx = 0; ty = (int)((H - 240 * st) / 2); bx = (int)(400 * st); by = (int)((H - 240 * sb) / 2);
        const int spare = (int)(W - 400 * st - 320 * sb); tx += spare / 2; bx += spare / 2;
    } else {               // square (RG Cube) or tall: stacked like a DS
        st = std::min(W / 400.f, H * 0.6f / 240.f); sb = std::min(W / 320.f, (H - 240 * st) / 240.f);
        const int total = (int)(240 * st + 240 * sb);
        tx = (int)((W - 400 * st) / 2); ty = (int)((H - total) / 2); bx = (int)((W - 320 * sb) / 2); by = ty + (int)(240 * st);
    }
    g_scr[0].dst = {tx, ty, (int)(400 * st), (int)(240 * st)}; g_scr[0].q = st * SS;
    g_scr[1].dst = {bx, by, (int)(320 * sb), (int)(240 * sb)}; g_scr[1].q = sb * SS;
    g_targetK = std::max(1.f, g_scr[0].q / 2.f);
    for (int i = 0; i < 2; i++) {
        Screen& s = g_scr[i];
        if (s.impl.st) SDL_DestroyTexture(s.impl.st);
        s.impl.st = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, (int)std::ceil(s.gw * s.q), (int)std::ceil(s.gh * s.q));
        SDL_SetTextureScaleMode(s.impl.st, SDL_ScaleModeLinear);
        s.impl.k = s.q;
    }
    for (TexImpl* t : g_texs) if (t->target && t->st) { SDL_DestroyTexture(t->st); t->st = nullptr; }   // rebuilt at the new boost
}
C3D_RenderTarget* C2D_CreateScreenTarget(int s, int) { return &g_scr[s == GFX_TOP ? 0 : 1].rt; }
C3D_RenderTarget* C3D_RenderTargetCreateFromTex(C3D_Tex* t, int, int, GPU_DEPTHBUF) { auto* r = new C3D_RenderTarget{t->impl, -1}; return r; }

// ============================================================ drawing
static float M[6] = {1, 0, 0, 1, 0, 0};
static TexImpl* g_cur = nullptr;
static SDL_BlendMode g_blend = SDL_BLENDMODE_BLEND;
static std::vector<SDL_Vertex> g_v;
static SDL_Texture* g_bTex = nullptr;
static SDL_BlendMode g_bBlend = SDL_BLENDMODE_BLEND;
static bool g_inFrame = false;
static void flushBatch() {
    if (g_v.empty()) return;
    if (!g_paused && g_cur) {
        SDL_SetRenderDrawBlendMode(g_ren, g_bBlend);
        if (g_bTex) SDL_SetTextureBlendMode(g_bTex, g_bBlend);
        SDL_RenderGeometry(g_ren, g_bTex, g_v.data(), (int)g_v.size(), nullptr, 0);
    }
    g_v.clear();
}
static void useTarget(TexImpl* t) {
    flushBatch(); g_cur = t;
    if (g_paused || !t) return;
    if (t->target) makeTarget(t);
    SDL_SetRenderTarget(g_ren, t->st);
}
static inline SDL_Color col(u32 c) { return SDL_Color{(Uint8)(c & 255), (Uint8)(c >> 8 & 255), (Uint8)(c >> 16 & 255), (Uint8)(c >> 24)}; }
static inline void xf(float x, float y, float& ox, float& oy) {
    const float k = g_cur ? g_cur->k : 1.f;
    ox = (M[0] * x + M[2] * y + M[4]) * k; oy = (M[1] * x + M[3] * y + M[5]) * k;
}
static void vtx(float x, float y, u32 c, float u = 0, float v = 0) { SDL_Vertex a; xf(x, y, a.position.x, a.position.y); a.color = col(c); a.tex_coord = {u, v}; g_v.push_back(a); }
static void begin(SDL_Texture* t) {
    if (t != g_bTex || g_blend != g_bBlend || g_v.size() > 60000) { flushBatch(); g_bTex = t; g_bBlend = g_blend; }
}
void C3D_AlphaBlend(int, int, int srcC, int dstC, int, int) {
    g_blend = (srcC == GPU_ONE && dstC == GPU_ZERO) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND;
}
void C2D_Flush() { flushBatch(); }
void C2D_TargetClear(C3D_RenderTarget* rt, u32 c) {
    TexImpl* keep = g_cur; useTarget(rt->impl);
    if (!g_paused && g_cur) { const SDL_Color k = col(c); SDL_SetRenderDrawColor(g_ren, k.r, k.g, k.b, k.a); SDL_RenderClear(g_ren); }
    useTarget(keep);
}
void C2D_SceneBegin(C3D_RenderTarget* rt) { useTarget(rt->impl); }
void C2D_ViewReset() { const float I[6] = {1, 0, 0, 1, 0, 0}; memcpy(M, I, sizeof M); }
void C2D_ViewSave(C3D_Mtx* m) { memcpy(m->m, M, sizeof M); }
void C2D_ViewRestore(const C3D_Mtx* m) { memcpy(M, m->m, sizeof M); }
void C2D_ViewTranslate(float x, float y) { M[4] += M[0] * x + M[2] * y; M[5] += M[1] * x + M[3] * y; }
void C2D_ViewScale(float x, float y) { M[0] *= x; M[1] *= x; M[2] *= y; M[3] *= y; }
void C2D_ViewRotate(float r) { const float c = std::cos(r), s = std::sin(r); const float a = M[0], b = M[1], cc = M[2], d = M[3];
    M[0] = a * c + cc * s; M[1] = b * c + d * s; M[2] = -a * s + cc * c; M[3] = -b * s + d * c; }
static void quad(float x0, float y0, float x1, float y1, u32 c0, u32 c1, u32 c2, u32 c3) {   // tl tr bl br
    begin(nullptr);
    vtx(x0, y0, c0); vtx(x1, y0, c1); vtx(x0, y1, c2);
    vtx(x1, y0, c1); vtx(x1, y1, c3); vtx(x0, y1, c2);
}
bool C2D_DrawRectSolid(float x, float y, float, float w, float h, u32 c) { quad(x, y, x + w, y + h, c, c, c, c); return true; }
bool C2D_DrawRectangle(float x, float y, float, float w, float h, u32 c0, u32 c1, u32 c2, u32 c3) { quad(x, y, x + w, y + h, c0, c1, c2, c3); return true; }
bool C2D_DrawTriangle(float x0, float y0, u32 c0, float x1, float y1, u32 c1, float x2, float y2, u32 c2, float) {
    begin(nullptr); vtx(x0, y0, c0); vtx(x1, y1, c1); vtx(x2, y2, c2); return true; }
bool C2D_DrawLine(float x0, float y0, u32 c0, float x1, float y1, u32 c1, float th, float) {
    const float dx = x1 - x0, dy = y1 - y0, l = std::sqrt(dx * dx + dy * dy); if (l < 1e-4f) return true;
    const float nx = -dy / l * th / 2, ny = dx / l * th / 2;
    begin(nullptr);
    vtx(x0 + nx, y0 + ny, c0); vtx(x1 + nx, y1 + ny, c1); vtx(x1 - nx, y1 - ny, c1);
    vtx(x0 + nx, y0 + ny, c0); vtx(x1 - nx, y1 - ny, c1); vtx(x0 - nx, y0 - ny, c0);
    return true;
}
static void ell(float cx, float cy, float rx, float ry, u32 c) {
    if (rx <= 0 || ry <= 0) return;
    const float k = (g_cur ? g_cur->k : 1.f) * std::sqrt(std::fabs(M[0] * M[3] - M[1] * M[2]));
    const int n = std::clamp((int)(std::max(rx, ry) * k * 0.9f), 12, 96);
    begin(nullptr);
    float px = cx + rx, py = cy;
    for (int i = 1; i <= n; i++) {
        const float a = 6.2831853f * i / n, qx = cx + std::cos(a) * rx, qy = cy + std::sin(a) * ry;
        vtx(cx, cy, c); vtx(px, py, c); vtx(qx, qy, c); px = qx; py = qy;
    }
}
bool C2D_DrawCircleSolid(float x, float y, float, float r, u32 c) { ell(x, y, r, r, c); return true; }
bool C2D_DrawEllipseSolid(float x, float y, float, float w, float h, u32 c) { ell(x + w / 2, y + h / 2, w / 2, h / 2, c); return true; }
bool C2D_DrawImageAt(C2D_Image img, float x, float y, float, const C2D_ImageTint* tint, float sx, float sy) {
    if (!img.tex || !img.tex->impl || !img.subtex) return false;
    TexImpl* t = img.tex->impl;
    if (t == g_cur) return false;   // can't sample the texture being drawn into
    flushBatch();                   // the source may have just been drawn into: keep order
    SDL_Texture* st = sdlTex(t);
    if (!st) return false;
    const Tex3DS_SubTexture* s = img.subtex;
    const float w = s->width * sx, h = s->height * sy;
    const float u0 = s->left, u1 = s->right, v0 = 1.f - s->top, v1 = 1.f - s->bottom;
    const u32 c = tint ? tint->c : 0xFFFFFFFFu;
    begin(st);
    vtx(x, y, c, u0, v0); vtx(x + w, y, c, u1, v0); vtx(x, y + h, c, u0, v1);
    vtx(x + w, y, c, u1, v0); vtx(x + w, y + h, c, u1, v1); vtx(x, y + h, c, u0, v1);
    return true;
}

// ============================================================ text (Poppins, metrics matched to the 3DS font so layouts don't move)
static stbtt_packedchar g_pc[95];
static SDL_Texture* g_fontTex = nullptr;
static std::vector<uint8_t> g_fontBits;
static const float FONT_PX = 64.f;
static float g_ascent = 0;
static const int FA_W = 1024, FA_H = 1024;
static bool loadFont() {
    SDL_RWops* rw = SDL_RWFromFile("font.ttf", "rb");
    if (!rw) rw = SDL_RWFromFile("assets/font.ttf", "rb");
    if (!rw) return false;
    const Sint64 n = SDL_RWsize(rw); std::vector<uint8_t> ttf((size_t)n); SDL_RWread(rw, ttf.data(), 1, (size_t)n); SDL_RWclose(rw);
    std::vector<uint8_t> a(FA_W * FA_H);
    stbtt_pack_context pc; stbtt_PackBegin(&pc, a.data(), FA_W, FA_H, 0, 2, nullptr);
    stbtt_PackSetOversampling(&pc, 2, 2);
    stbtt_PackFontRange(&pc, ttf.data(), 0, FONT_PX, 32, 95, g_pc);
    stbtt_PackEnd(&pc);
    stbtt_fontinfo fi; stbtt_InitFont(&fi, ttf.data(), 0);
    int asc, desc, gap; stbtt_GetFontVMetrics(&fi, &asc, &desc, &gap);
    g_ascent = asc * stbtt_ScaleForPixelHeight(&fi, FONT_PX);
    g_fontBits.resize((size_t)FA_W * FA_H * 4);
    for (int i = 0; i < FA_W * FA_H; i++) { uint8_t* d = &g_fontBits[i * 4]; d[0] = d[1] = d[2] = 255; d[3] = a[i]; }
    return true;
}
static SDL_Texture* fontTex() {
    if (!g_fontTex && !g_fontBits.empty() && !g_paused) {
        g_fontTex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, FA_W, FA_H);
        SDL_SetTextureBlendMode(g_fontTex, SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(g_fontTex, SDL_ScaleModeLinear);
        SDL_UpdateTexture(g_fontTex, nullptr, g_fontBits.data(), FA_W * 4);
    }
    return g_fontTex;
}
static inline float adv3ds(unsigned char ch) { return ((ch >= 32 && ch < 127) ? FONTW[ch - 32] : 12.f) * 30.f / 24.f * 0.82f; }
static float textWidth(const std::string& s) { float w = 0; for (unsigned char ch : s) w += adv3ds(ch); return w; }
void C2D_TextGetDimensions(const C2D_Text* t, float sx, float sy, float* w, float* h) { if (w) *w = textWidth(t->s) * sx; if (h) *h = 30 * sy; }
// draws a string; size = the 3DS scale (1.0 = 30px line); x,y = top-left in game coordinates
static void drawString(const std::string& s, float x, float y, float sx, float sy, u32 c) {
    SDL_Texture* ft = fontTex(); if (!ft) return;
    const float k = 24.f / FONT_PX;   // glyph pixels -> game pixels at scale 1 (3DS cap height ~ 24px line)
    const float base = y + 23.f * sy;
    float pen = x;
    begin(ft);
    for (unsigned char ch : s) {
        const float a = adv3ds(ch) * sx;
        if (ch > 32 && ch < 127) {
            const stbtt_packedchar& g = g_pc[ch - 32];
            const float gw = g.xadvance > 0 ? g.xadvance : 1;
            const float fx = a / (gw * k * sx) ;   // squeeze/stretch so each letter takes the 3DS width
            const float x0 = pen + g.xoff * k * sx * fx, x1 = pen + g.xoff2 * k * sx * fx;
            const float y0 = base + g.yoff * k * sy, y1 = base + g.yoff2 * k * sy;
            const float u0 = g.x0 / (float)FA_W, u1 = g.x1 / (float)FA_W, v0 = g.y0 / (float)FA_H, v1 = g.y1 / (float)FA_H;
            vtx(x0, y0, c, u0, v0); vtx(x1, y0, c, u1, v0); vtx(x0, y1, c, u0, v1);
            vtx(x1, y0, c, u1, v0); vtx(x1, y1, c, u1, v1); vtx(x0, y1, c, u0, v1);
        }
        pen += a;
    }
}
void C2D_DrawText(const C2D_Text* t, u32 flags, float x, float y, float, float sx, float sy, ...) {
    u32 c = 0xff000000;
    if (flags & C2D_WithColor) { va_list ap; va_start(ap, sy); c = va_arg(ap, u32); va_end(ap); }
    const float w = textWidth(t->s) * sx;
    if (flags & C2D_AlignCenter) x -= w / 2; else if (flags & C2D_AlignRight) x -= w;
    if (flags & C2D_AtBaseline) y -= 23.f * sy;
    drawString(t->s, x, y, sx, sy, c);
}

// ============================================================ frames
static u32 g_held = 0, g_prev = 0, g_down = 0, g_padKeys = 0, g_keyKeys = 0;
static touchPosition g_tp{0, 0};
static bool g_touching = false;

void C3D_FrameBegin(int) { g_inFrame = true; }
static void present() {
    if (g_paused) return;
    SDL_SetRenderTarget(g_ren, nullptr);
    SDL_SetRenderDrawColor(g_ren, 255, 214, 230, 255);   // soft pink around the screens
    SDL_RenderClear(g_ren);
    for (int i = 0; i < 2; i++) if (g_scr[i].impl.st) { SDL_SetTextureBlendMode(g_scr[i].impl.st, SDL_BLENDMODE_NONE); SDL_RenderCopy(g_ren, g_scr[i].impl.st, nullptr, &g_scr[i].dst); }
}
#ifndef __ANDROID__
// desktop testing only: SBL2_SHOTS="120,300" saves those frames as BMPs; SBL2_SCRIPT="100:tap:160,120;140:key:A"
static int g_frameNo = 0;
static void testHooks() {
    static std::vector<int> shots; static bool init = false;
    if (!init) { init = true; if (const char* e = getenv("SBL2_SHOTS")) { for (const char* p = e; *p; ) { shots.push_back(atoi(p)); while (*p && *p != ',') p++; if (*p) p++; } } }
    for (int f : shots) if (f == g_frameNo) {
        SDL_Surface* sf = SDL_CreateRGBSurfaceWithFormat(0, g_winW, g_winH, 32, SDL_PIXELFORMAT_ARGB8888);
        SDL_RenderReadPixels(g_ren, nullptr, SDL_PIXELFORMAT_ARGB8888, sf->pixels, sf->pitch);
        char fn[64]; snprintf(fn, sizeof fn, "shot_%05d.bmp", f); SDL_SaveBMP(sf, fn); SDL_FreeSurface(sf);
    }
    if (getenv("SBL2_QUIT") && g_frameNo >= atoi(getenv("SBL2_QUIT"))) g_quit = true;
}
static void scriptInput() {
    static std::vector<std::string> ev; static bool init = false;
    if (!init) { init = true; if (const char* e = getenv("SBL2_SCRIPT")) { std::string S = e; size_t p = 0; while (p < S.size()) { size_t q = S.find(';', p); if (q == std::string::npos) q = S.size(); ev.push_back(S.substr(p, q - p)); p = q + 1; } } }
    for (auto& x : ev) { int f = 0; char kind[8] = {0}, arg[32] = {0}; if (sscanf(x.c_str(), "%d:%7[^:]:%31s", &f, kind, arg) < 2) continue;
        if (!strcmp(kind, "tap") && g_frameNo >= f && g_frameNo < f + 3) { int a, b; sscanf(arg, "%d,%d", &a, &b); g_tp = {(u16)a, (u16)b}; g_touching = true; }
        if (!strcmp(kind, "tap") && g_frameNo == f + 3) g_touching = false;
        if (!strcmp(kind, "key")) { u32 k = 0; switch (arg[0]) { case 'A': k = KEY_A; break; case 'B': k = KEY_B; break; case 'X': k = KEY_X; break; case 'Y': k = KEY_Y; break; case 'S': k = KEY_START; break; case 'L': k = KEY_L; break; case 'R': k = KEY_R; break; case 'D': k = KEY_DDOWN; break; case 'U': k = KEY_DUP; break; }
            if (g_frameNo >= f && g_frameNo < f + 3) g_keyKeys |= k; else if (g_frameNo == f + 3) g_keyKeys &= ~k; } }
}
#endif
void C3D_FrameEnd(int) {
    flushBatch(); g_inFrame = false;
    present();
#ifndef __ANDROID__
    testHooks(); g_frameNo++; scriptInput();
#endif
    if (!g_paused) SDL_RenderPresent(g_ren);
    g_cur = nullptr;
}

// ============================================================ input
static circlePosition g_circle{0, 0};
static float g_lx = 0, g_ly = 0, g_rx = 0, g_ry = 0;
static SDL_GameController* g_pad = nullptr;
static u32 padMap(int b) {
    switch (b) {   // Nintendo layout: the right face button is A, the bottom one is B
    case SDL_CONTROLLER_BUTTON_A: return KEY_B;  case SDL_CONTROLLER_BUTTON_B: return KEY_A;
    case SDL_CONTROLLER_BUTTON_X: return KEY_Y;  case SDL_CONTROLLER_BUTTON_Y: return KEY_X;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return KEY_L; case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return KEY_R;
    case SDL_CONTROLLER_BUTTON_START: return KEY_START; case SDL_CONTROLLER_BUTTON_BACK: return KEY_SELECT;
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return KEY_DUP; case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return KEY_DDOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return KEY_DLEFT; case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return KEY_DRIGHT;
    default: return 0; }
}
static u32 keyMap(SDL_Keycode k) {
    switch (k) {
    case SDLK_x: case SDLK_RETURN: return KEY_A; case SDLK_z: case SDLK_BACKSPACE: case SDLK_AC_BACK: return KEY_B;
    case SDLK_s: return KEY_X; case SDLK_a: return KEY_Y; case SDLK_q: return KEY_L; case SDLK_w: return KEY_R;
    case SDLK_SPACE: case SDLK_MENU: return KEY_START; case SDLK_TAB: return KEY_SELECT;
    case SDLK_UP: return KEY_DUP; case SDLK_DOWN: return KEY_DDOWN; case SDLK_LEFT: return KEY_DLEFT; case SDLK_RIGHT: return KEY_DRIGHT;
    case SDLK_ESCAPE: return KEY_B;
    default: return 0; }
}
static bool toBottom(float wx, float wy, u16& px, u16& py) {   // window pixels -> 320x240 bottom-screen coordinates
    const SDL_Rect& d = g_scr[1].dst;
    const float x = (wx - d.x) * 320.f / d.w, y = (wy - d.y) * 240.f / d.h;
    if (x < -8 || y < -8 || x > 328 || y > 248) return false;
    px = (u16)std::clamp((int)x, 0, 319); py = (u16)std::clamp((int)y, 0, 239); return true;
}
static void handle(const SDL_Event& e) {
    switch (e.type) {
    case SDL_QUIT: g_quit = true; break;
    case SDL_APP_WILLENTERBACKGROUND: case SDL_APP_DIDENTERBACKGROUND: if (!g_paused) { g_paused = true; g_saveTurn = true; } break;
    case SDL_APP_DIDENTERFOREGROUND: g_paused = false; break;
    case SDL_RENDER_DEVICE_RESET:
        for (TexImpl* t : g_texs) { t->st = nullptr; t->dirty = true; }
        g_fontTex = nullptr; for (auto& s : g_scr) s.impl.st = nullptr; layout(); break;
    case SDL_RENDER_TARGETS_RESET: break;
    case SDL_WINDOWEVENT: if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) layout(); break;
    case SDL_CONTROLLERDEVICEADDED: if (!g_pad) g_pad = SDL_GameControllerOpen(e.cdevice.which); break;
    case SDL_CONTROLLERBUTTONDOWN: g_padKeys |= padMap(e.cbutton.button); break;
    case SDL_CONTROLLERBUTTONUP: g_padKeys &= ~padMap(e.cbutton.button); break;
    case SDL_CONTROLLERAXISMOTION: {
        const float v = e.caxis.value / 32767.f;
        switch (e.caxis.axis) {
        case SDL_CONTROLLER_AXIS_LEFTX: g_lx = v; break; case SDL_CONTROLLER_AXIS_LEFTY: g_ly = v; break;
        case SDL_CONTROLLER_AXIS_RIGHTX: g_rx = v; break; case SDL_CONTROLLER_AXIS_RIGHTY: g_ry = v; break;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT: if (v > 0.5f) g_padKeys |= KEY_ZL; else g_padKeys &= ~KEY_ZL; break;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: if (v > 0.5f) g_padKeys |= KEY_ZR; else g_padKeys &= ~KEY_ZR; break; }
        break; }
    case SDL_KEYDOWN: if (!e.key.repeat) g_keyKeys |= keyMap(e.key.keysym.sym); break;
    case SDL_KEYUP: g_keyKeys &= ~keyMap(e.key.keysym.sym); break;
    case SDL_FINGERDOWN: case SDL_FINGERMOTION: {
        u16 x, y; if (toBottom(e.tfinger.x * g_winW, e.tfinger.y * g_winH, x, y)) { g_tp = {x, y}; g_touching = true; }
        break; }
    case SDL_FINGERUP: g_touching = false; break;
    case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEMOTION: {
        if (e.type == SDL_MOUSEMOTION && !(e.motion.state & SDL_BUTTON_LMASK)) break;
        if (e.type == SDL_MOUSEMOTION && e.motion.which == SDL_TOUCH_MOUSEID) break;
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.which == SDL_TOUCH_MOUSEID) break;
        int ww, wh, ow, oh; SDL_GetWindowSize(g_win, &ww, &wh); SDL_GetRendererOutputSize(g_ren, &ow, &oh);
        const float mx = (e.type == SDL_MOUSEMOTION ? e.motion.x : e.button.x) * (float)ow / ww, my = (e.type == SDL_MOUSEMOTION ? e.motion.y : e.button.y) * (float)oh / wh;
        u16 x, y; if (toBottom(mx, my, x, y)) { g_tp = {x, y}; g_touching = true; }
        break; }
    case SDL_MOUSEBUTTONUP: if (e.button.which != SDL_TOUCH_MOUSEID) g_touching = false; break;
    }
}
bool aptMainLoop() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) handle(e);
    if (g_paused && g_saveTurn) return !g_quit;   // one more game step so it can save (nothing is drawn)
    while (g_paused && !g_quit) { if (SDL_WaitEvent(&e)) handle(e); }   // then sleep until we're back
    return !g_quit;
}
void hidScanInput() {
    u32 k = g_padKeys | g_keyKeys;
    const float dz = 0.35f;
    if (g_lx > dz) k |= KEY_CPAD_RIGHT; if (g_lx < -dz) k |= KEY_CPAD_LEFT; if (g_ly > dz) k |= KEY_CPAD_DOWN; if (g_ly < -dz) k |= KEY_CPAD_UP;
    if (g_rx > dz) k |= KEY_CSTICK_RIGHT; if (g_rx < -dz) k |= KEY_CSTICK_LEFT; if (g_ry > dz) k |= KEY_CSTICK_DOWN; if (g_ry < -dz) k |= KEY_CSTICK_UP;
    if (g_touching) k |= KEY_TOUCH;
    g_circle = {(s16)(g_lx * 156), (s16)(-g_ly * 156)};
    g_down = k & ~g_prev; g_held = k; g_prev = k;
}
u32 hidKeysDown() { return g_down; }
u32 hidKeysHeld() { return g_held; }
void hidTouchRead(touchPosition* t) { *t = g_touching ? g_tp : g_tp; }
void hidCircleRead(circlePosition* c) { *c = g_circle; }
u64 osGetTime() { return SDL_GetTicks64(); }
void gfxInitDefault() {}
void gfxExit() {}
void svcSleepThread(long long ns) { SDL_Delay((Uint32)std::max(1LL, ns / 1000000LL)); }
bool platformPaused() { return g_paused; }
bool platformWantsSave() { const bool r = g_saveTurn; g_saveTurn = false; return r; }

// ============================================================ keyboard (Android's on-screen keyboard over a text box)
void swkbdInit(SwkbdState* s, int type, int, int maxLen) { memset(s, 0, sizeof *s); s->type = type; s->maxLen = maxLen > 0 ? maxLen : 64; }
void swkbdSetHintText(SwkbdState* s, const char* h) { snprintf(s->hint, sizeof s->hint, "%s", h ? h : ""); }
void swkbdSetInitialText(SwkbdState* s, const char* t) { snprintf(s->initial, sizeof s->initial, "%s", t ? t : ""); }
static void drawUi(const std::string& s, float x, float y, float size, u32 c, bool center) {   // window-space text
    C2D_Text t{s}; float w = textWidth(s) * size;
    drawString(s, center ? x - w / 2 : x, y, size, size, c);
}
SwkbdButton swkbdInputText(SwkbdState* s, char* buf, size_t size) {
    std::string text = s->initial;
    TexImpl win; win.k = 1.f;   // draw straight to the window
    SDL_StartTextInput();
    const SDL_Rect box = {g_winW / 2 - 300, g_scr[0].dst.y + 20, 600, 150};
    SDL_SetTextInputRect(&box);
    int result = SWKBD_BUTTON_NONE; u32 prev = g_padKeys | g_keyKeys;
    float blink = 0; Uint64 last = SDL_GetTicks64();
    while (result == SWKBD_BUTTON_NONE && !g_quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_TEXTINPUT) {
                for (const char* p = e.text.text; *p; p++) {
                    unsigned char ch = (unsigned char)*p;
                    if (ch < 32 || ch > 126) continue;   // the game's font is plain English letters
                    if (s->type == SWKBD_TYPE_NUMPAD && !(ch >= '0' && ch <= '9')) continue;
                    if ((int)text.size() < s->maxLen) text += (char)ch;
                }
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_BACKSPACE && !text.empty()) { text.pop_back(); continue; }
                if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) { result = SWKBD_BUTTON_CONFIRM; continue; }
                if (e.key.keysym.sym == SDLK_AC_BACK || e.key.keysym.sym == SDLK_ESCAPE) { result = SWKBD_BUTTON_LEFT; continue; }
                handle(e);
            } else if (e.type == SDL_FINGERDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                float fx, fy;
                if (e.type == SDL_FINGERDOWN) { fx = e.tfinger.x * g_winW; fy = e.tfinger.y * g_winH; }
                else { int ww, wh; SDL_GetWindowSize(g_win, &ww, &wh); fx = e.button.x * (float)g_winW / ww; fy = e.button.y * (float)g_winH / wh; }
                const float by = box.y + box.h - 52;
                if (fy >= by && fy <= by + 44) {
                    if (fx >= box.x + 20 && fx <= box.x + 200) result = SWKBD_BUTTON_LEFT;
                    else if (fx >= box.x + box.w - 200 && fx <= box.x + box.w - 20) result = SWKBD_BUTTON_CONFIRM;
                } else SDL_StartTextInput();   // tapping the box brings the keyboard back
            } else handle(e);
        }
        const u32 k = g_padKeys | g_keyKeys, dn = k & ~prev; prev = k;
        if (dn & KEY_START) result = SWKBD_BUTTON_CONFIRM;
        if (dn & (KEY_SELECT)) result = SWKBD_BUTTON_LEFT;
        if (g_paused) continue;
        const Uint64 now = SDL_GetTicks64(); blink += (now - last) / 1000.f; last = now;
        // last game frame underneath, dimmed
        present();
        flushBatch(); g_cur = &win; SDL_SetRenderTarget(g_ren, nullptr); C2D_ViewReset(); g_blend = SDL_BLENDMODE_BLEND;
        C2D_DrawRectSolid(0, 0, 0, (float)g_winW, (float)g_winH, C2D_Color32(40, 10, 30, 150));
        const float bx = (float)box.x, by = (float)box.y;
        C2D_DrawRectSolid(bx, by, 0, (float)box.w, (float)box.h, C2D_Color32(255, 240, 248, 250));
        C2D_DrawRectSolid(bx, by, 0, (float)box.w, 6, C2D_Color32(240, 100, 170, 255));
        drawUi(s->hint, bx + box.w / 2.f, by + 12, 0.9f, C2D_Color32(200, 60, 130, 255), true);
        C2D_DrawRectSolid(bx + 20, by + 48, 0, box.w - 40.f, 44, C2D_Color32(255, 255, 255, 255));
        std::string shown = text; while (!shown.empty() && textWidth(shown) * 1.1f > box.w - 70) shown.erase(0, 1);
        drawUi(shown + ((int)(blink * 2) % 2 ? "|" : " "), bx + 30, by + 52, 1.1f, C2D_Color32(90, 40, 80, 255), false);
        const float yb = by + box.h - 52;
        C2D_DrawRectSolid(bx + 20, yb, 0, 180, 44, C2D_Color32(220, 200, 230, 255)); drawUi("Cancel", bx + 110, yb + 6, 0.95f, C2D_Color32(255, 255, 255, 255), true);
        C2D_DrawRectSolid(bx + box.w - 200.f, yb, 0, 180, 44, C2D_Color32(240, 100, 170, 255)); drawUi("OK", bx + box.w - 110.f, yb + 6, 0.95f, C2D_Color32(255, 255, 255, 255), true);
        flushBatch(); g_cur = nullptr;
        SDL_RenderPresent(g_ren);
    }
    SDL_StopTextInput();
    g_padKeys = 0; g_keyKeys = 0; g_touching = false; g_prev = 0xFFFFFFFFu;   // don't let the closing press leak into the game
    snprintf(buf, size, "%s", result == SWKBD_BUTTON_CONFIRM ? text.c_str() : "");
    return result;
}

// ============================================================ sound: the 3DS DSP channels, mixed in software
struct Chn { float rate = 22050, mixL = 1, mixR = 1, frac = 0; std::vector<ndspWaveBuf*> q; };
static Chn g_ch[24];
static SDL_AudioDeviceID g_dev = 0;
static int g_outRate = 48000;
static void audioCb(void*, Uint8* stream, int len) {
    float* out = (float*)stream; const int frames = len / (int)(sizeof(float) * 2);
    memset(stream, 0, len);
    for (Chn& c : g_ch) {
        const float step = c.rate / g_outRate;
        for (int f = 0; f < frames && !c.q.empty(); ) {
            ndspWaveBuf* w = c.q.front();
            const s16* d = w->data_pcm16 ? w->data_pcm16 : (const s16*)w->data_vaddr;
            if (!d || !w->nsamples) { w->status = NDSP_WBUF_DONE; c.q.erase(c.q.begin()); continue; }
            w->status = NDSP_WBUF_PLAYING;
            while (f < frames && w->pos_ < w->nsamples) {
                const float sm = d[w->pos_] / 32768.f;
                out[f * 2] += sm * c.mixL; out[f * 2 + 1] += sm * c.mixR; f++;
                c.frac += step; const u32 adv = (u32)c.frac; c.frac -= adv; w->pos_ += adv;
            }
            if (w->pos_ >= w->nsamples) {
                if (w->looping) w->pos_ = 0;
                else { w->status = NDSP_WBUF_DONE; c.q.erase(c.q.begin()); }
            }
        }
    }
    for (int i = 0; i < frames * 2; i++) out[i] = std::clamp(out[i], -1.f, 1.f);
}
Result ndspInit() {
    SDL_AudioSpec want{}, have{};
    want.freq = 48000; want.format = AUDIO_F32SYS; want.channels = 2; want.samples = 1024; want.callback = audioCb;
    g_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!g_dev) return -1;
    g_outRate = have.freq;
    SDL_PauseAudioDevice(g_dev, 0);
    return 0;
}
void ndspExit() { if (g_dev) SDL_CloseAudioDevice(g_dev); g_dev = 0; }
struct ALock { ALock() { if (g_dev) SDL_LockAudioDevice(g_dev); } ~ALock() { if (g_dev) SDL_UnlockAudioDevice(g_dev); } };
void ndspChnReset(int ch) { ALock l; for (auto* w : g_ch[ch].q) w->status = NDSP_WBUF_DONE; g_ch[ch] = Chn{}; }
void ndspChnSetRate(int ch, float r) { ALock l; g_ch[ch].rate = r; }
void ndspChnSetMix(int ch, float* m) { ALock l; g_ch[ch].mixL = m[0]; g_ch[ch].mixR = m[1]; }
void ndspChnWaveBufAdd(int ch, ndspWaveBuf* w) { ALock l; w->pos_ = 0; w->status = NDSP_WBUF_QUEUED; g_ch[ch].q.push_back(w); }
void ndspChnWaveBufClear(int ch) { ALock l; for (auto* w : g_ch[ch].q) w->status = NDSP_WBUF_DONE; g_ch[ch].q.clear(); }

// ============================================================ start-up
extern "C" int SDL_main(int, char*[]) {
    SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "0");          // we pause ourselves after saving
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");        // Back = the B button
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight Portrait");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
#ifdef __ANDROID__
    g_win = SDL_CreateWindow("Sugarbaby Life 2", 0, 0, 0, 0, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
#else
    g_win = SDL_CreateWindow("Sugarbaby Life 2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#endif
    if (!g_win) { SDL_Log("window: %s", SDL_GetError()); return 1; }
    g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (!g_ren) g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_TARGETTEXTURE);
    if (!g_ren) { SDL_Log("renderer: %s", SDL_GetError()); return 1; }
    for (int i = 0; i < SDL_NumJoysticks(); i++) if (SDL_IsGameController(i)) { g_pad = SDL_GameControllerOpen(i); break; }
    g_scr[0].gw = 400; g_scr[0].gh = 240; g_scr[1].gw = 320; g_scr[1].gh = 240;
    for (int i = 0; i < 2; i++) { g_scr[i].impl.target = true; g_scr[i].rt = {&g_scr[i].impl, i}; }
    layout();
    if (!loadFont()) SDL_Log("font.ttf missing - text will not show");
    // saves: the game writes to "sdmc:/..." - a folder of that name inside the app's private storage
    const char* env = getenv("SBL2_DATA");
    char* pref = env ? nullptr : SDL_GetPrefPath("Sugarbaby", "SugarbabyLife2");
    const std::string dir = env ? env : (pref ? pref : ".");
    if (pref) SDL_free(pref);
    if (chdir(dir.c_str()) != 0) SDL_Log("chdir %s failed", dir.c_str());
    mkdir("sdmc:", 0777); mkdir("romfs:", 0777);
    const int r = sbl2_main();
    SDL_Quit();
    return r;
}
#ifndef __ANDROID__
int main(int argc, char* argv[]) { return SDL_main(argc, argv); }
#endif
