// ============================================================ Glam Doll style (default)
// Semi-realistic fashion-doll look: human proportions (about 6.5 heads tall),
// detailed faces built from parts the player can customise (face shape, eyes,
// brows, nose, lips, makeup, freckles, glasses, piercings, facial hair) and a
// wide range of skin tones and hair textures. Everything is drawn from shapes,
// so it stays crisp on the 3DS screens at any size.
#include "people.hpp"
#include "draw.hpp"
#include <cmath>

using namespace col;

// helpers shared with people.cpp
void topPatternOn(const Pen& p, float yTop, float yBot, float lTop, float rTop, float lBot, float rBot, int pat, float seed);
void drawHat(const Pen& p, int holiday, float t);

// ---------------------------------------------------------------- palettes
// 0..5 are the original six tones (old saves keep their colour); 6..9 were added.
static const RGB GSKIN[NUM_SKINS] = {
    {255, 226, 206}, {244, 198, 164}, {228, 176, 132}, {200, 138, 96}, {160, 106, 74}, {110, 70, 50},
    {252, 216, 200}, {216, 172, 122}, {134, 86, 58}, {82, 52, 38}};
const char* SKIN_NAMES[NUM_SKINS] = {"Porcelain", "Light", "Medium", "Tan", "Brown", "Deep",
                                     "Rosy Fair", "Golden Olive", "Warm Brown", "Ebony"};
const uint8_t SKIN_ORDER[NUM_SKINS] = {0, 6, 1, 2, 7, 3, 4, 8, 5, 9};
static const RGB GHAIR[NUM_HAIRCOLS] = {
    {30, 22, 26}, {78, 46, 32}, {160, 102, 56}, {234, 196, 118}, {246, 236, 210},
    {160, 52, 38}, {240, 120, 172}, {206, 202, 206}, {170, 140, 220}, {120, 176, 230}};
const char* HAIRCOL_NAMES[NUM_HAIRCOLS] = {"Jet Black", "Espresso", "Caramel", "Honey Blonde", "Platinum",
                                           "Cherry Red", "Bubblegum", "Silver", "Lavender", "Ice Blue"};
RGB skinRGB(int i) { return GSKIN[((i % NUM_SKINS) + NUM_SKINS) % NUM_SKINS]; }
RGB hairRGB(int i) { return GHAIR[((i % NUM_HAIRCOLS) + NUM_HAIRCOLS) % NUM_HAIRCOLS]; }
// 10 choices each (shared by every art style)
static const RGB IRIS10[10] = {{112, 70, 40}, {58, 36, 28}, {142, 112, 54}, {70, 142, 92}, {72, 124, 204}, {128, 140, 152}, {146, 92, 186}, {200, 140, 40}, {40, 170, 170}, {200, 60, 70}};
static const RGB SHADOW10[10] = {{0, 0, 0}, {255, 120, 186}, {74, 62, 84}, {232, 190, 98}, {192, 142, 232}, {110, 162, 232}, {170, 108, 70}, {80, 200, 150}, {255, 150, 70}, {200, 40, 90}};
static const RGB BLUSH10[10] = {{0, 0, 0}, {255, 116, 156}, {255, 150, 118}, {204, 60, 112}, {172, 100, 60}, {255, 90, 120}, {240, 130, 200}, {255, 180, 140}, {230, 110, 90}, {255, 200, 120}};
RGB eyeRGB(int i) { return IRIS10[((i % 10) + 10) % 10]; }
RGB shadowRGB(int i) { return SHADOW10[((i % 10) + 10) % 10]; }
RGB blushRGB(int i) { return BLUSH10[((i % 10) + 10) % 10]; }
#define IRIS IRIS10
#define SHADOWS SHADOW10
#define BLUSHES BLUSH10

FaceDef randomFace(bool guy, uint32_t seed) {
    uint32_t x = seed * 2654435761u + 0x9E3779B9u;
    auto r = [&](int n) { x ^= x << 13; x ^= x >> 17; x ^= x << 5; return (uint8_t)(x % (uint32_t)n); };
    FaceDef f;
    { const int b = r(10); f.pad = (uint8_t)(b < 5 ? 0 : b < 8 ? 1 : 2); }   // body type: slim / mid-size / plus-size
    f.shape = r(5); f.eyes = r(6); f.eyeCol = r(7); f.brows = r(5); f.nose = r(4); f.lipShape = r(4);
    if (guy) {
        f.shadow = 0; f.liner = r(8) == 0 ? 1 : 0; f.blush = 0;
        f.beard = r(3) == 0 ? (uint8_t)(1 + r(3)) : 0;
        if (f.brows == 3) f.brows = 2;
    } else {
        f.shadow = r(7); f.liner = r(3); f.blush = 0;   // no blush unless the player adds it
    }
    f.marks = r(5) == 0 ? (uint8_t)(1 + r(3)) : 0;
    f.glasses = r(6) == 0 ? (uint8_t)(1 + r(4)) : 0;
    f.pierce = guy ? (r(5) == 0 ? 1 : 0) : r(5);
    return f;
}

// ---------------------------------------------------------------- small helpers
static float g_cutY = 1e9f;   // close-ups cut long hair off at the frame
static inline float cut(float y) { return y < g_cutY ? y : g_cutY; }

static void ring(const Pen& p, float cx, float cy, float rx, float ry, float th, u32 c) {
    const int N = 14;
    for (int i = 0; i < N; i++) {
        float a0 = i * 6.2831853f / N, a1 = (i + 1) * 6.2831853f / N;
        p.L(cx + std::cos(a0) * rx, cy + std::sin(a0) * ry, cx + std::cos(a1) * rx, cy + std::sin(a1) * ry, th, c);
    }
}

struct GlamCtx {
    const PersonDef& d;
    FaceDef f;
    RGB skin, hair;
    u32 SK, SKD, SKS, HR, HRD, HRL;
    bool kid, guy;
    float chinY;
};

// ---------------------------------------------------------------- hair
static void hairBack(const Pen& p, const GlamCtx& g, float t) {
    const u32 HR = g.HR, HRD = g.HRD, HRL = g.HRL;
    const int hs = g.d.hairStyle % NUM_HAIRSTYLES;
    const float sw = std::sin(t * 2.f) * 1.4f;
    if (hs != 5 && hs != 10 && hs != 8 && hs != 6) p.E(20, 6.4f, 16.4f, 15.4f, HR);   // volume behind the head
    switch (hs) {
    case 0:   // sleek & long
        p.Q(12.6f, 6, 27.4f, 6, 28.4f, cut(38), 11.6f, cut(38), HR);
        if (g_cutY > 42) { p.T(11.6f, 38, 16, 38, 12.8f, 41, HR); p.T(28.4f, 38, 24, 38, 27.2f, 41, HR); }
        p.L(13.6f, 10, 13.2f, cut(34), 0.6f, HRL);
        break;
    case 1:   // big glam waves
        for (int i = 0; i < 5; i++) {   // stacked, gently swaying sections
            float y0 = 6 + i * 6.f, y1 = y0 + 6, o0 = (i % 2) ? 1.2f : 0.f, o1 = (i % 2) ? 0.f : 1.2f;
            if (y0 > g_cutY) break;
            p.Q(11.4f - o0, y0, 28.6f + o0, y0, 28.6f + o1, cut(y1), 11.4f - o1, cut(y1), HR);
        }
        if (g_cutY > 38) { p.T(11.4f, 36, 16, 36, 12.6f, 39, HR); p.T(28.6f, 36, 24, 36, 27.4f, 39, HR); }
        p.L(13.2f, 9, 12.4f, cut(18), 0.6f, HRL); p.L(12.4f, 18, 13.4f, cut(30), 0.6f, HRL);
        break;
    case 2:   // high ponytail
        p.Ci(26.6f, 0.4f, 3.2f, HR);
        p.Q(25.6f, 0.4f, 29.6f, 1.2f, 31.2f + sw, cut(30), 27.6f + sw, cut(29), HR);
        p.L(28, 3, 29.6f + sw, cut(26), 0.5f, HRL);
        break;
    case 3:   // chin bob
        p.Q(12.2f, 6, 27.8f, 6, 28.6f, 19, 11.4f, 19, HR);
        p.E(13.2f, 18.6f, 4, 2.4f, HR); p.E(26.8f, 18.6f, 4, 2.4f, HR);
        break;
    case 4:   // top bun
        p.Ci(20, -3.6f, 4.4f, HR); p.L(17, -5.5f, 22, -6.4f, 0.6f, HRL);
        break;
    case 6: {   // big curls
        p.E(20, 6, 24, 20, HR);
        p.Q(9, 8, 31, 8, 30, cut(27), 10, cut(27), HR);
        if (g_cutY > 28) p.E(20, 27, 20, 4, HR);
        p.L(11.2f, 6, 10.6f, cut(14), 0.6f, HRL); p.L(10.8f, 16, 11.6f, cut(24), 0.6f, HRL);
        p.L(28.8f, 8, 29.4f, cut(16), 0.6f, HRL); p.L(16, -2.6f, 21, -3.4f, 0.6f, HRL);
        break;
    }
    case 7:   // box braids
        for (int i = 0; i < 10; i++) {
            float x0 = 12.6f + i * 1.65f, len = 40 - std::fabs(i - 4.5f) * 1.2f;
            float x1 = x0 + (x0 - 20) * 0.22f + sw * 0.2f;
            p.L(x0, 6, x1, cut(len), 1.5f, (i & 1) ? HRD : HR);
            if (len < g_cutY) p.R(x1 - 0.8f, len - 2.2f, 1.6f, 1.1f, C(GOLD));
        }
        break;
    case 8:   // afro
        p.E(20, 5, 27, 25, HR);
        p.L(12.6f, -1, 15.6f, -4.8f, 0.7f, HRL); p.L(17.6f, -6, 22, -6.4f, 0.7f, HRL);
        break;
    case 9:   // afro puffs
        for (int k = 0; k < 2; k++) {
            float cx = k ? 26.6f : 13.4f;
            p.Ci(cx, -1.8f, 5.4f, HR);
            p.L(cx - 3, -4, cx - 0.6f, -5.6f, 0.6f, HRL);
        }
        break;
    case 10:  // pixie
        p.E(20, 5, 15.6f, 12.4f, HR);
        break;
    case 11:  // locs
        for (int i = 0; i < 8; i++) {
            float x0 = 13 + i * 2.f, len = 34 - std::fabs(i - 3.5f) * 1.4f;
            float x1 = x0 + (x0 - 20) * 0.3f + sw * 0.25f;
            p.L(x0, 6, x1, cut(len), 2.0f, (i & 1) ? HRD : HR);
            for (float y = 14; y < cut(len) - 1; y += 5) { float k = (y - 6) / (len - 6); p.L(x0 + (x1 - x0) * k - 0.9f, y, x0 + (x1 - x0) * k + 0.9f, y + 0.6f, 0.3f, HRL); }
        }
        break;
    default:  // 5 short crop
        p.E(20, 4.6f, 15.0f, 11.6f, HR);
        break;
    }
    (void)HRL;
}

static void hairShine(const Pen& p, const GlamCtx& g) {   // anime-style glossy band (smooth style only)
    if (g_pix) return;
    const u32 SH = C(mix(g.hair, RGB{255, 240, 250}, 0.4f), 70);
    p.E(16.2f, 0.9f, 3.6f, 1.0f, SH); p.E(20.0f, 0.0f, 3.8f, 0.9f, SH); p.E(23.8f, 0.9f, 3.6f, 1.0f, SH);
}

static void hairFront(const Pen& p, const GlamCtx& g, float t) {
    const u32 HR = g.HR, HRD = g.HRD, HRL = g.HRL;
    const int hs = g.d.hairStyle % NUM_HAIRSTYLES;
    (void)t;
    switch (hs) {
    case 0:   // side part with a sweeping bang and front curtains
        p.E(20, 1.6f, 14.8f, 5.8f, HR);
        p.T(13.2f, 1.4f, 24.6f, 1.2f, 14.0f, 8.6f, HR);
        p.Q(12.4f, 2, 15.2f, 3, 14.8f, cut(30), 12.0f, cut(30), HR);
        p.Q(25.0f, 2.4f, 27.6f, 2, 28.2f, cut(30), 25.6f, cut(30), HR);
        p.L(14.6f, 2.2f, 21.6f, 0.4f, 0.6f, HRL); p.L(13.2f, 8, 13.4f, cut(26), 0.5f, HRL);
        break;
    case 1:   // middle part, face-framing waves
        p.E(20, 1.6f, 14.8f, 5.8f, HR);
        p.Q(20, 0, 14.4f, 1, 12.8f, 8.4f, 15.4f, 2.6f, HR); p.Q(20, 0, 25.6f, 1, 27.2f, 8.4f, 24.6f, 2.6f, HR);
        p.Q(11.8f, 4, 15.0f, 4, 14.4f, cut(24), 11.2f, cut(24), HR); p.Q(25.0f, 4, 28.2f, 4, 28.8f, cut(24), 25.6f, cut(24), HR);
        p.L(20, -0.8f, 20, 2, 0.4f, HRD);
        p.L(16, 0.4f, 13.8f, 6, 0.5f, HRL); p.L(24, 0.4f, 26.2f, 6, 0.5f, HRL);
        break;
    case 3:   // blunt bangs bob
        p.E(20, 1.2f, 14.8f, 5.4f, HR);
        p.R(13.4f, 0.8f, 13.2f, 4.6f, HR);
        p.Q(12.4f, 2, 14.8f, 2, 14.6f, 18.4f, 12.0f, 18.4f, HR); p.Q(25.2f, 2, 27.6f, 2, 28.0f, 18.4f, 25.4f, 18.4f, HR);
        for (int i = 0; i < 5; i++) p.L(14.6f + i * 2.6f, 2.4f, 14.4f + i * 2.6f, 5.2f, 0.25f, HRD);
        p.L(15, 1.0f, 24.5f, 0.8f, 0.6f, HRL);
        break;
    case 5:   // short crop with textured top
        p.E(20, 1.6f, 14.6f, 5.2f, HR);
        p.T(14, 1.6f, 17, -2.4f, 19.6f, 1.4f, HR); p.T(17.6f, 1.2f, 21.4f, -2.8f, 24, 1.4f, HR); p.T(21.8f, 1.6f, 25.6f, -1.4f, 26.6f, 3, HR);
        p.R(13.1f, 2.6f, 1.3f, 6.4f, HR); p.R(25.6f, 2.6f, 1.3f, 6.4f, HR);
        p.L(15.4f, 0.2f, 22, -0.8f, 0.5f, HRL);
        break;
    case 6:   // curly fringe
        p.E(20, 1.4f, 14.8f, 5.4f, HR);
        p.Q(13, 1, 27, 1, 26, 4.6f, 14, 4.6f, HR);
        p.Q(11.6f, 3, 14.6f, 3, 14.2f, 11, 11.4f, 11, HR); p.Q(25.4f, 3, 28.4f, 3, 28.6f, 11, 25.8f, 11, HR);
        p.L(15.4f, 0.4f, 19, -0.4f, 0.5f, HRL);
        break;
    case 7:   // braids: parted crown + braids draped in front of the shoulders
        p.E(20, 1.6f, 14.6f, 5.4f, HR);
        p.L(20, -0.8f, 20, 3.6f, 0.35f, HRD); p.L(16.4f, -0.2f, 15, 4, 0.3f, HRD); p.L(23.6f, -0.2f, 25, 4, 0.3f, HRD);
        for (int k = 0; k < 2; k++) {
            float x0 = k ? 26.6f : 13.4f, dir = k ? 1.f : -1.f;
            for (int j = 0; j < 2; j++) {
                float xs = x0 + dir * j * 1.4f;
                p.L(xs, 4, xs + dir * 0.6f, cut(40 - j * 3), 1.4f, j ? HRD : HR);
                for (float y = 8; y < cut(40 - j * 3); y += 2.4f) p.L(xs - 0.6f + dir * 0.6f * (y / 40), y, xs + 0.6f + dir * 0.6f * (y / 40), y + 0.9f, 0.25f, HRL);
            }
        }
        break;
    case 8:   // afro hairline
        p.E(20, 1.8f, 15.2f, 6.0f, HR);
        p.Q(13.4f, 2, 26.6f, 2, 26, 4, 14, 4, HR);
        break;
    case 9:   // sleek centre part for puffs
        p.E(20, 1.8f, 14.6f, 5.6f, HR);
        p.L(20, -1, 20, 3.4f, 0.35f, HRD);
        p.L(16.5f, 0.6f, 14.6f, 4, 0.4f, HRL);
        p.R(12.2f, -2.2f, 2.4f, 1.6f, C(HOT)); p.R(25.4f, -2.2f, 2.4f, 1.6f, C(HOT));
        break;
    case 10:  // pixie swoop
        p.E(20, 1.4f, 14.8f, 5.4f, HR);
        p.T(13.2f, 0.6f, 26.4f, 0.4f, 14.4f, 7.2f, HR);
        p.T(13, 2, 14.6f, 2, 13.4f, 10, HR); p.T(25.4f, 2, 27, 2, 26.6f, 9, HR);
        p.L(15, 1.6f, 23.6f, 0.4f, 0.55f, HRL);
        break;
    case 11:  // locs, half-up
        p.E(20, 1.4f, 14.8f, 5.6f, HR);
        p.Ci(20, -2.8f, 2.6f, HR);
        p.L(13.2f, 3, 12.6f, cut(30), 1.8f, HR); p.L(26.8f, 3, 27.6f, cut(30), 1.8f, HRD);
        p.L(14.8f, 2, 19, -2, 0.4f, HRL);
        break;
    default:  // 2 ponytail / 4 bun: sleek pulled back
        p.E(20, 1.8f, 14.6f, 5.6f, HR);
        p.Q(13.2f, 3, 14.6f, 2, 14.4f, 7.6f, 13.2f, 8.2f, HR); p.Q(25.4f, 2, 26.8f, 3, 26.8f, 8.2f, 25.6f, 7.6f, HR);
        p.L(15.6f, 0.4f, 19.4f, -0.8f, 0.5f, HRL); p.L(21, -0.8f, 24.4f, 0.6f, 0.4f, HRL);
        if (hs == 2) p.R(25.2f, -0.6f, 2.0f, 2.2f, C(HOT));
        break;
    }
}

// ---------------------------------------------------------------- face
struct EyeShape { float w, h, tilt; bool crease; };
static const EyeShape EYES[6] = {{4.5f, 2.6f, 0.15f, true}, {4.2f, 3.0f, 0.05f, true}, {4.7f, 2.2f, 0.8f, true},
                                 {4.8f, 3.2f, 0.15f, true}, {4.5f, 2.1f, 0.25f, true}, {4.5f, 1.9f, 0.3f, false}};

static void drawEye(const Pen& p, const GlamCtx& g, float ex, float ey, float dir, bool closed, float look) {
    EyeShape es = EYES[g.f.eyes % 6];
    float w = es.w, h = es.h;
    if (g.guy) { w *= 0.94f; h *= 0.86f; }
    if (g.kid) { w *= 1.1f; h *= 1.18f; }
    const float tl = es.tilt * 0.6f;
    // rounded, open "doe" lid: inner corner a touch higher than before and the lid's peak slightly
    // toward the inner side, which reads as friendly instead of a glare
    const float ix = ex - dir * w / 2, iy = ey - 0.1f;      // inner corner
    const float ox = ex + dir * w / 2, oy = ey - tl * 0.5f; // outer corner
    const float tx = ex - dir * 0.15f, ty = ey - h / 2 - 0.1f;
    const float bx = ex + dir * 0.1f, by = ey + h / 2;
    const u32 DK = C(70, 40, 48), DKS = C(70, 40, 48, 90);
    const bool makeup = !g.kid && g.f.shadow > 0;
    if (makeup) {
        RGB sc = SHADOWS[g.f.shadow % 10];
        p.E(ex + dir * 0.3f, ey - h * 0.55f, w + 1.0f, h * 0.8f, C(mix(sc, g.skin, 0.3f), 70));
        if (g.f.shadow == 2) p.E(ex + dir * 0.8f, ey - h * 0.4f, w * 0.6f, h * 0.6f, C(sc, 50));
        p.Ci(ix + dir * 0.4f, iy - 0.2f, 0.5f, C(255, 255, 255, 110));   // inner-corner highlight
    }
    // crease
    if (es.crease) {
        float ch = g.f.eyes == 4 ? 0.45f : 0.95f;
        p.L(ix + dir * 0.6f, ty - ch + 0.4f, tx, ty - ch, 0.3f, C(g.skin.r * 0.7f, g.skin.g * 0.58f, g.skin.b * 0.58f, 60));
        p.L(tx, ty - ch, ox - dir * 0.3f, oy - ch * 0.6f, 0.3f, C(g.skin.r * 0.7f, g.skin.g * 0.58f, g.skin.b * 0.58f, 60));
    }
    if (closed) {
        p.L(ix, iy, ex, ey + 0.55f, 0.5f, DK); p.L(ex, ey + 0.55f, ox, oy + 0.2f, 0.5f, DK);
        if (!g.guy) { p.L(ox, oy + 0.2f, ox + dir * 0.8f, oy + 0.9f, 0.35f, DK); p.L(ex + dir * 0.8f, ey + 0.6f, ex + dir * 1.2f, ey + 1.4f, 0.3f, DK); }
        return;
    }
    // white of the eye
    const u32 WH = C(252, 249, 246);
    p.Q(ix, iy, tx, ty, ox, oy, bx, by, WH);
    p.E(ex, ey - tl * 0.4f, w * 0.7f, h * 0.96f, WH);
    // iris, pupil, catch-lights
    const RGB ir = IRIS[g.f.eyeCol % 10];
    float r = h * 0.47f;
    if (r < 0.8f) r = 0.8f;
    float icx = ex + look * 0.5f, icy = ey - tl * 0.35f + 0.05f;
    p.Ci(icx, icy, r + 0.12f, C(shade(ir, 0.55f)));
    p.Ci(icx, icy, r, C(ir));
    p.Ci(icx, icy + r * 0.3f, r * 0.55f, C(shade(ir, 1.25f), 120));
    p.Ci(icx, icy, r * 0.46f, C(18, 12, 14));
    p.Ci(icx + 0.35f, icy - 0.35f, 0.32f, C(WHITE));
    p.Ci(icx - 0.3f, icy + 0.35f, 0.15f, C(255, 255, 255, 200));
    // upper lid line (+ eyeliner) and lashes
    float th = g.guy ? 0.36f : 0.42f;
    if (!g.kid && g.f.liner) th += 0.08f;
    p.L(ix, iy, tx, ty, th, DK); p.L(tx, ty, ox, oy, th, DK);
    if (!g.kid && g.f.liner == 2) { p.T(ox - dir * 0.4f, oy - 0.15f, ox + dir * 0.9f, oy - 0.7f, ox, oy + 0.15f, DK); }
    if (!g.guy) {
        p.L(ox, oy, ox + dir * 0.85f, oy - 0.75f, 0.32f, DK);
        p.L(ex + dir * 1.0f, ty + 0.1f, ex + dir * 1.5f, ty - 0.7f, 0.28f, DK);
        p.L(ex, ty, ex + dir * 0.2f, ty - 0.7f, 0.25f, DK);
    }
    // lower lash line
    p.L(ix + dir * 0.4f, iy + 0.1f, bx, by, 0.22f, DKS); p.L(bx, by, ox, oy + 0.1f, 0.22f, DKS);
}

static void drawBrow(const Pen& p, const GlamCtx& g, float ex, float ey, float dir) {
    static const float TH[5] = {0.36f, 0.4f, 0.55f, 0.32f, 0.62f}, AR[5] = {0.4f, 0.1f, 0.35f, 0.7f, 0.25f};
    int b = g.f.brows % 5;
    float th = TH[b] * (g.guy ? 1.2f : 1.f), ar = AR[b];
    const int hc = g.d.hairCol % NUM_HAIRCOLS;
    RGB bc = (hc == 3 || hc == 4 || hc >= 6) ? RGB{140, 100, 74} : mix(shade(g.hair, 0.95f), g.skin, 0.2f);
    if (hc == 0) bc = RGB{78, 56, 56};
    const u32 BC = C(bc);
    float by = ey - 3.1f - (g.f.eyes == 3 || g.f.eyes == 1 ? 0.4f : 0.f);
    // soft, slightly raised arch: the inner end sits higher than the tail so the look stays sweet, never cross
    float ix = ex - dir * 1.7f, px = ex + dir * 0.5f, tx = ex + dir * 2.5f;
    p.L(ix, by - 0.15f, px, by - 0.25f - ar, th, BC);
    p.L(px, by - 0.25f - ar, tx, by + 0.35f, th * 0.75f, BC);
    p.Ci(ix, by - 0.15f, th * 0.5f, BC);
}


// ---------------------------------------------------------------- pixel face
// At room size a head is only ~13 pixels tall, so the face is hand-placed pixel by
// pixel (like a sprite artist would) instead of being drawn with shapes.
static inline void px(int tx, int ty, u32 c) { rect(tx * 2.f, ty * 2.f, 2.f, 2.f, c); }

static void pixFace(const Pen& p, const GlamCtx& g, Pose pose, float t) {
    const FaceDef& f = g.f;
    const RGB sk = g.skin;
    const int cx = (int)std::floor(p.X(20) / 2.f + 0.5f);          // columns cx-1 | cx straddle the centre
    const int ey = (int)std::floor(p.Y(9.8f) / 2.f);
    const int chin = (int)std::floor(p.Y(g.chinY) / 2.f);
    const int my = std::min(ey + 4, chin - 1);
    auto both = [&](int o, int y, u32 c) { px(cx - 1 - o, y, c); px(cx + o, y, c); };
    const u32 DK = C(40, 22, 32);
    const bool closed = pose == PO_SLEEP || pose == PO_SITSLEEP || std::fmod(t * 0.9f + g.d.skin * 0.37f + g.d.hairCol * 0.21f, 3.7f) < 0.12f;
    const bool girl = !g.guy, grown = !g.kid;
    const int es = f.eyes % 6;
    const u32 LASH = C(58, 34, 40);
    // eyeshadow: one soft row of colour on the lid
    if (grown && f.shadow) { RGB sc = mix(SHADOWS[f.shadow % 10], sk, 0.35f); both(1, ey - 2, C(sc, 120)); both(2, ey - 2, C(sc, 120)); }
    // brows: two soft pixels and a tail
    {
        const int hc = g.d.hairCol % NUM_HAIRCOLS;
        RGB bc = (hc == 3 || hc == 4 || hc >= 6) ? RGB{140, 100, 74} : mix(shade(g.hair, 0.9f), sk, 0.22f);
        if (hc == 0) bc = RGB{74, 52, 52};
        const u32 B = C(bc, 235);
        int by = ey - 3 - (f.brows == 3 ? 1 : 0);
        both(1, by, B); both(2, by, B);
        if (f.brows == 1 || f.brows == 2 || f.brows == 4) both(3, by, C(bc, 170)); else both(3, by + 1, C(bc, 170));
        if (f.brows == 4 || (g.guy && f.brows == 2)) both(1, by - 1, C(bc, 110));
    }
    // eyes: lash line over a two-tone iris; round/doe eyes get a catch-light
    const RGB ir = IRIS[f.eyeCol % 10];
    if (closed) {
        both(1, ey, LASH); both(2, ey, LASH);
    } else {
        both(1, ey - 1, LASH); both(2, ey - 1, LASH);
        if (girl && grown && f.liner) both(3, ey - 1, C(58, 34, 40, 200));   // tiny flick
        both(1, ey, C(shade(ir, 0.62f)));
        both(2, ey, C(ir));
        if (es == 1 || es == 3) {
            both(1, ey + 1, C(ir)); both(2, ey + 1, C(shade(ir, 1.25f)));
            px(cx - 3, ey, C(255, 255, 255)); px(cx + 2, ey, C(255, 255, 255));   // sparkle on the outer side
        }
    }
    // blush & freckles
    if (girl && f.blush) both(3, ey + 2, C(BLUSHES[f.blush % 10], 85));
    else if (g.kid) both(3, ey + 2, C(255, 130, 150, 70));
    if (f.marks & 1) { px(cx - 3, ey + 2, C(shade(sk, 0.72f), 150)); px(cx + 2, ey + 2, C(shade(sk, 0.72f), 150)); px(cx - 2, ey + 1, C(shade(sk, 0.76f), 110)); px(cx + 1, ey + 1, C(shade(sk, 0.76f), 110)); }
    // nose: a single soft shadow pixel
    px(cx, ey + 2, C(shade(sk, 0.8f), f.nose == 2 ? 190 : 150));
    if (f.nose == 2) px(cx - 1, ey + 2, C(shade(sk, 0.84f), 100));
    // mouth: two pixels, colour softened toward the skin so it never shouts
    RGB lc = mix(g.d.lips, sk, girl && grown ? 0.3f : 0.55f);
    if (g.guy || g.kid) lc = mix(shade(sk, 0.82f), RGB{214, 120, 126}, g.kid ? 0.35f : 0.2f);
    const int ls = f.lipShape % 4;
    both(0, my, C(lc));
    if (!closed || pose != PO_SLEEP) both(1, my - 1, C(shade(lc, 0.85f), girl ? 150 : 110));   // little smile corners
    if (girl && grown && ls == 3) both(1, my, C(lc, 120));
    if (girl && grown && (ls == 0 || ls == 1) && my + 1 < chin) px(cx - 1 + (cx & 1), my + 1, C(shade(lc, 1.18f), 120));
    if (f.marks & 2) px(cx + 2, my, C(70, 44, 40));
    // facial hair accents
    if (g.guy && grown && f.beard >= 2) { both(1, my - 1, C(shade(g.hair, 0.85f), 200)); both(0, my + 1, C(shade(g.hair, 0.85f), 200)); }
    // piercings
    if (f.pierce == 1) both(4, ey + 2, C(236, 236, 244));
    if (f.pierce == 2 || f.pierce == 4) { both(4, ey + 2, C(GOLD)); both(4, ey + 3, C(GOLD)); }
    if (f.pierce >= 3) px(cx - 2, ey + 2, C(250, 250, 255));
    // glasses
    if (f.glasses) {
        const u32 FR = f.glasses == 2 ? C(BERRY) : f.glasses == 4 ? C(210, 210, 220, 150) : C(36, 26, 32);
        for (int o = 1; o <= 3; o++) { both(o, ey - 2, FR); both(o, ey + 1, FR); }
        both(0, ey - 1, FR); both(4, ey - 1, FR); both(4, ey, FR);
        if (f.glasses == 2) both(4, ey - 3, FR);
    }
}

static void drawFace(const Pen& p, GlamCtx& g, Pose pose, float t) {
    const FaceDef& f = g.f;
    const RGB sk = g.skin;
    const u32 SK = g.SK, SKS = g.SKS;
    // face-shape parameters
    float jw = 6.9f, cw = 2.6f, chinY = 17.3f, cranW = 15.0f;
    switch (f.shape % 5) {
    case 1: jw = 7.2f; cw = 3.3f; chinY = 17.0f; cranW = 15.6f; break;   // round
    case 2: jw = 6.8f; cw = 1.8f; chinY = 17.6f; cranW = 15.4f; break;   // heart
    case 3: jw = 7.2f; cw = 3.4f; chinY = 17.4f; cranW = 15.2f; break;                  // square
    case 4: jw = 6.6f; cw = 2.4f; chinY = 18.2f; cranW = 14.4f; break;   // long
    default: break;
    }
    if (g.kid) { cw += 0.6f; chinY -= 0.6f; cranW += 0.6f; }
    if (g.guy) { jw += 0.3f; cw += 0.5f; }
    g.chinY = chinY;
    // shadow cast on the neck, ears, head
    if (!g_pix) p.E(20, chinY + 0.8f, 5.6f, 1.8f, C(shade(sk, 0.82f), 70));
    p.E(13.3f, 10.3f, 2.1f, 3.6f, SK); p.E(26.7f, 10.3f, 2.1f, 3.6f, SK);
    p.E(13.4f, 10.4f, 0.9f, 2.0f, C(shade(sk, 0.82f)));
    p.E(20, 8.4f, cranW, 15.4f, SK);
    p.Q(20 - jw, 10, 20 + jw, 10, 20 + cw, chinY - 0.4f, 20 - cw, chinY - 0.4f, SK);
    p.E(20, chinY - 0.7f, cw * 2 + 1.4f, 1.6f, SK);
    // soft contour on the far side + cheek highlight (the "3D" look)
    p.Q(20 + jw - 1.2f, 10, 20 + jw, 10, 20 + cw, chinY - 0.4f, 20 + cw - 0.8f, chinY - 0.6f, C(shade(sk, 0.9f), 120));
    p.E(16.0f, 12.0f, 2.6f, 1.2f, C(255, 255, 255, 34));
    // blush
    if (f.blush && !g.guy) { RGB bc = BLUSHES[f.blush % 10]; p.E(15.4f, 13.1f, 3.2f, 1.7f, C(bc, g.kid ? 50 : 72)); p.E(24.6f, 13.1f, 3.2f, 1.7f, C(bc, g.kid ? 50 : 72)); }
    else if (g.kid) { p.E(15.4f, 13.1f, 2.8f, 1.4f, C(255, 130, 150, 50)); p.E(24.6f, 13.1f, 2.8f, 1.4f, C(255, 130, 150, 50)); }
    // facial hair (under the features)
    if (g.guy && !g.kid && f.beard) {
        RGB hcol = (g.d.hairCol % NUM_HAIRCOLS) >= 6 ? RGB{90, 60, 46} : g.hair;
        if (f.beard == 1) { p.Q(20 - jw + 0.6f, 12.2f, 20 + jw - 0.6f, 12.2f, 20 + cw, chinY - 0.3f, 20 - cw, chinY - 0.3f, C(hcol, 52)); }
        else if (f.beard == 2) { p.E(20, chinY - 1.2f, cw * 2 + 0.6f, 2.6f, C(shade(hcol, 0.9f), 220)); p.E(20, 14.7f, 4.2f, 0.9f, C(shade(hcol, 0.9f), 220)); }
        else { p.Q(20 - jw + 0.3f, 11.4f, 20 + jw - 0.3f, 11.4f, 20 + cw + 0.4f, chinY + 0.2f, 20 - cw - 0.4f, chinY + 0.2f, C(shade(hcol, 0.9f), 235)); p.E(20, chinY - 0.4f, cw * 2 + 2, 2.4f, C(shade(hcol, 0.9f), 235)); p.E(20, 15.6f, 3.6f, 1.6f, SK); }
    }
    if (g_pix && p.s * 19.5f < 36.f) { pixFace(p, g, pose, t); return; }   // small head: pixel-placed face
    // nose
    const u32 NS = C(shade(sk, 0.74f), 80), NH = C(255, 255, 255, 0);
    switch (f.nose % 4) {
    case 1:   // straight
        p.L(21.0f, 10.4f, 21.3f, 13.1f, 0.35f, C(shade(sk, 0.78f), 120));
        p.Ci(19.2f, 13.9f, 0.34f, NS); p.Ci(20.8f, 13.9f, 0.34f, NS); p.L(19.8f, 10.8f, 19.8f, 12.8f, 0.3f, NH);
        break;
    case 2:   // wide
        p.E(18.9f, 13.7f, 1.4f, 0.9f, C(shade(sk, 0.86f), 140)); p.E(21.1f, 13.7f, 1.4f, 0.9f, C(shade(sk, 0.86f), 140));
        p.Ci(19.0f, 13.9f, 0.38f, NS); p.Ci(21.0f, 13.9f, 0.38f, NS); p.Ci(20, 13.1f, 0.55f, NH);
        break;
    case 3:   // upturned
        p.Ci(19.4f, 13.5f, 0.3f, NS); p.Ci(20.6f, 13.5f, 0.3f, NS); p.Ci(20, 12.8f, 0.5f, NH);
        p.L(20.8f, 11.4f, 21.0f, 12.8f, 0.3f, C(shade(sk, 0.78f), 100));
        break;
    default:  // button
        p.Ci(19.3f, 13.8f, 0.32f, NS); p.Ci(20.7f, 13.8f, 0.32f, NS); p.Ci(20, 13.1f, 0.5f, NH);
        p.L(20.9f, 11.2f, 21.1f, 13.0f, 0.3f, C(shade(sk, 0.8f), 100));
        break;
    }
    // lips
    {
        static const float LW[4] = {1.85f, 1.6f, 1.8f, 2.15f}, LU[4] = {0.62f, 0.7f, 0.4f, 0.66f}, LL[4] = {0.85f, 0.72f, 0.5f, 0.92f};
        int ls = f.lipShape % 4;
        float w = LW[ls], uh = LU[ls], lh = LL[ls], ly = 14.9f + (chinY - 17.3f) * 0.4f;
        RGB lc = mix(g.d.lips, sk, 0.3f);
        if (g.guy || g.kid) { lc = mix(shade(sk, 0.82f), RGB{220, 120, 130}, g.kid ? 0.35f : 0.22f); w *= 0.86f; uh *= 0.8f; lh *= 0.85f; }
        p.E(20 - w * 0.42f, ly - uh * 0.45f, w * 1.05f, uh * 1.3f, C(lc));
        p.E(20 + w * 0.42f, ly - uh * 0.45f, w * 1.05f, uh * 1.3f, C(lc));
        p.E(20, ly + lh * 0.5f, w * 1.7f, lh * 1.6f, C(shade(lc, 1.07f)));
        p.L(20 - w, ly - 0.3f, 20, ly + 0.15f, 0.3f, C(shade(lc, 0.6f))); p.L(20, ly + 0.15f, 20 + w, ly - 0.3f, 0.3f, C(shade(lc, 0.6f)));
        p.Ci(20 - w, ly, 0.22f, C(shade(sk, 0.7f))); p.Ci(20 + w, ly, 0.22f, C(shade(sk, 0.7f)));
        if (!g.guy) p.E(20.4f, ly + lh * 0.5f, w * 0.4f, lh * 0.3f, C(255, 255, 255, 80));
    }
    // eyes & brows
    const bool closed = pose == PO_SLEEP || pose == PO_SITSLEEP || std::fmod(t * 0.9f + g.d.skin * 0.37f + g.d.hairCol * 0.21f, 3.7f) < 0.12f;
    const float ey = 10.2f, look = pose == PO_PHONE ? 0.f : std::sin(t * 0.37f + g.d.skin) > 0.85f ? 1.f : 0.f;
    drawEye(p, g, 16.4f, ey, -1, closed, look);
    drawEye(p, g, 23.6f, ey, 1, closed, look);
    drawBrow(p, g, 16.4f, ey, -1);
    drawBrow(p, g, 23.6f, ey, 1);
    // freckles & beauty marks
    if (f.marks & 1) {
        static const float FR[][2] = {{16.2f, 12.6f}, {17.4f, 12.0f}, {15.0f, 12.2f}, {16.6f, 13.4f}, {18.6f, 11.8f},
                                      {23.8f, 12.6f}, {22.6f, 12.0f}, {25.0f, 12.2f}, {23.4f, 13.4f}, {21.4f, 11.8f}, {20, 11.6f}};
        for (const auto& q : FR) p.Ci(q[0], q[1], 0.2f, C(shade(sk, 0.66f), 170));
    }
    if (f.marks & 2) p.Ci(23.0f, 15.0f, 0.3f, C(52, 30, 30));
    // piercings
    const u32 MET = C(GOLD), MET2 = C(236, 236, 244);
    if (f.pierce == 1) { p.Ci(13.1f, 12.7f, 0.45f, MET2); p.Ci(26.9f, 12.7f, 0.45f, MET2); }
    if (f.pierce == 2 || f.pierce == 4) { ring(p, 13.0f, 13.9f, 1.1f, 1.3f, 0.3f, MET); ring(p, 27.0f, 13.9f, 1.1f, 1.3f, 0.3f, MET); }
    if (f.pierce >= 3) p.Ci(18.9f, 13.4f, 0.24f, MET2);
    // glasses
    if (f.glasses) {
        const u32 FR = f.glasses == 2 ? C(BERRY) : f.glasses == 4 ? C(200, 200, 210, 140) : C(40, 30, 34);
        const u32 LENS = C(200, 220, 255, 36);
        for (int k = 0; k < 2; k++) {
            float cx = k ? 23.4f : 16.6f, dir = k ? 1.f : -1.f;
            if (f.glasses == 3) { p.R(cx - 2.4f, 8.2f, 4.8f, 3.2f, LENS); p.L(cx - 2.4f, 8.2f, cx + 2.4f, 8.2f, 0.35f, FR); p.L(cx - 2.4f, 11.4f, cx + 2.4f, 11.4f, 0.35f, FR); p.L(cx - 2.4f, 8.2f, cx - 2.4f, 11.4f, 0.35f, FR); p.L(cx + 2.4f, 8.2f, cx + 2.4f, 11.4f, 0.35f, FR); }
            else {
                p.E(cx, 9.8f, 5.0f, 3.8f, LENS);
                if (f.glasses != 4) ring(p, cx, 9.8f, 2.5f, 1.9f, 0.32f, FR);
                if (f.glasses == 2) p.T(cx + dir * 1.6f, 8.2f, cx + dir * 3.2f, 7.4f, cx + dir * 2.4f, 9.2f, FR);
            }
            p.L(cx + dir * 2.5f, 9.4f, cx + dir * 3.6f, 9.2f, 0.3f, FR);
        }
        p.L(19.0f, 9.4f, 21.0f, 9.4f, 0.3f, FR);
        p.L(15.0f, 8.8f, 16.0f, 8.4f, 0.25f, C(255, 255, 255, 140));
    }
}

// ---------------------------------------------------------------- outfit accessories
static void glamAccessories(const Pen& p, const Pen& hp, int outfit, float waistY) {
    if (outfit < FIRST_AESTHETIC || outfit >= NUM_OUTFITS) return;
    const OutfitDef& o = OUTFITS[outfit];
    const uint16_t a = o.acc;
    const u32 AC = C(o.accCol), ACD = C(shade(o.accCol, 0.7f));
    if (a & ACC_FLANNEL) {
        p.R(15.4f, waistY - 1.4f, 9.2f, 3.0f, AC);
        p.L(15.6f, waistY - 0.4f, 24.4f, waistY - 0.4f, 0.5f, C(BLACK, 110));
        p.Q(15.2f, waistY + 1, 17.6f, waistY + 1, 16.6f, waistY + 13, 13.6f, waistY + 12, AC);
        p.Q(22.4f, waistY + 1, 24.8f, waistY + 1, 26.4f, waistY + 12, 23.4f, waistY + 13, ACD);
    }
    if (a & ACC_CHOKER) { p.R(17.8f, 21.6f, 4.4f, 1.1f, C(BLACK)); p.Ci(20, 23.2f, 0.6f, C(HOT)); }
    if (a & ACC_PEARLS) for (int i = 0; i < 9; i++) { float u = i / 8.f - 0.5f; p.Ci(20 + u * 8.f, 26.4f + (0.25f - u * u) * 7.f, 0.55f, C(255, 252, 240)); }
    if (a & ACC_STREAKS) { hp.L(15.2f, 0.4f, 13.2f, 22, 1.0f, AC); hp.L(25.4f, 0.6f, 27.2f, 20, 0.9f, C(80, 200, 255)); }
    if (a & ACC_BEANIE) {
        hp.E(20, 0.6f, 16.8f, 9.2f, AC);
        hp.R(11.8f, 2.0f, 16.4f, 2.4f, ACD);
        for (int i = 0; i < 6; i++) hp.L(13.4f + i * 2.6f, -2.6f, 13.6f + i * 2.6f, 1.8f, 0.3f, ACD);
    }
    if (a & ACC_TIARA) {
        hp.L(14.4f, -0.4f, 25.6f, -0.4f, 0.7f, C(230, 230, 245));
        hp.T(18.6f, -0.4f, 21.4f, -0.4f, 20, -3.4f, C(230, 230, 245));
        hp.Ci(20, -1.4f, 0.6f, C(HOT));
        hp.Sp(16.2f, -0.8f, 1.2f, C(255, 255, 255, 230)); hp.Sp(23.8f, -0.8f, 1.2f, C(255, 255, 255, 230));
    }
    if (a & ACC_CLIPS) for (int i = 0; i < 2; i++) {
        float bx = i ? 25.4f : 14.8f, by = 1.8f + i * 0.8f;
        hp.T(bx, by, bx - 1.6f, by - 1.2f, bx - 1.5f, by + 0.9f, AC); hp.T(bx, by, bx + 1.6f, by - 1.2f, bx + 1.5f, by + 0.9f, AC);
        hp.Ci(bx, by, 0.35f, ACD);
    }
    if (a & ACC_BOW) {
        float bx = 25.4f, by = -0.6f, s = 2.6f;
        hp.T(bx, by, bx - s, by - s * 0.7f, bx - s, by + s * 0.7f, AC); hp.T(bx, by, bx + s, by - s * 0.7f, bx + s, by + s * 0.7f, AC);
        hp.L(bx - 0.3f, by, bx - 1.2f, by + s * 1.3f, 0.6f, AC); hp.L(bx + 0.3f, by, bx + 1.2f, by + s * 1.3f, 0.6f, AC);
        hp.Ci(bx, by, s * 0.3f, ACD);
    }
    if (a & ACC_HOOPS) { ring(hp, 13.0f, 14.4f, 1.6f, 1.9f, 0.35f, C(GOLD)); ring(hp, 27.0f, 14.4f, 1.6f, 1.9f, 0.35f, C(GOLD)); }
    if (a & ACC_SHADES) {
        const u32 SH = C(30, 22, 30, 236);
        hp.E(16.6f, 9.8f, 5.6f, 3.6f, SH); hp.E(23.4f, 9.8f, 5.6f, 3.6f, SH);
        hp.L(19.2f, 9.2f, 20.8f, 9.2f, 0.45f, SH);
        hp.L(14.8f, 9.2f, 16.0f, 8.6f, 0.35f, C(255, 255, 255, 160)); hp.L(21.6f, 9.2f, 22.8f, 8.6f, 0.35f, C(255, 255, 255, 160));
    }
}

// ---------------------------------------------------------------- body
// The figure is designed in a 40 x 120 box (feet at the bottom); local y 62 is the
// seat line, so a sitting sim's "footY" is the hip.
static void glamDraw(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip, bool bust, float bustCx, float bustCy) {
    GlamCtx g{d, d.f, GSKIN[d.skin % NUM_SKINS], GHAIR[d.hairCol % NUM_HAIRCOLS], 0, 0, 0, 0, 0, 0, d.age == KID, d.guy, 18.f};
    const RGB skin = g.skin;
    g.SK = C(skin); g.SKD = C(shade(skin, 0.86f)); g.SKS = C(shade(skin, 0.74f), 150);
    g.HR = C(g.hair); g.HRD = C(shade(g.hair, 0.78f)); g.HRL = C(shade(g.hair, 1.45f), 160);
    const u32 SK = g.SK, SKD = g.SKD;

    const bool sitSleep = pose == PO_SITSLEEP;
    const bool sitting = pose == PO_SIT || sitSleep;
    const bool walking = pose == PO_WALK;
    if (pose == PO_LIE) pose = PO_SLEEP;
    const float ph = g_walkPhase * 6.2832f;
    const float wdx = walking ? std::sin(ph) * 3.0f : 0.f;
    float bob = pose == PO_DANCE ? -std::fabs(std::sin(t * 7.f)) * 3.5f : walking ? -std::fabs(std::sin(ph)) * 1.2f : 0.f;

    Pen p(x, (sitting ? footY + 58 * s : footY) + bob, 40, 120, s, flip);
    if (bust) { p = Pen(bustCx, 0, 40, 120, s, false); p.oy = bustCy - 18 * s; }
    // head pen: kids get a bigger head on the same neck point
    const float hk = d.age == KID ? 1.95f : d.age == TEEN ? 1.68f : 1.62f;   // big expressive 'dollz' heads
    // Shorter, natural legs: everything above the hips sits lower while the feet stay planted.
    const float U = (!bust && !sitting) ? 10.f : 0.f;
    Pen pl = p;
    p.oy += U * p.s;
    Pen hp = p;
    hp.s = p.s * hk;
    hp.ox = p.X(20) - 20 * hp.s;
    hp.oy = p.Y(21.0f) - 19.5f * hp.s;
    const float breathe = (bust || sitting) ? 0.f : std::sin(t * 1.6f + d.skin) * 0.25f;
    hp.oy += breathe * s;

    // outfit colours
    RGB top = d.top, bot = d.bottom, shoes = d.shoes;
    Bottom bt = d.bt;
    int pat = d.topPattern;
    if (outfit > 0) { const OutfitDef& o = OUTFITS[outfit]; top = o.top; bot = o.bottom; shoes = o.shoes; bt = o.bt; pat = o.topPattern; }
    if (d.guy && (bt == BT_MINI || bt == BT_GOWN)) bt = BT_PANTS;
    const u32 TP = C(top), TPD = C(shade(top, 0.82f), 200), BT = C(bot), BTD = C(shade(bot, 0.8f));
    const bool girl = !d.guy;

    // body measurements (half-widths)
    float sL = girl ? 12.8f : 11.4f, sR = 40 - sL;
    float bw = girl ? 6.0f : 7.4f, ww = girl ? 4.4f : 5.8f, hw = girl ? 7.2f : 6.4f;
    if (g.kid) { bw = 5.0f; ww = 4.6f; hw = 5.4f; sL = 13.4f; sR = 26.6f; }

    // ---- back hair ----
    g_cutY = bust ? 33.f : 1e9f;
    hairBack(hp, g, t);
    g_cutY = 1e9f;

    // ---- legs, shoes, bottoms ----
    if (!bust) {
        const float footTop = sitting ? 89.f : 112.5f;
        const float H = 55 + U, K = H + (footTop - H) * 0.52f, CY = K + (footTop - K) * 0.32f;
        const u32 LEG = bt == BT_PANTS ? BT : SK, LEGD = bt == BT_PANTS ? BTD : SKD;
        const float cp = (!walking && !sitting && girl && !g.kid) ? 1.0f : 0.f;   // contrapposto
        for (int k = 0; k < 2; k++) {
            float m = k ? -1.f : 1.f;   // mirror about x = 20
            auto X = [&](float lx) { return k ? 40 - lx : lx; };
            float sh = (k ? -wdx : wdx);
            float kneeX = sh * 0.5f + (k ? cp * 0.5f : 0.f), ankX = sh + (k ? cp : 0.f);
            u32 c = k ? LEG : LEGD;
            if (sitting) {
                pl.Q(X(20 - hw), 55, X(19.8f), 55, X(19.4f), 67, X(14.0f), 67, c);
                pl.E(X(16.7f), 67, 5.6f, 3.4f, c);
                pl.Q(X(14.6f), 67, X(18.8f), 67, X(18.0f), footTop, X(16.0f), footTop, c);
            } else {
                pl.Q(X(20 - hw), H, X(19.9f), H, X(19.3f) + (k ? -kneeX : kneeX), K, X(14.6f) + (k ? -kneeX : kneeX), K, c);
                pl.E(X(17.0f) + (k ? -kneeX : kneeX), K, 4.0f, 2.4f, c);
                pl.Q(X(14.6f) + (k ? -kneeX : kneeX), K, X(19.3f) + (k ? -kneeX : kneeX), K, X(18.5f) + (k ? -ankX : ankX), footTop, X(15.6f) + (k ? -ankX : ankX), footTop, c);
                if (bt != BT_PANTS) pl.E(X(16.6f) + (k ? -(kneeX + ankX) / 2 : (kneeX + ankX) / 2), CY, 4.6f, (footTop - K) * 0.46f, c);   // calf
                if (bt != BT_PANTS && k == 0) pl.L(X(17.6f) + kneeX, H + 5, X(17.4f) + kneeX, K - 3, 0.35f, C(255, 255, 255, 40));
            }
            // fluffy leg warmers (Y2K aesthetics)
            if (outfit >= FIRST_AESTHETIC && outfit < NUM_OUTFITS && (OUTFITS[outfit].acc & ACC_LEGWARM)) {
                const RGB wc = OUTFITS[outfit].accCol;
                float off = sitting ? 0.f : (k ? -ankX : ankX);
                float yA = sitting ? 74.f : K + (footTop - K) * 0.3f;
                pl.Q(X(14.6f) + off, yA, X(19.6f) + off, yA, X(19.8f) + off, footTop + 0.6f, X(14.2f) + off, footTop + 0.6f, C(wc));
                for (int rr = 0; rr < 4; rr++) { float yy = yA + (footTop - yA) * (rr + 0.5f) / 4; pl.L(X(14.6f) + off, yy, X(19.6f) + off, yy, 0.4f, C(shade(wc, 0.82f))); }
            }
            // shoes
            float fx = (sitting ? X(17.0f) : X(16.95f) + (k ? -ankX : ankX));
            float fy = footTop;
            const u32 SHC = C(shoes), SOLE = C(shade(shoes, 0.6f));
            if (girl && !g.kid) {   // chunky Y2K platforms
                pl.R(fx - 2.0f, fy - 0.4f, 4.0f, 3.6f, SHC);
                pl.E(fx, fy + 3.0f, 4.6f, 2.4f, SHC);
                pl.R(fx - 2.4f, fy + 3.4f, 4.8f, 3.2f, SOLE);
                pl.R(fx - 2.4f, fy + 4.6f, 4.8f, 0.8f, C(255, 255, 255, 170));
                pl.L(fx - 1.0f, fy + 0.4f, fx - 0.8f, fy + 2.2f, 0.35f, C(255, 255, 255, 150));
            } else {   // sneakers / flats
                pl.E(fx, fy + 3.4f, 4.6f, 4.8f, SHC);
                pl.E(fx, fy + 5.4f, 5.0f, 2.2f, SHC);
                pl.R(fx - 2.5f, fy + 5.4f, 5.0f, 1.4f, C(WHITE));
                pl.L(fx - 1.2f, fy + 1.8f, fx + 1.2f, fy + 1.8f, 0.3f, C(255, 255, 255, 170));
            }
            (void)m;
        }
        // bottoms
        switch (bt) {
        case BT_PANTS:
            p.Q(20 - ww, 45.5f, 20 + ww, 45.5f, 20 + hw, 56, 20 - hw, 56, BT);
            p.L(20, 52, 20, 58, 0.35f, BTD);
            p.L(20 + ww - 1, 47, 20 + hw - 1.6f, 55, 0.3f, BTD);
            break;
        case BT_MINI:
            p.Q(20 - ww, 45.5f, 20 + ww, 45.5f, 20 + hw + 2.6f, sitting ? 60 : 64, 20 - hw - 2.6f, sitting ? 60 : 64, BT);
            for (int i = 0; i < 4; i++) { float u = (i + 0.5f) / 4.f - 0.5f; p.L(20 + u * ww * 2, 46.5f, 20 + u * (hw + 2.6f) * 2, sitting ? 59.6f : 63.6f, 0.3f, BTD); }
            break;
        case BT_SHORTS:
            p.Q(20 - ww, 45.5f, 20 + ww, 45.5f, 20 + hw + 0.8f, 61, 20 - hw - 0.8f, 61, BT);
            p.L(20, 53, 20, 61, 0.4f, BTD);
            break;
        case BT_GOWN: {
            float hem = (sitting ? 92.f : 118.f - U);
            p.Q(20 - ww, 45.5f, 20 + ww, 45.5f, 20 + hw + 8, hem, 20 - hw - 8, hem, BT);
            p.Q(20 + 0.5f, 52, 20 + ww, 46, 20 + hw + 8, hem, 20 + hw + 2, hem, BTD);
            p.L(19, 54, 16, hem, 0.3f, C(255, 255, 255, 50));
            for (int i = 0; i < 4; i++) p.Sp(14 + i * 4.f, 68 + (i * 13 % 34), 1.2f, C(255, 255, 255, (uint8_t)(110 + 100 * std::sin(t * 3 + i))));
            break;
        }
        }
    }

    // ---- neck & torso ----
    p.R(17.5f, 16, 5.0f, 11, SK);
    p.R(21.6f, 16, 0.9f, 11, C(shade(g.skin, 0.9f)));
    const float tb = bust ? 34.f : 45.5f;   // where the top ends
    p.Q(sL, 25.5f, sR, 25.5f, 20 + bw, 35, 20 - bw, 35, TP);
    p.Ci(sL + 1.3f, 27.0f, 2.0f, TP); p.Ci(sR - 1.3f, 27.0f, 2.0f, TP);
    if (!bust) {
        p.Q(20 - bw, 35, 20 + bw, 35, 20 + ww, tb, 20 - ww, tb, TP);
        p.Q(20 + bw - 1.4f, 35, 20 + bw, 35, 20 + ww, tb, 20 + ww - 1.2f, tb, TPD);   // side shading
        topPatternOn(p, 28, tb - 0.5f, sL + 1.5f, sR - 1.5f, 20 - ww, 20 + ww, pat, (float)d.skin);
        if (girl && !g.kid) { p.L(16.0f, 34.6f, 19.2f, 35.4f, 0.3f, TPD); p.L(20.8f, 35.4f, 24.0f, 34.6f, 0.3f, TPD); }
        if (bt != BT_GOWN && girl) p.R(20 - ww, tb - 1.6f, ww * 2, 1.6f, C(GOLD));
        if (bt == BT_GOWN) p.Q(20 - bw, 35, 20 + bw, 35, 20 + ww, tb, 20 - ww, tb, BT);
    } else topPatternOn(p, 28, 34, sL + 1.5f, sR - 1.5f, 20 - bw, 20 + bw, pat, (float)d.skin);
    // neckline
    if (girl) { p.E(20, 25.4f, 6.6f, 4.6f, SK); p.L(16.4f, 25.4f, 18.6f, 26.0f, 0.25f, SKD); p.L(21.4f, 26.0f, 23.6f, 25.4f, 0.25f, SKD); }
    else { p.E(20, 25.4f, 4.6f, 2.0f, SK); p.L(17.6f, 26.2f, 22.4f, 26.2f, 0.35f, TPD); }

    // ---- arms ----
    if (!bust) {
        const float ua = girl ? 3.4f : 4.0f, fa = girl ? 3.0f : 3.6f;
        if (g.kid) { }
        auto arm = [&](float x0, float y0, float x1, float y1, float x2, float y2, bool back) {
            u32 c = back ? SKD : SK;
            p.L(x0, y0, x1, y1, ua, c);
            p.L(x1, y1, x2, y2, fa, c);
            p.Ci(x1, y1, fa * 0.5f, c);
            p.E(x2 + (x2 - x1) * 0.08f, y2 + 1.4f, 2.1f, 3.2f, c);
            p.L(x0, y0, x0 + (x1 - x0) * 0.32f, y0 + (y1 - y0) * 0.32f, ua + 0.7f, TP);   // short sleeve
        };
        const float lsx = sL + 1.0f, rsx = sR - 1.0f, sy = 27.0f;
        const float wave = std::sin(t * 9.f) * 3.f;
        // left arm (screen-left when not flipped)
        if (pose == PO_DANCE) arm(lsx, sy, 9.5f, 20 + wave * 0.3f, 10.4f, 10.5f, true);
        else if (sitting) arm(lsx, sy, 12.8f, 42, 16.6f, 55, true);
        else if (pose == PO_PHONE) arm(lsx, sy, 13.0f, 40, 18.0f, 36.5f, true);
        else arm(lsx, sy, 11.8f - wdx * 0.3f, 43, 12.4f - wdx * 0.7f, 57, true);
        // phone in both hands
        if (pose == PO_PHONE) { p.R(18.1f, 31.6f, 3.8f, 5.8f, C(220, 216, 226)); p.R(18.5f, 32.2f, 3.0f, 3.9f, C(255, 140, 200)); p.Ci(20, 33.4f, 0.6f, C(255, 255, 255, 200)); }
        // right arm
        if (pose == PO_PHONE) arm(rsx, sy, 27.0f, 40, 22.0f, 36.5f, false);
        else if (pose == PO_WAVE || pose == PO_DANCE) arm(rsx, sy, 30.6f, 21.0f - wave * 0.3f, 30.8f + (pose == PO_WAVE ? wave * 0.4f : 0), 11.0f, false);
        else if (sitting) arm(rsx, sy, 27.2f, 42, 23.4f, 55, false);
        else if (girl && !g.kid && !walking) arm(rsx, sy, 30.4f, 37.5f, 25.6f, 44.0f, false);   // hand on hip
        else arm(rsx, sy, 28.2f + wdx * 0.3f, 43, 27.6f + wdx * 0.7f, 57, false);
        if (girl && !g.kid) p.R(10.9f - (sitting ? -4.2f : wdx * 0.7f), 53.2f - (sitting ? 0 : 0), 3.0f, 0.9f, C(GOLD));   // bangle
        // handbag on the elbow for grown-up girls standing around
        if (girl && !g.kid && !sitting && pose == PO_IDLE) {
            const u32 BG = C(pat == 2 ? GOLD : HOT), BGD = C(shade(pat == 2 ? GOLD : HOT, 0.75f));
            p.L(11.4f, 42.6f, 8.6f, 47.6f, 0.4f, C(GOLD)); p.L(12.2f, 43.0f, 12.6f, 47.6f, 0.4f, C(GOLD));
            p.Q(7.4f, 47.4f, 13.4f, 47.4f, 14.0f, 53.6f, 6.8f, 53.6f, BG);
            p.R(6.8f, 52.2f, 7.2f, 1.4f, BGD); p.Ci(10.4f, 49.4f, 0.5f, C(GOLD));
        }
    }

    // ---- head ----
    drawFace(hp, g, pose, t);
    hairFront(hp, g, t);
    if (!g_ink && g.d.hairStyle % NUM_HAIRSTYLES != 8 && g.d.hairStyle % NUM_HAIRSTYLES != 9) hairShine(hp, g);
    glamAccessories(p, hp, bust ? 0 : outfit, 45.5f);
    if (bust && outfit >= FIRST_AESTHETIC) glamAccessories(p, hp, outfit, 200.f);

    if (!bust && !g_ink && (pose == PO_SLEEP || sitSleep)) {
        float zt = std::fmod(t, 2.f);
        text("z", hp.X(30) + zt * 4, hp.Y(-6) - zt * 6, 0.4f * s + 0.12f, C(255, 255, 255, (uint8_t)(220 - zt * 100)));
    }
    // a little diva twinkle in the hair now and then
    float spk = std::fmod(t * 0.5f + d.skin * 0.21f + d.hairCol * 0.13f, 3.f);
    if (spk < 0.5f && !g_ink && !bust) hp.Sp(14.5f, 0.5f, 1.8f + spk * 2.5f, C(255, 255, 255, (uint8_t)(220 - spk * 400)));
    if (holiday) { Pen hh = hp; hh.oy -= 2.5f * hh.s; drawHat(hh, holiday, t); }
    if (outfit == 8 && girl) {
        Pen hh = hp; hh.oy -= 1.5f * hh.s;
        hh.T(13, 2, 15.5f, -5, 18, 1, C(BLACK)); hh.T(22, 1, 24.5f, -5, 27, 2, C(BLACK));
        hh.T(14.2f, 1, 15.5f, -3, 16.8f, 1, C(HOT)); hh.T(23.2f, 1, 24.5f, -3, 25.8f, 1.5f, C(HOT));
    }
}

// Every figure gets a crisp dark outline: the silhouette is stamped in ink one
// pixel up/down/left/right, then the coloured figure is drawn on top.
static u32 inkCol() {
    switch (g_artStyle) {
    case 5: return C(196, 104, 156);   // Pastel Pixel: rose outlines
    case 6: return C(66, 38, 26);      // Cozy Pixel: warm brown outlines
    case 4: return C(132, 86, 108);    // HD Glam: soft rosy-brown line
    default: return C(48, 26, 42);     // Pixel Glam: deep plum
    }
}
void drawGlamPerson(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip) {
    static const float O[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    const float o = g_pix ? 2.0f : s < 0.75f ? 0.6f : 0.75f;   // one art-pixel outline
    g_ink = inkCol();
    for (const auto& k : O) glamDraw(d, outfit, x + k[0] * o, footY + k[1] * o, s, t, holiday, pose, flip, false, 0, 0);
    g_ink = 0;
    glamDraw(d, outfit, x, footY, s, t, holiday, pose, flip, false, 0, 0);
}

void drawGlamBust(const PersonDef& d, int outfit, float cx, float cy, float s, float t, bool blink) {
    static const float O[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    const float tt = blink ? t : 0.5f;
    g_ink = inkCol();
    for (const auto& k : O) glamDraw(d, outfit, 0, 0, s, tt, 0, PO_IDLE, false, true, cx + k[0] * (g_pix ? 2.f : 1.2f), cy + k[1] * (g_pix ? 2.f : 1.2f));
    g_ink = 0;
    glamDraw(d, outfit, 0, 0, s, tt, 0, PO_IDLE, false, true, cx, cy);
}
