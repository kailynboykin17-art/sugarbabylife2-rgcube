#include "game.hpp"
// "Flip Phone" art style: chunky mid-2000s mobile-game pixel art. Small chibi people
// (~40 px) built on a cell grid, flat cel shading with 2-3 tone ramps, clean dark
// outlines and a limited ~64 colour palette. Drawn as merged runs of rects (cached).
#include "people.hpp"
#include "draw.hpp"
#include <cmath>
#include <cstring>
#include <vector>

using namespace col;

namespace {

enum : uint8_t {
    E_ = 0, SKIN, SKIN2, HAIR, HAIRHI, HAIRSH, TOP, TOPALT, TOP2, BOT, BOTALT, SHOE, SOLE, WHITEC, IRIS, DARK, LIPS, LIPHI,
    BLUSH, SHADOWC, BROW, GOLDC, ACC, ACC2, HAT, HAT2, LENS, FRAME, STICK1, STICK2, GEM, PHONEC, TEETH, NCOL
};
constexpr int GW = 26, GH = 60;

struct Grid {
    uint8_t c[GH][GW];
    void clear() { memset(c, 0, sizeof c); }
    void set(int x, int y, uint8_t v) { if (x >= 0 && x < GW && y >= 0 && y < GH) c[y][x] = v; }
    uint8_t get(int x, int y) const { return (x >= 0 && x < GW && y >= 0 && y < GH) ? c[y][x] : 0; }
    void rect(int x0, int y0, int x1, int y1, uint8_t v) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) set(x, y, v); }
    void ell(float cx, float cy, float rx, float ry, uint8_t v) {
        for (int y = (int)(cy - ry) - 1; y <= (int)(cy + ry) + 1; y++)
            for (int x = (int)(cx - rx) - 1; x <= (int)(cx + rx) + 1; x++) {
                float dx = (x + 0.5f - cx) / rx, dy = (y + 0.5f - cy) / ry;
                if (dx * dx + dy * dy <= 1.f) set(x, y, v);
            }
    }
    void over(int x0, int y0, int x1, int y1, uint8_t from, uint8_t v) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if (get(x, y) == from) set(x, y, v); }
    void mirror(int x, int y, uint8_t v) { set(x, y, v); set(GW - 1 - x, y, v); }   // symmetric pair about the center
};

// ~64 colour limited palette: everything is snapped to it
const RGB PAL64[64] = {
    {20, 16, 28}, {40, 30, 44}, {70, 52, 62}, {110, 86, 92}, {160, 140, 140}, {210, 200, 196}, {246, 244, 236}, {255, 255, 255},
    {255, 226, 200}, {244, 196, 160}, {222, 164, 122}, {190, 128, 88}, {150, 94, 62}, {112, 68, 46}, {78, 46, 34}, {56, 32, 26},
    {120, 20, 30}, {176, 36, 44}, {224, 60, 56}, {255, 110, 90}, {255, 160, 140}, {255, 200, 190}, {200, 60, 110}, {240, 90, 150},
    {255, 140, 190}, {255, 196, 222}, {150, 40, 120}, {200, 90, 200}, {230, 150, 236}, {110, 60, 170}, {150, 110, 220}, {200, 180, 250},
    {24, 36, 90}, {36, 64, 150}, {60, 110, 210}, {110, 160, 240}, {170, 210, 255}, {20, 90, 110}, {30, 140, 150}, {60, 196, 196},
    {140, 230, 220}, {30, 90, 50}, {50, 140, 70}, {100, 190, 90}, {170, 230, 130}, {90, 100, 40}, {150, 150, 60}, {120, 70, 20},
    {180, 100, 30}, {230, 140, 40}, {255, 180, 60}, {255, 220, 100}, {255, 244, 170}, {200, 160, 60}, {236, 210, 120}, {140, 120, 100},
    {190, 170, 140}, {96, 96, 110}, {140, 140, 160}, {250, 120, 40}, {214, 80, 20}, {255, 90, 60}, {64, 40, 100}, {44, 26, 60}};
RGB snap(RGB c) {
    int best = 0, bd = 1 << 30;
    for (int i = 0; i < 64; i++) {
        int dr = c.r - PAL64[i].r, dg = c.g - PAL64[i].g, db = c.b - PAL64[i].b;
        int d = dr * dr * 3 + dg * dg * 4 + db * db * 2;
        if (d < bd) { bd = d; best = i; }
    }
    return PAL64[best];
}
const RGB OUTLINE = {30, 20, 36};

struct Wear { RGB top, bot, shoes; Bottom bt; uint8_t pat, topStyle, shoeStyle, hat; uint16_t acc; RGB accCol; };
Wear wearOf(const PersonDef& d, int outfit) {
    Wear w{d.top, d.bottom, d.shoes, d.bt, d.topPattern, d.topStyle, d.shoeStyle, d.hat, d.acc, d.accCol};
    if (outfit > 0 && outfit < NUM_OUTFITS) {
        const OutfitDef& o = OUTFITS[outfit];
        w = {o.top, o.bottom, o.shoes, o.bt, o.topPattern, o.topStyle, o.shoeStyle, o.hat, o.acc, o.accCol};
    }
    if (d.guy && (w.bt == BT_MINI || w.bt == BT_GOWN || w.bt == BT_MAXI || w.bt == BT_PLEATED)) w.bt = w.bt == BT_MINI || w.bt == BT_PLEATED ? BT_SHORTS : BT_PANTS;
    if (d.guy && (w.topStyle == TS_DRESS || w.topStyle == TS_TUBE)) w.topStyle = TS_TEE;
    if (d.guy && (w.shoeStyle == SS_HEELS || w.shoeStyle == SS_PLATFORMS)) w.shoeStyle = SS_SNEAKERS;
    if (d.age == KID && (w.shoeStyle == SS_HEELS)) w.shoeStyle = SS_SNEAKERS;
    return w;
}

struct Ctx {
    const PersonDef& d; Wear w; int mood; bool blink; int pose, frame, walk;
    int headCY, torso0, torsoEnd, hip, legEnd, footRow;
};

// ---------------------------------------------------------------- hair
void hairBack(Grid& g, const Ctx& k) {
    const int hs = k.d.hairStyle % NUM_HAIRSTYLES, cy = k.headCY;
    switch (hs) {
    case 0: g.ell(13, cy, 9.0f, 8.3f, HAIR); g.rect(4, cy, 21, cy + 15, HAIR); for (int x = 6; x <= 19; x += 2) g.set(x, cy + 16, HAIR); break;               // sleek long
    case 1: g.ell(13, cy, 9.6f, 8.8f, HAIR); g.rect(3, cy, 22, cy + 13, HAIR); for (int y = cy; y <= cy + 14; y += 2) { g.set(3, y, HAIR); g.set(22, y + 1, HAIR); } break;   // big waves
    case 2: g.ell(13, cy, 8.6f, 8.2f, HAIR); g.rect(19, cy - 4, 21, cy + 10, HAIR); g.set(22, cy + 2, HAIR); g.rect(20, cy + 11, 21, cy + 12, HAIR); g.rect(19, cy - 4, 21, cy - 3, ACC); break;   // ponytail
    case 3: g.ell(13, cy, 9.2f, 8.6f, HAIR); g.rect(4, cy, 21, cy + 6, HAIR); break;                                                  // bob
    case 6: case 18: case 25: g.ell(13, cy, 9.4f, 8.8f, HAIR); for (int y = cy; y <= cy + (hs == 6 ? 9 : hs == 25 ? 7 : 3); y++) { g.set(4 + (y & 1), y, HAIR); g.set(21 - (y & 1), y, HAIR); g.rect(5, y, 20, y, HAIR); } break;
    case 7: g.ell(13, cy, 9.0f, 8.4f, HAIR); g.rect(4, cy, 21, cy + 19, HAIR); for (int x = 6; x <= 19; x += 3) g.rect(x, cy + 2, x, cy + 19, HAIRSH); break;   // box braids
    case 8: g.ell(13, cy - 1, 11.5f, 10.8f, HAIR); break;                                                                                   // afro
    case 9: g.ell(13, cy, 8.4f, 8.0f, HAIR); g.ell(5, cy - 6, 3.6f, 3.6f, HAIR); g.ell(21, cy - 6, 3.6f, 3.6f, HAIR); break;            // puffs
    case 11: g.ell(13, cy, 9.0f, 8.4f, HAIR); for (int x = 5; x <= 20; x += 2) g.rect(x, cy, x, cy + 15 + (x % 4 == 1), HAIR); break;    // locs
    case 12: g.ell(13, cy, 8.6f, 8.2f, HAIR); g.rect(3, cy - 2, 5, cy + 9, HAIR); g.rect(20, cy - 2, 22, cy + 9, HAIR); g.set(4, cy - 3, ACC); g.set(21, cy - 3, ACC); break;   // pigtails
    case 13: g.ell(13, cy, 8.4f, 8.0f, HAIR); g.ell(7, cy - 7, 2.6f, 2.6f, HAIR); g.ell(19, cy - 7, 2.6f, 2.6f, HAIR); break;           // space buns
    case 15: g.ell(13, cy, 9.2f, 8.6f, HAIR); for (int x = 5; x <= 20; x += 2) g.rect(x, cy, x, cy + 6, HAIR); break;                 // twists
    case 21: g.ell(13, cy, 8.8f, 8.4f, HAIR); g.rect(4, cy, 21, cy + 6, HAIR); break;                                                  // emo
    case 22: g.ell(13, cy, 8.2f, 8.0f, HAIR); g.ell(13, cy - 8, 4.4f, 3.6f, HAIR); break;                                               // high puff
    case 26: g.ell(13, cy, 9.0f, 8.4f, HAIR); g.rect(4, cy, 21, cy + 13, HAIR); g.ell(13, cy - 8, 2.6f, 2.2f, HAIR); break;                 // half-up
    case 4: g.ell(13, cy, 8.4f, 8.0f, HAIR); g.ell(13, cy - 8, 3.4f, 2.8f, HAIR); break;                                                 // top bun
    case 23: g.ell(13, cy, 8.2f, 8.0f, HAIR); g.ell(7, cy - 6, 1.8f, 1.8f, HAIR); g.ell(13, cy - 8, 1.8f, 1.8f, HAIR); g.ell(19, cy - 6, 1.8f, 1.8f, HAIR); break;
    case 24: break;                                                                                                                      // bald
    default: g.ell(13, cy, 8.4f, 8.1f, HAIR); break;
    }
}
void hairFront(Grid& g, const Ctx& k) {
    const int hs = k.d.hairStyle % NUM_HAIRSTYLES, cy = k.headCY;
    if (hs == 24) { g.set(10, cy - 5, WHITEC); return; }
    const int capEnd = (hs == 16 || hs == 17 || hs == 14) ? cy - 4 : cy - 3;
    for (int y = 0; y <= capEnd; y++) for (int x = 0; x < GW; x++) if (g.get(x, y) == SKIN) g.set(x, y, HAIR);
    switch (hs) {
    case 0: case 26: g.rect(7, cy - 3, 12, cy - 2, HAIR); g.set(7, cy - 1, HAIR); g.rect(5, cy - 2, 5, cy + 5, HAIR); g.rect(20, cy - 3, 20, cy + 5, HAIR); break;   // side part
    case 1: g.rect(7, cy - 3, 18, cy - 3, HAIR); g.rect(5, cy - 2, 5, cy + 6, HAIR); g.rect(20, cy - 2, 20, cy + 6, HAIR); g.set(8, cy - 2, HAIR); g.set(17, cy - 2, HAIR); break;
    case 3: case 15: g.rect(7, cy - 3, 18, cy - 2, HAIR); g.rect(5, cy - 2, 5, cy + 4, HAIR); g.rect(20, cy - 2, 20, cy + 4, HAIR); break;   // blunt bangs
    case 21: g.rect(6, cy - 3, 14, cy - 2, HAIR); g.rect(6, cy - 1, 11, cy + 1, HAIR); g.rect(6, cy + 2, 8, cy + 3, HAIR); break;          // emo fringe over one eye
    case 6: case 18: case 25: g.rect(7, cy - 3, 18, cy - 3, HAIR); for (int x = 7; x <= 18; x += 2) g.set(x, cy - 2, HAIR); g.set(5, cy - 1, HAIR); g.set(20, cy - 1, HAIR); break;
    case 10: g.rect(8, cy - 3, 16, cy - 3, HAIR); g.rect(14, cy - 2, 17, cy - 2, HAIR); break;                                            // pixie swoop
    case 14: for (int x = 8; x <= 18; x += 2) g.rect(x, 0, x, cy - 4, HAIRSH); break;                                                    // cornrows
    case 16: g.over(4, cy - 3, 21, cy + 1, SKIN, SKIN); for (int y = cy - 4; y <= cy - 1; y++) { if (g.get(6, y) == SKIN) g.set(6, y, HAIRSH); if (g.get(19, y) == SKIN) g.set(19, y, HAIRSH); } break;   // fade
    case 17: break;                                                                                                                      // buzz
    case 19: g.rect(8, cy - 3, 18, cy - 3, HAIR); g.set(10, cy - 5, HAIRHI); g.set(10, cy - 4, HAIRSH); break;                             // side part (short)
    case 20: for (int y = 0; y <= cy - 3; y++) for (int x = 0; x < GW; x++) if (g.get(x, y) == HAIR && (x < 11 || x > 14)) g.set(x, y, HAIRSH); g.rect(11, cy - 11, 14, cy - 5, HAIR); break;   // mohawk
    case 27: for (int i = 0; i < 5; i++) { int x = 7 + i * 3; g.set(x, cy - 9, HAIR); g.rect(x - 1, cy - 8, x + 1, cy - 7, HAIR); } g.rect(7, cy - 3, 18, cy - 3, HAIR); break;   // spiky
    case 5: g.rect(7, cy - 3, 18, cy - 3, HAIR); g.set(16, cy - 2, HAIR); break;
    default: g.rect(7, cy - 3, 18, cy - 3, HAIR); g.rect(5, cy - 2, 5, cy + 2, HAIR); g.rect(20, cy - 2, 20, cy + 2, HAIR); break;
    }
    // shine: a little highlight streak, plus texture lines for curly/braided styles
    g.set(9, cy - 6, HAIRHI); g.set(10, cy - 6, HAIRHI); g.set(8, cy - 5, HAIRHI);
    if (hs == 8 || hs == 9 || hs == 22 || hs == 6) for (int y = 0; y < cy - 3; y += 2) for (int x = (y & 2); x < GW; x += 4) if (g.get(x, y) == HAIR) g.set(x, y, HAIRSH);
}

// ---------------------------------------------------------------- face (eyes ~ rows cy+1..cy+2)
void dollEye(Grid& g, const Ctx& k, int side) {
    // big almond doll eye: 5 cols x 4 rows (cols 6..10, mirrored 15..19), glossy 3-wide iris,
    // heavy upper lashes with an outer flick; shadow & liner are player choices
    const FaceDef& f = k.d.f;
    const int ey = k.headCY + 2;
    auto S = [&](int x, int y, uint8_t v) { g.set(side < 0 ? x : GW - 1 - x, y, v); };
    const bool closed = k.blink || k.mood == 7 || k.pose == PO_SLEEP;
    const int e = f.eyes % 10, sh = f.shadow % 10, ln = f.liner % 10;
    if (sh) { for (int x = 7; x <= 10; x++) S(x, ey - 1, SHADOWC); if (sh >= 4) S(6, ey - 1, SHADOWC); }
    if (closed || (k.mood == 4 && side > 0)) {   // closed / wink: a lashy smile-curve
        S(6, ey + 2, DARK); S(7, ey + 3, DARK); S(8, ey + 3, DARK); S(9, ey + 3, DARK); S(10, ey + 2, DARK); S(5, ey + 2, DARK); S(5, ey + 1, DARK); return;
    }
    const bool heavy = e == 4 || e == 6 || k.mood == 3 || k.mood == 6;
    const bool narrow = e == 5;
    // upper lid + lashes (thick, doll-like)
    for (int x = 7; x <= 10; x++) S(x, ey, DARK);
    S(6, ey + 1, DARK); S(5, ey, DARK); S(5, ey - 1, DARK);        // outer corner flick
    if (e == 2 || e == 7 || ln >= 2) { S(4, ey - 1, DARK); S(4, ey - 2, ln == 3 ? DARK : E_); }   // cat / winged
    // eyeball
    const int r0 = ey + 1, r1 = ey + (narrow ? 2 : 3);
    for (int y = r0; y <= r1; y++) { S(7, y, WHITEC); S(11, y, E_); S(10, y, WHITEC); S(8, y, IRIS); S(9, y, IRIS); }
    if (!narrow) { S(7, r0, IRIS); }                               // iris leans to the outer corner a touch
    S(8, r0 + 1 <= r1 ? r0 + 1 : r0, DARK);                       // pupil
    S(9, r0, WHITEC);                                              // catchlight
    if (e == 8) S(7, r1, WHITEC);                                  // extra sparkle
    if (heavy) { S(7, r0, SKIN2); S(8, r0, DARK); S(9, r0, DARK); S(10, r0, SKIN2); }
    // lower lash line
    S(8, r1 + 1, ln == 5 ? DARK : SKIN2); S(9, r1 + 1, ln == 5 ? DARK : SKIN2);
    if (e == 3 || e == 1) S(10, r1 + 1, SKIN2);
    if (k.mood == 9) { S(8, r0 + 1, WHITEC); S(10, r0 + 1, IRIS); }   // embarrassed glance
    if (k.mood == 5) S(8, r0 + 1, WHITEC);                           // surprised: tiny pupils
}
void eye(Grid& g, const Ctx& k, int side) {
    if (!k.d.guy && k.d.age != KID) { dollEye(g, k, side); return; }
    const FaceDef& f = k.d.f;
    const int cy = k.headCY, ey = cy + 1;
    // left eye occupies cols 8..10, right eye mirrored (15..17); "o" = outer col
    auto S = [&](int x, int y, uint8_t v) { g.set(side < 0 ? x : GW - 1 - x, y, v); };
    const int x0 = 8, xo = 8, xi = 10;   // outer = 8 (left eye)
    const bool closed = k.blink || k.mood == 7 || k.pose == PO_SLEEP;
    const bool girl = !k.d.guy;
    // eyeshadow above the lid
    if (f.shadow % 10 && girl) { S(x0, ey - 1, SHADOWC); S(x0 + 1, ey - 1, SHADOWC); S(x0 + 2, ey - 1, SHADOWC); }
    if (closed) { S(x0, ey + 1, DARK); S(x0 + 1, ey + 1, DARK); S(x0 + 2, ey + 1, DARK); if (girl) S(xo - 1, ey + 1, DARK); return; }
    if (k.mood == 1) { S(x0, ey + 1, DARK); S(x0 + 1, ey, DARK); S(x0 + 2, ey + 1, DARK); return; }   // ^ ^ excited
    if (k.mood == 4 && side > 0) { S(x0, ey + 1, DARK); S(x0 + 1, ey + 1, DARK); S(x0 + 2, ey + 1, DARK); return; }   // wink
    int e = f.eyes % 10;
    // lid line
    int lidY = ey;
    auto lid = [&](int w) { for (int i = 0; i < w; i++) S(x0 + i - (w > 3 ? 1 : 0), lidY, DARK); };
    switch (e) {
    case 0: lid(3); S(x0, ey + 1, IRIS); S(x0 + 1, ey + 1, IRIS); S(x0 + 1, ey + 1, IRIS); S(x0 + 2, ey + 1, IRIS); S(x0 + 1, ey + 1, WHITEC); break;   // almond
    case 1: lid(3); S(x0, ey + 1, IRIS); S(x0 + 1, ey + 1, WHITEC); S(x0 + 2, ey + 1, IRIS); S(x0, ey + 2, IRIS); S(x0 + 1, ey + 2, IRIS); S(x0 + 2, ey + 2, IRIS); break;   // round
    case 2: lid(3); S(xo - 1, ey - 1, DARK); S(x0, ey + 1, IRIS); S(x0 + 1, ey + 1, IRIS); S(x0 + 2, ey + 1, WHITEC); break;   // cat
    case 3: lid(3); S(x0, ey + 1, IRIS); S(x0 + 1, ey + 1, WHITEC); S(x0 + 2, ey + 1, IRIS); S(x0, ey + 2, IRIS); S(x0 + 1, ey + 2, IRIS); S(x0 + 2, ey + 2, IRIS); S(xo - 1, ey + 2, WHITEC); break;   // doe
    case 4: S(x0, ey - 1, SKIN2); S(x0 + 1, ey - 1, SKIN2); S(x0 + 2, ey - 1, SKIN2); lid(3); S(x0 + 1, ey + 1, IRIS); S(x0 + 2, ey + 1, IRIS); break;   // hooded
    case 5: lid(3); S(x0 + 1, ey + 1, IRIS); break;                                                                                                   // monolid
    case 6: lid(3); S(x0, ey + 1, DARK); S(x0 + 1, ey + 1, IRIS); S(x0 + 2, ey + 1, IRIS); break;                                                     // sleepy
    case 7: S(x0, ey, DARK); S(x0 + 1, ey, DARK); S(x0 + 2, ey, DARK); S(xo - 1, ey - 1, DARK); S(x0, ey + 1, IRIS); S(x0 + 1, ey + 1, IRIS); S(x0 + 2, ey + 1, WHITEC); break;   // upturned
    case 8: lid(3); S(x0, ey + 1, WHITEC); S(x0 + 1, ey + 1, IRIS); S(x0 + 2, ey + 1, IRIS); S(x0, ey + 2, IRIS); S(x0 + 1, ey + 2, IRIS); S(x0 + 2, ey + 2, WHITEC); break;   // sparkle
    default: lid(4); S(xo - 1, ey + 1, WHITEC); S(x0, ey + 1, IRIS); S(x0 + 1, ey + 1, IRIS); S(x0 + 2, ey + 1, WHITEC); break;   // wide
    }
    if (k.mood == 2) S(xi, ey - 1, BROW);   // sad droop
    // eyeliner
    if (girl) switch (f.liner % 10) {
    case 2: S(xo - 1, ey - 1, DARK); break;
    case 3: S(xo - 1, ey, DARK); S(xo - 2, ey - 1, DARK); break;
    case 4: S(x0, ey + 2, SHADOWC); S(x0 + 1, ey + 2, SHADOWC); break;
    case 5: S(x0, ey + 2, DARK); S(x0 + 1, ey + 2, DARK); break;
    case 6: S(xo - 1, ey - 1, DARK); S(x0, ey - 2, DARK); break;
    case 7: S(x0, ey - 1, WHITEC); S(x0 + 2, ey - 1, GOLDC); break;
    case 8: S(x0, ey, STICK1); S(x0 + 1, ey, STICK1); S(x0 + 2, ey, STICK1); break;
    case 9: S(xo - 1, ey - 1, DARK); S(xo - 1, ey + 2, DARK); break;
    default: break;
    }
}
void brows(Grid& g, const Ctx& k, int side) {
    auto S = [&](int x, int y, uint8_t v) { g.set(side < 0 ? x : GW - 1 - x, y, v); };
    const bool doll = !k.d.guy && k.d.age != KID;
    const int by = doll ? k.headCY : k.headCY - 1;
    int b = k.d.f.brows % 10;
    int lift = (k.mood == 5 || k.mood == 1) ? -1 : 0;
    if (k.mood == 3) { S(8, by + lift, BROW); S(9, by + lift, BROW); S(10, by + 1, BROW); return; }   // annoyed
    if (k.mood == 2 || k.mood == 9) { S(8, by + 1, BROW); S(9, by, BROW); S(10, by - 1, BROW); return; }
    int y = by + lift;
    switch (b) {
    case 0: S(8, y, BROW); S(9, y - 1, BROW); S(10, y, BROW); break;
    case 1: S(8, y, BROW); S(9, y, BROW); S(10, y, BROW); break;
    case 2: S(8, y, BROW); S(9, y, BROW); S(10, y, BROW); S(9, y + 1 - 1, BROW); S(10, y - 1, BROW); break;
    case 3: S(8, y - 1, BROW); S(9, y - 2, BROW); S(10, y - 1, BROW); break;
    case 4: S(7, y, BROW); S(8, y - 1, BROW); S(9, y - 1, BROW); S(10, y - 1, BROW); S(8, y, BROW); S(9, y, BROW); S(10, y, BROW); break;
    case 5: S(9, y, BROW); S(10, y, BROW); break;
    case 6: S(8, y, BROW); S(10, y, BROW); S(9, y - 1, BROW); break;
    case 7: S(8, y - 1, BROW); S(9, y, BROW); S(10, y + 1, BROW); break;
    case 8: S(8, y, BROW); S(10, y, BROW); break;
    default: S(7, y + 1, BROW); S(8, y, BROW); S(9, y, BROW); S(10, y + 1, BROW); break;
    }
}
void face(Grid& g, const Ctx& k) {
    const FaceDef& f = k.d.f;
    const int cy = k.headCY;
    const bool girl = !k.d.guy;
    for (int s = -1; s <= 1; s += 2) { eye(g, k, s); brows(g, k, s); }
    // blush (opt-in)
    if (f.blush % 10) { g.mirror(7, cy + 3, BLUSH); g.mirror(8, cy + 3, BLUSH); }
    // nose
    const bool dollF = girl && k.d.age != KID;
    const int ny = cy + (dollF ? 5 : 3);
    switch (f.nose % 10) {
    case 0: g.set(12, ny, SKIN2); break;
    case 1: g.set(12, ny - 1, SKIN2); g.set(12, ny, SKIN2); break;
    case 2: g.set(12, ny, SKIN2); g.set(13, ny, SKIN2); break;
    case 3: g.set(13, ny - 1, SKIN2); g.set(12, ny, SKIN2); break;
    case 4: break;
    case 5: g.set(13, ny - 2, SKIN2); g.set(13, ny - 1, SKIN2); g.set(12, ny, SKIN2); break;
    case 6: g.set(12, ny, SKIN2); g.set(11, ny, SKIN2); break;
    case 7: g.set(13, ny - 1, SKIN2); g.set(14, ny, SKIN2); break;
    case 8: g.set(11, ny, SKIN2); g.set(12, ny, SKIN2); g.set(13, ny, SKIN2); break;
    default: g.set(12, ny - 2, SKIN2); g.set(12, ny - 1, SKIN2); g.set(12, ny, SKIN2); break;
    }
    // mouth
    const int my = cy + (dollF ? 7 : 5);
    const uint8_t L = LIPS;
    switch (k.mood) {
    case 2: g.set(11, my + 1, DARK); g.set(12, my, DARK); g.set(13, my, DARK); g.set(14, my + 1, DARK); break;   // frown
    case 5: g.set(12, my, DARK); g.set(13, my, DARK); g.set(12, my + 1, DARK); g.set(13, my + 1, DARK); break;  // O
    case 3: case 6: g.set(11, my, DARK); g.set(12, my, DARK); g.set(13, my, DARK); break;                      // flat
    case 7: g.set(12, my, DARK); break;
    default:
        switch (f.lipShape % 10) {
        case 0: g.rect(11, my, 14, my, L); g.rect(12, my + 1, 13, my + 1, L); break;
        case 1: g.set(11, my, L); g.set(14, my, L); g.rect(11, my + 1, 14, my + 1, L); g.rect(12, my + 2, 13, my + 2, L); break;
        case 2: g.rect(11, my, 14, my, L); break;
        case 3: g.rect(12, my, 13, my + 1, L); break;
        case 4: g.rect(10, my, 15, my, DARK); g.rect(11, my + 1, 14, my + 1, DARK); g.rect(11, my, 14, my, TEETH); break;
        case 5: g.set(12, my, L); g.set(13, my, L); break;
        case 6: g.set(11, my, L); g.set(14, my, L); g.rect(11, my + 1, 14, my + 1, L); break;
        case 7: g.rect(11, my, 14, my, L); g.rect(11, my + 1, 14, my + 1, L); g.set(12, my, LIPHI); break;
        case 8: g.rect(11, my + 1, 13, my + 1, L); g.set(14, my, L); break;
        default: g.set(12, my, L); g.set(13, my, L); g.set(11, my + 1, L); g.set(14, my + 1, L); g.set(12, my + 2, L); g.set(13, my + 2, L); break;
        }
        if (!girl || k.d.age == KID) for (int y = my; y <= my + 2; y++) for (int x = 10; x <= 15; x++) if (g.get(x, y) == LIPS) g.set(x, y, k.mood == 0 && x > 10 && x < 15 && y == my + 1 ? DARK : SKIN2);
        if (k.mood == 1 || k.mood == 8) { g.set(10, my - 1, girl ? L : SKIN2); g.set(15, my - 1, girl ? L : SKIN2); }
        if (k.mood == 4) g.set(15, my - 1, DARK);
        break;
    }
    if (girl && k.d.age != KID && k.mood != 2 && k.mood != 5 && k.mood != 3 && k.mood != 6 && k.mood != 7) {   // glossy full doll lips
        for (int x = 10; x <= 15; x++) for (int y = my; y <= my + 2; y++) if (g.get(x, y) == LIPS || g.get(x, y) == LIPHI || g.get(x, y) == DARK || g.get(x, y) == TEETH) g.set(x, y, SKIN);
        const int ls = f.lipShape % 10;
        g.set(12, my, L); g.set(13, my, L);                          // upper lip
        if (ls == 0 || ls == 3 || ls == 7 || ls == 9) { g.set(11, my, L); g.set(14, my, L); }
        g.rect(11, my + 1, 14, my + 1, L);                          // full lower lip
        if (ls == 3 || ls == 9) { g.set(10, my + 1, L); g.set(15, my + 1, L); }
        g.set(12, my + 1, LIPHI);                                   // a single glossy highlight
        if (k.mood == 1 || k.mood == 8) { g.set(10, my, L); g.set(15, my, L); }   // big smile corners
        if (ls == 4) g.rect(11, my, 14, my, TEETH);
    }
    // face art
    switch (f.marks % 10) {
    case 1: case 3: g.set(7, cy + 7, SKIN2); g.set(9, cy + 6, SKIN2); g.set(16, cy + 6, SKIN2); g.set(18, cy + 7, SKIN2); if ((f.marks % 10) == 1) break; // fallthrough
    case 2: g.set(16, my, DARK); break;
    case 4: g.set(17, cy + 3, STICK2); g.set(18, cy + 3, STICK2); g.set(17, cy + 4, STICK2); break;
    case 5: g.set(17, cy + 4, GOLDC); g.set(16, cy + 4, GOLDC); g.set(17, cy + 3, GOLDC); break;
    case 6: g.set(7, cy + 4, WHITEC); g.set(18, cy + 3, WHITEC); g.set(17, cy + 5, WHITEC); break;
    case 7: g.set(12, cy - 2, GEM); g.set(13, cy - 2, GEM); break;
    case 8: g.set(19, cy, STICK1); g.set(20, cy - 1, STICK1); g.set(20, cy + 1, STICK1); break;
    case 9: g.mirror(8, cy + 3, WHITEC); g.mirror(9, cy + 4, WHITEC); break;
    default: break;
    }
    // facial hair
    if (k.d.guy) switch (k.d.f.beard % 10) {
    case 1: for (int x = 9; x <= 16; x += 2) g.set(x, my + 2, SKIN2); g.set(10, my + 1, SKIN2); g.set(15, my + 1, SKIN2); break;
    case 2: g.rect(11, my + 2, 14, my + 3, HAIR); g.rect(11, my - 1, 14, my - 1, HAIR); break;
    case 3: for (int y = cy + 2; y <= my + 3; y++) { g.set(6, y, HAIR); g.set(19, y, HAIR); } g.rect(7, my + 1, 18, my + 3, HAIR); g.rect(10, my - 1, 15, my - 1, HAIR); break;
    case 4: g.rect(10, my - 1, 15, my - 1, HAIR); break;
    case 5: for (int y = cy + 2; y <= my + 3; y++) { g.set(6, y, HAIR); g.set(19, y, HAIR); } g.rect(7, my + 3, 18, my + 3, HAIR); break;
    case 6: g.set(12, my + 2, HAIR); g.set(13, my + 2, HAIR); break;
    case 7: g.rect(9, my + 1, 16, my + 3, HAIR); g.rect(10, my - 1, 15, my - 1, HAIR); break;
    case 8: for (int y = cy + 1; y <= my + 2; y++) { g.set(6, y, HAIR); g.set(7, y, HAIR); g.set(18, y, HAIR); g.set(19, y, HAIR); } break;
    case 9: g.rect(10, my - 1, 15, my - 1, HAIR); g.set(9, my, HAIR); g.set(16, my, HAIR); g.set(9, my + 1, HAIR); g.set(16, my + 1, HAIR); break;
    default: break;
    }
    // glasses
    const int gy = cy + 1;
    switch (f.glasses % 10) {
    case 0: break;
    case 5: case 6: for (int s = 0; s < 2; s++) { int x = s ? 15 : 8; g.rect(x, gy, x + 2, gy + 1, LENS); g.set(x + 1, gy + 2, LENS); } g.rect(11, gy, 14, gy, FRAME); break;
    case 7: g.rect(7, gy, 18, gy, GOLDC); for (int s = 0; s < 2; s++) { int x = s ? 15 : 8; g.rect(x, gy + 1, x + 2, gy + 2, LENS); } break;
    default: {
        uint8_t fr = (f.glasses % 10) == 4 ? WHITEC : FRAME;
        for (int s = 0; s < 2; s++) {
            int x = s ? 14 : 7;
            g.rect(x, gy - 1, x + 4, gy - 1, fr); g.set(x, gy, fr); g.set(x + 4, gy, fr); g.set(x, gy + 1, fr); g.set(x + 4, gy + 1, fr); g.rect(x, gy + 2, x + 4, gy + 2, fr);
            if ((f.glasses % 10) == 8) for (int y = gy; y <= gy + 1; y++) for (int xx = x + 1; xx <= x + 3; xx++) if (g.get(xx, y) == SKIN) g.set(xx, y, LENS);
        }
        g.rect(12, gy, 13, gy, (f.glasses % 10) == 9 ? WHITEC : fr);
        if ((f.glasses % 10) == 2) { g.set(6, gy - 2, fr); g.set(19, gy - 2, fr); }
        break;
    }
    }
    // piercings (ears at cols 5 / 20)
    const int ery = cy + 2;
    switch (f.pierce % 10) {
    case 1: g.mirror(5, ery, GOLDC); break;
    case 2: g.mirror(5, ery + 1, GOLDC); g.mirror(4, ery + 2, GOLDC); g.mirror(5, ery + 3, GOLDC); break;
    case 3: g.set(14, cy + 3, GOLDC); break;
    case 4: g.mirror(5, ery + 1, GOLDC); g.mirror(4, ery + 2, GOLDC); g.mirror(5, ery + 3, GOLDC); g.set(14, cy + 3, GOLDC); break;
    case 5: g.set(12, cy + 4, GOLDC); g.set(13, cy + 4, GOLDC); break;
    case 6: g.set(17, cy - 2, GOLDC); break;
    case 7: g.set(14, my + 1, GOLDC); break;
    case 8: g.mirror(5, ery, GOLDC); g.mirror(5, ery - 1, WHITEC); g.mirror(5, ery - 2, GOLDC); break;
    case 9: g.mirror(5, ery + 1, GOLDC); g.mirror(5, ery + 2, GOLDC); break;
    default: break;
    }
}

// ---------------------------------------------------------------- hats & accessories
void hat(Grid& g, const Ctx& k, int holiday) {
    const int cy = k.headCY;
    int h = k.w.hat % NUM_HATS;
    if (holiday == 1) h = 100; else if (holiday == 2) h = 101; else if (holiday == 3) h = 102;
    switch (h) {
    case HT_STRAW: g.rect(3, cy - 4, 22, cy - 4, HAT); g.rect(7, cy - 8, 18, cy - 5, HAT); g.rect(7, cy - 5, 18, cy - 5, HAT2); break;
    case HT_CAP: g.ell(13, cy - 4, 7.6f, 4.2f, HAT); g.rect(5, cy - 4, 21, 0 + cy - 4, HAT); g.rect(14, cy - 3, 22, cy - 3, HAT2); break;
    case HT_BEANIE: g.ell(13, cy - 4, 8, 5.4f, HAT); g.rect(5, cy - 3, 20, cy - 2, HAT2); g.set(13, cy - 10, HAT2); break;
    case HT_BUCKET: g.rect(6, cy - 8, 19, cy - 4, HAT); g.rect(4, cy - 3, 21, cy - 3, HAT2); break;
    case HT_HEADBAND: g.rect(6, cy - 4, 19, cy - 4, HAT); break;
    case HT_BOW: g.rect(16, cy - 9, 17, cy - 7, HAT); g.rect(20, cy - 9, 21, cy - 7, HAT); g.rect(18, cy - 8, 19, cy - 8, HAT2); break;
    case HT_FLOWERS: for (int i = 0; i < 5; i++) g.set(7 + i * 3, cy - 6 + (i & 1), i & 1 ? HAT : HAT2); break;
    case HT_BANDANA: g.ell(13, cy - 4, 7.8f, 4.6f, HAT); for (int x = 7; x <= 19; x += 3) g.set(x, cy - 5, WHITEC); g.rect(20, cy - 2, 21, cy, HAT); break;
    case HT_HEADWRAP: g.ell(13, cy - 6, 8.4f, 6.4f, HAT); g.rect(6, cy - 3, 20, cy - 3, HAT2); g.ell(15, cy - 11, 3, 2, HAT); break;
    case HT_TIARA: g.rect(8, cy - 6, 17, cy - 6, GOLDC); g.set(10, cy - 7, GOLDC); g.set(12, cy - 8, GEM); g.set(12, cy - 7, GOLDC); g.set(15, cy - 7, GOLDC); break;
    case HT_COWBOY: g.rect(2, cy - 4, 23, cy - 4, HAT); g.rect(7, cy - 10, 18, cy - 5, HAT); g.rect(7, cy - 6, 18, cy - 6, HAT2); g.set(12, cy - 10, E_); break;
    case 100: g.rect(10, cy - 13, 15, cy - 7, HAT); g.set(12, cy - 14, GOLDC); g.rect(10, cy - 9, 15, cy - 9, GOLDC); break;   // party hat
    case 101: g.rect(9, cy - 13, 16, cy - 7, DARK); g.rect(3, cy - 6, 22, cy - 6, DARK); g.rect(9, cy - 8, 16, cy - 8, HAT2); break;   // witch
    case 102: g.rect(7, cy - 9, 18, cy - 5, HAT); g.rect(6, cy - 4, 19, cy - 4, WHITEC); g.set(19, cy - 10, WHITEC); break;   // santa
    default: break;
    }
}

// ---------------------------------------------------------------- build
void build(Grid& g, const PersonDef& d, int outfit, int pose, int frame, int walk, int holiday, bool blink, int mood, int& footRow, int& hipRow) {
    g.clear();
    Ctx k{d, wearOf(d, outfit), mood, blink, pose, frame, walk, 10, 21, 0, 0, 0, 0};   // big doll head on a slim body
    const bool guy = d.guy, girl = !guy;
    const int age = d.age;
    if (age == BABY) {   // a little bundle with a big head
        g.ell(13, 30, 6, 5, TOP); g.rect(8, 32, 18, 37, TOP); g.rect(9, 35, 17, 35, TOPALT);
        g.ell(13, 25, 5.6f, 5.2f, SKIN);
        g.ell(13, 22, 5, 2.6f, HAIR);
        g.set(11, 26, blink ? SKIN2 : DARK); g.set(15, 26, blink ? SKIN2 : DARK); g.set(13, 28, LIPS);
        footRow = 38; hipRow = 34; return;
    }
    k.torsoEnd = age == ADULT ? 32 : age == TEEN ? 31 : 27;   // slim, fashionable proportions (long torso & legs)
    k.hip = k.torsoEnd + 1;
    k.legEnd = age == ADULT ? 52 : age == TEEN ? 49 : 39;
    if (pose == PO_SIT) k.legEnd = k.hip + 5;
    footRow = k.legEnd + 2; hipRow = k.hip + 1;
    const int sh = guy ? 1 : 0;
    const Wear& w = k.w;
    // back hair
    if (!(guy && d.hairStyle == 0 && false)) hairBack(g, k);
    // legs
    const int b0 = k.hip;
    int lx0 = guy ? 9 : 10, lx1 = 11, rx0 = 14, rx1 = guy ? 16 : 15;
    int lEnd = k.legEnd - (walk == 1 ? 1 : 0), rEnd = k.legEnd - (walk == 2 ? 1 : 0);
    if (walk == 1) { lx0 -= 1; lx1 -= 1; } else if (walk == 2) { rx0 += 1; rx1 += 1; }
    g.rect(lx0, b0, lx1, lEnd, SKIN); g.rect(rx0, b0, rx1, rEnd, SKIN);
    g.rect(lx1 - 0, b0, rx0, b0 + 2, SKIN);
    // bottoms
    const bool dress = w.topStyle == TS_DRESS;
    auto legsCover = [&](int toY, uint8_t v) { g.rect(lx0, b0, lx1, std::min(toY, lEnd), v); g.rect(rx0, b0, rx1, std::min(toY, rEnd), v); g.rect(lx1, b0, rx0, b0 + 2, v); };
    if (!dress) switch (w.bt) {
    case BT_PANTS: case BT_JEANS: case BT_SWEATS: case BT_OVERALLS:
        legsCover(k.legEnd, BOT);
        if (w.bt == BT_JEANS) { for (int y = b0 + 3; y < lEnd; y += 2) { g.set(lx0, y, BOTALT); g.set(rx1, y, BOTALT); } }
        if (w.bt == BT_SWEATS) { g.rect(lx0, lEnd, lx1, lEnd, BOTALT); g.rect(rx0, rEnd, rx1, rEnd, BOTALT); }
        g.rect(lx0, b0, rx1, b0, BOTALT);
        break;
    case BT_SHORTS: legsCover(b0 + 4, BOT); g.rect(lx0, b0, rx1, b0, BOTALT); break;
    case BT_SWIM: legsCover(b0 + 1, BOT); break;
    case BT_MINI: case BT_PLEATED:
        for (int y = b0; y <= b0 + 4; y++) { int e = (y - b0) / 2; g.rect(9 - e, y, 16 + e, y, BOT); }
        if (w.bt == BT_PLEATED) for (int x = 9; x <= 16; x += 2) g.set(x, b0 + 3, BOTALT);
        break;
    case BT_GOWN: case BT_MAXI:
        for (int y = b0; y <= k.legEnd + 1; y++) { int e = (y - b0) / 3; g.rect(9 - e, y, 16 + e, y, BOT); }
        if (w.bt == BT_GOWN) { g.set(11, b0 + 4, WHITEC); g.set(15, b0 + 8, WHITEC); }
        break;
    default: legsCover(k.legEnd, BOT); break;
    }
    // shoes
    auto shoe = [&](int x0, int x1, int y, bool left) {
        int xo = left ? x0 - 1 : x1 + 1;   // toes point outward
        switch (w.shoeStyle) {
        case SS_BOOTS: g.rect(x0, y - 6, x1, y + 1, SHOE); g.set(xo, y + 1, SHOE); g.rect(x0, y - 6, x1, y - 6, SOLE); break;   // knee boots
        case SS_HEELS: g.rect(x0, y, x1, y, SHOE); g.set(xo, y + 1, SHOE); g.set(left ? x1 : x0, y + 1, SOLE); break;
        case SS_PLATFORMS: g.rect(x0, y - 1, x1, y, SHOE); g.set(xo, y, SHOE); g.rect(x0 - (left ? 1 : 0), y + 1, x1 + (left ? 0 : 1), y + 1, SOLE); break;
        case SS_SANDALS: case SS_FLIPFLOPS: g.set(x0 + (left ? 0 : 1), y, SHOE); g.rect(x0 - (left ? 1 : 0), y + 1, x1 + (left ? 0 : 1), y + 1, SOLE); break;
        case SS_SLIPPERS: g.rect(x0 - 1, y, x1 + 1, y + 1, SHOE); g.set(xo, y, WHITEC); break;
        case SS_BAREFOOT: g.rect(x0, y + 1, x1, y + 1, SKIN); g.set(xo, y + 1, SKIN); break;
        case SS_LOAFERS: g.rect(x0, y, x1, y, SHOE); g.rect(x0 - (left ? 1 : 0), y + 1, x1 + (left ? 0 : 1), y + 1, SHOE); g.set(left ? x0 : x1, y, GOLDC); break;
        default: g.rect(x0, y, x1, y, SHOE); g.rect(x0 - (left ? 1 : 0), y + 1, x1 + (left ? 0 : 1), y + 1, WHITEC); break;   // sneakers
        }
    };
    shoe(lx0, lx1, lEnd + 1, true); shoe(rx0, rx1, rEnd + 1, false);
    // torso
    const int t0 = k.torso0, tE = k.torsoEnd;
    g.rect(8 - sh, t0, 17 + sh, t0 + 1, TOP);           // shoulders
    g.rect(9 - sh, t0 + 2, 16 + sh, tE, TOP);
    if (girl) { g.rect(9, tE - 3, 9, tE - 1, E_); g.rect(16, tE - 3, 16, tE - 1, E_); g.set(9, tE - 2, TOP); }   // waist
    int sleeve = 2;   // rows of sleeve on the upper arm (0 = bare, 99 = long)
    switch (w.topStyle) {
    case TS_TANK: g.rect(11, t0, 14, t0, SKIN); g.set(8 - sh, t0, SKIN); g.set(17 + sh, t0, SKIN); sleeve = 0; break;
    case TS_CROP: g.rect(11, t0, 14, t0, SKIN); g.rect(9, tE - 2, 16, tE, SKIN); g.set(12, tE - 1, SKIN2); sleeve = 1; break;
    case TS_HOODIE: sleeve = 99; g.rect(10, t0, 15, t0, TOPALT); g.set(11, t0 + 1, WHITEC); g.set(14, t0 + 1, WHITEC); g.rect(10, tE - 3, 15, tE - 2, TOPALT); break;
    case TS_JERSEY: g.rect(11, t0 + 2, 11, t0 + 5, WHITEC); g.rect(13, t0 + 2, 14, t0 + 2, WHITEC); g.set(14, t0 + 3, WHITEC); g.rect(13, t0 + 4, 14, t0 + 4, WHITEC); g.set(13, t0 + 5, WHITEC); g.rect(13, t0 + 6, 14, t0 + 6, WHITEC); break;
    case TS_BUTTON: case TS_POLO: g.set(11, t0, WHITEC); g.set(14, t0, WHITEC); g.set(12, t0, SKIN); g.set(13, t0, SKIN); for (int y = t0 + 1; y <= tE; y += 2) g.set(12, y, TOPALT); if (w.topStyle == TS_BUTTON) sleeve = 99; break;
    case TS_SWEATER: sleeve = 99; g.rect(9 - sh, tE, 16 + sh, tE, TOPALT); g.rect(11, t0, 14, t0, TOPALT); break;
    case TS_DRESS: g.rect(11, t0, 14, t0, SKIN); sleeve = 0; for (int y = tE; y <= tE + 7; y++) { int e = (y - tE) / 2; g.rect(9 - e, y, 16 + e, y, TOP); } g.rect(9, tE - 2, 16, tE - 2, TOPALT); break;
    case TS_JACKET: case TS_BLAZER: case TS_CARDIGAN: sleeve = 99; g.rect(12, t0 + 1, 13, tE, w.topStyle == TS_CARDIGAN ? SKIN : WHITEC); g.set(11, t0 + 1, TOPALT); g.set(14, t0 + 1, TOPALT); if (w.topStyle == TS_BLAZER) { g.set(11, t0 + 2, TOPALT); g.set(14, t0 + 2, TOPALT); } if (w.topStyle == TS_JACKET) { g.set(10, t0 + 3, TOPALT); g.set(15, t0 + 3, TOPALT); } break;
    case TS_TUBE: g.rect(8 - sh, t0, 17 + sh, t0 + 1, SKIN); g.rect(9, t0 + 2, 16, t0 + 2, SKIN); sleeve = 0; break;
    case TS_RAINCOAT: sleeve = 99; for (int y = tE; y <= tE + 6; y++) g.rect(9 - sh, y, 16 + sh, y, TOP); for (int y = t0 + 2; y <= tE + 5; y += 3) g.set(12, y, TOPALT); break;
    default: g.rect(11, t0, 14, t0, SKIN); g.set(12, t0 + 1, SKIN); break;   // tee
    }
    // print patterns
    if (w.pat) for (int y = t0 + 1; y <= tE; y++) for (int x = 8; x <= 17; x++) {
        if (g.get(x, y) != TOP) continue;
        bool on = false;
        switch (w.pat) {
        case 1: on = ((x + y) % 4) == 0; break; case 2: on = ((x * 3 + y * 5) % 7) == 0; break;
        case 3: on = (y % 2) == 0; break; case 4: on = (x % 3 == 0) || (y % 3 == 0); break;
        case 5: on = ((x + y) & 1) == 0; break; case 6: on = (x == 12 || x == 13) && y == t0 + 3; break;
        case 7: on = ((x * 7 + y * 3) % 9) == 0; break; default: on = (x >= 11 && x <= 14 && y >= t0 + 3 && y <= t0 + 5); break;
        }
        if (on) g.set(x, y, TOPALT);
    }
    if (w.bt == BT_OVERALLS && !dress) { g.rect(10, tE - 3, 15, tE, BOT); g.set(10, t0, BOT); g.set(15, t0, BOT); g.rect(10, t0 + 1, 10, tE - 4, BOT); g.rect(15, t0 + 1, 15, tE - 4, BOT); }
    // neck
    g.rect(12, t0 - 2, 13, t0 - 1, SKIN);
    // arms
    const int la = 7 - sh, ra = 18 + sh, handY = tE + 1;
    auto armDown = [&](int x, int lift) {
        for (int y = t0; y <= handY - 1 - lift; y++) g.set(x, y, (sleeve == 99 || y - t0 < sleeve) ? TOP : SKIN);
        g.set(x, handY - lift, SKIN);
        if (w.acc & ACC_BANGLES) g.set(x, handY - 1 - lift, GOLDC);
    };
    auto armUp = [&](int x, int hx) {
        g.set(x, t0, TOP); for (int y = t0 - 6; y < t0; y++) g.set(hx, y, (sleeve == 99) ? TOP : SKIN); g.set(hx, t0 - 7, SKIN);
    };
    if (pose == PO_DANCE) { armUp(la, la - (frame & 1)); armUp(ra, ra + ((frame + 1) & 1)); }
    else if (pose == PO_WAVE) { armDown(la, 0); armUp(ra, ra + (frame & 1)); }
    else if (pose == PO_PHONE) { armDown(la, 0); g.set(ra, t0, TOP); g.rect(ra, k.headCY + 2, ra, t0 - 1, sleeve == 99 ? TOP : SKIN); g.rect(ra - 1, k.headCY - 1, ra - 1, k.headCY + 2, PHONEC); }
    else {
        armDown(la, walk == 2 ? 1 : 0); armDown(ra, walk == 1 ? 1 : 0);
        if ((w.acc & ACC_BAG) && !G.hidePurse && walk == 0) { g.rect(ra + 1, handY - 1, ra + 3, handY + 1, ACC); g.set(ra + 2, handY - 2, GOLDC); }
    }
    // head
    const int cy = k.headCY;
    g.ell(13, cy + 1.5f, 8.0f, 8.6f, SKIN);
    g.rect(10, cy + 9, 15, cy + 10, SKIN);   // soft doll chin
    g.rect(12, cy + 11, 13, k.torso0 - 1, SKIN);   // neck
    g.set(4, cy + 1, SKIN); g.set(21, cy + 1, SKIN);   // ears
    face(g, k);
    hairFront(g, k);
    // necklace bits
    if (w.acc & (ACC_NECKLACE | ACC_PEARLS)) { g.rect(11, t0, 14, t0, (w.acc & ACC_PEARLS) ? WHITEC : GOLDC); g.set(12, t0 + 1, (w.acc & ACC_PEARLS) ? WHITEC : GOLDC); }
    if (w.acc & ACC_CHOKER) g.rect(12, t0 - 1, 13, t0 - 1, DARK);
    if ((w.acc & ACC_HOOPS) && !(d.f.pierce % 10)) { g.mirror(5, cy + 3, GOLDC); g.mirror(4, cy + 4, GOLDC); }
    if ((w.acc & ACC_SHADES) && pose != PO_SLEEP) { g.rect(7, cy + 1, 18, cy + 1, DARK); g.rect(8, cy + 2, 10, cy + 2, DARK); g.rect(15, cy + 2, 17, cy + 2, DARK); }
    if (w.acc & ACC_BOW) { g.rect(17, cy - 7, 18, cy - 6, ACC); g.rect(20, cy - 7, 21, cy - 6, ACC); g.set(19, cy - 6, ACC2); }
    if (w.acc & ACC_TIARA) { g.rect(9, cy - 6, 16, cy - 6, GOLDC); g.set(12, cy - 7, GEM); }
    if (w.acc & ACC_BEANIE) { g.ell(13, cy - 4, 8, 5.4f, ACC); g.rect(5, cy - 3, 20, cy - 2, ACC2); }
    if (w.acc & ACC_STREAKS) for (int y = 0; y < GH; y++) { if (g.get(7, y) == HAIR) g.set(7, y, ACC); if (g.get(18, y) == HAIR) g.set(18, y, ACC2); }
    if (w.acc & ACC_FLANNEL) { g.rect(8, tE, 17, tE, ACC); g.set(9, tE + 1, ACC2); g.set(16, tE + 1, ACC2); }
    if (w.acc & ACC_LEGWARM) { g.rect(lx0, lEnd - 3, lx1, lEnd, ACC); g.rect(rx0, rEnd - 3, rx1, rEnd, ACC); }
    hat(g, k, holiday);
}

RGB palette(uint8_t i, const PersonDef& d, const Wear& w) {
    RGB skin = skinRGB(d.skin), hair = hairRGB(d.hairCol);
    switch (i) {
    case SKIN: return skin;
    case SKIN2: return shade(skin, 0.8f);
    case HAIR: return hair;
    case HAIRHI: return shade(hair, 1.45f);
    case HAIRSH: return shade(hair, 0.72f);
    case TOP: return w.top;
    case TOPALT: switch (w.pat) { case 1: return RGB{30, 24, 30}; case 2: return RGB{110, 66, 30}; case 4: return shade(w.top, 0.6f); case 6: return RGB{255, 90, 150}; case 7: return RGB{255, 255, 255}; case 8: return RGB{255, 220, 80}; default: return shade(w.top, (w.top.r + w.top.g + w.top.b) > 500 ? 0.75f : 1.4f); }
    case BOT: return w.bot;
    case BOTALT: return shade(w.bot, (w.bot.r + w.bot.g + w.bot.b) > 500 ? 0.8f : 1.35f);
    case SHOE: return w.shoes;
    case SOLE: return shade(w.shoes, 0.6f);
    case WHITEC: return RGB{246, 244, 236};
    case IRIS: return eyeRGB(d.f.eyeCol);
    case DARK: return RGB{30, 20, 36};
    case LIPS: return d.lips;
    case LIPHI: return mix(d.lips, RGB{255, 255, 255}, 0.35f);
    case BLUSH: return mix(blushRGB(d.f.blush ? d.f.blush : 1), skin, 0.25f);
    case SHADOWC: return shadowRGB(d.f.shadow ? d.f.shadow : 1);
    case BROW: return shade(hair, d.hairCol >= 3 && d.hairCol != 5 ? 0.55f : 0.8f);
    case GOLDC: return RGB{255, 200, 60};
    case ACC: return w.accCol;
    case ACC2: return shade(w.accCol, 0.7f);
    case HAT: return w.hat == HT_TIARA ? RGB{255, 200, 60} : w.accCol;
    case HAT2: return RGB{246, 244, 236};
    case LENS: return (d.f.glasses % 10) == 5 ? RGB{255, 90, 150} : (d.f.glasses % 10) == 6 ? RGB{255, 210, 60} : (d.f.glasses % 10) == 7 ? RGB{40, 30, 44} : RGB{255, 150, 200};
    case FRAME: return (d.f.glasses % 10) == 2 ? RGB{200, 40, 90} : RGB{30, 20, 36};
    case STICK1: return (d.f.liner % 10) == 8 && (d.f.marks % 10) != 8 ? RGB{30, 140, 150} : RGB{150, 110, 220};
    case STICK2: return RGB{255, 90, 150};
    case GEM: return RGB{110, 220, 240};
    case PHONEC: return RGB{200, 200, 220};
    case TEETH: return RGB{255, 255, 255};
    default: return RGB{255, 0, 255};
    }
}
bool shadeable(uint8_t i) { return i == SKIN || i == HAIR || i == TOP || i == BOT || i == SHOE || i == ACC || i == HAT; }

struct Run { int16_t x, y, w; u32 col; };
struct CacheEntry { uint64_t key; std::vector<Run> runs; int footRow, hipRow; };
std::vector<CacheEntry> g_cache;
uint64_t fnv(uint64_t h, uint32_t v) { for (int i = 0; i < 4; i++) { h ^= (v >> (i * 8)) & 0xff; h *= 1099511628211ull; } return h; }
uint32_t pack(const RGB& c) { return (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b; }

const CacheEntry& get(const PersonDef& d, int outfit, int pose, int frame, int walk, int holiday, bool blink, int mood) {
    uint64_t key = 1469598103934665603ull;
    key = fnv(key, d.guy | d.age << 1 | d.skin << 4 | d.hairCol << 8 | d.hairStyle << 12 | d.bt << 18 | d.topPattern << 22);
    key = fnv(key, pack(d.top)); key = fnv(key, pack(d.bottom)); key = fnv(key, pack(d.shoes)); key = fnv(key, pack(d.lips)); key = fnv(key, pack(d.accCol));
    key = fnv(key, d.topStyle | d.shoeStyle << 5 | d.hat << 10 | (uint32_t)d.acc << 16);
    const FaceDef& f = d.f;
    key = fnv(key, f.eyes | f.eyeCol << 4 | f.brows << 8 | f.nose << 12 | f.lipShape << 16 | f.shadow << 20 | f.liner << 24 | f.blush << 28);
    key = fnv(key, f.marks | f.glasses << 4 | f.pierce << 8 | f.beard << 12);
    key = fnv(key, outfit | pose << 8 | frame << 12 | holiday << 14 | blink << 17 | walk << 18 | mood << 20);
    for (const CacheEntry& e : g_cache) if (e.key == key) return e;
    static Grid g;
    int footRow = 40, hipRow = 26;
    build(g, d, outfit, pose, frame, walk, holiday, blink, mood, footRow, hipRow);
    const Wear w = wearOf(d, outfit);
    // 3-tone ramps: lit left edge, base, shaded right edge
    static RGB col[GH][GW];
    static bool fill[GH][GW];
    for (int y = 0; y < GH; y++) for (int x = 0; x < GW; x++) {
        uint8_t v = g.c[y][x];
        fill[y][x] = v != 0;
        if (!v) continue;
        RGB c = snap(palette(v, d, w));
        if (shadeable(v)) {
            if (g.get(x + 1, y) != v && g.get(x - 1, y) == v) c = snap(shade(c, 0.74f));
            else if (g.get(x - 1, y) != v && g.get(x + 1, y) == v && g.get(x + 2, y) == v) c = snap(shade(c, 1.18f));
            else if (g.get(x, y + 1) == 0 && v == HAIR) c = snap(shade(c, 0.8f));
        }
        col[y][x] = c;
    }
    // clean dark outline around the silhouette
    for (int y = 0; y < GH; y++) for (int x = 0; x < GW; x++) {
        if (g.c[y][x]) continue;
        if (g.get(x - 1, y) || g.get(x + 1, y) || g.get(x, y - 1) || g.get(x, y + 1)) { fill[y][x] = true; col[y][x] = OUTLINE; }
    }
    CacheEntry e; e.key = key; e.footRow = footRow; e.hipRow = hipRow;
    for (int y = 0; y < GH; y++) {
        int x = 0;
        while (x < GW) {
            if (!fill[y][x]) { x++; continue; }
            RGB c = col[y][x];
            int run = 1;
            while (x + run < GW && fill[y][x + run] && col[y][x + run].r == c.r && col[y][x + run].g == c.g && col[y][x + run].b == c.b) run++;
            e.runs.push_back({(int16_t)x, (int16_t)y, (int16_t)run, C(c)});
            x += run;
        }
    }
    if (g_cache.size() >= 80) g_cache.clear();
    g_cache.push_back(e);
    return g_cache.back();
}

void stamp(const CacheEntry& ce, float ox, float oy, float cs, bool flip, int maxRow = GH) {
    for (const Run& r : ce.runs) {
        if (r.y >= maxRow) continue;
        float rx = flip ? ox + (GW - r.x - r.w) * cs : ox + r.x * cs;
        rect(rx, oy + r.y * cs, r.w * cs, cs, r.col);
    }
}

}   // namespace

float flipHeight(Age a) { return 2.f * (a == ADULT ? 54 : a == TEEN ? 51 : a == KID ? 41 : 16); }

void drawFlipPerson(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip) {
    bool forceBlink = false;
    if (pose == PO_SITSLEEP) { pose = PO_SIT; forceBlink = true; }
    if (pose == PO_LIE) pose = PO_SLEEP;
    int walk = 0;
    if (pose == PO_WALK) { int ph = ((int)(g_walkPhase * 4.f)) & 3; walk = ph == 0 ? 1 : ph == 2 ? 2 : 0; pose = PO_IDLE; }
    int frame = (pose == PO_DANCE || pose == PO_WAVE) ? ((int)(t * 4.f) & 1) : 0;
    bool blink = forceBlink || (pose != PO_SLEEP && std::fmod(t * 0.9f + d.skin * 0.37f, 3.7f) < 0.12f);
    const CacheEntry& ce = get(d, outfit, pose, frame, walk, holiday, blink, g_stickMood);
    // whole-pixel cells: 2 screen px per cell at normal size, always integer
    float cs = std::fmax(1.f, std::round(2.f * s));
    if (cs > 2.f) cs = 2.f * std::round(cs / 2.f);
    float ox = x - GW * cs / 2.f, oy = (pose == PO_SIT ? footY - ce.hipRow * cs : footY - ce.footRow * cs);
    if (pose == PO_DANCE) oy -= (float)((int)(t * 4.f) & 1) * cs;
    ox = std::floor(ox / 2.f) * 2.f; oy = std::floor(oy / 2.f) * 2.f;
    // soft drop shadow (a flat dark oval)
    if (pose != PO_SIT && pose != PO_SLEEP) rect(x - 6 * cs, footY - cs, 12 * cs, cs, C(0, 0, 0, 60));
    stamp(ce, ox, oy, cs, flip);
    if (pose == PO_SLEEP) {
        float zt = std::fmod(t, 2.f);
        rect(std::floor(x + 8 * cs + zt * 4), std::floor(oy + 2 * cs - zt * 6), 3 * cs / 2, cs / 2, C(255, 255, 255, 200));
    }
}

void drawFlipBust(const PersonDef& d0, int outfit, float cx, float cy, float r, float t) {
    PersonDef d = d0;
    if (d.age == BABY) d.age = KID;
    bool blink = std::fmod(t * 0.9f + d.skin * 0.37f, 3.7f) < 0.12f;
    const CacheEntry& ce = get(d, outfit, PO_IDLE, 0, 0, 0, blink, g_stickMood);
    float cs = std::fmax(1.f, std::floor(r * 2.f / 20.f));
    float ox = std::floor(cx - GW * cs / 2.f), oy = std::floor(cy - 11.f * cs);
    stamp(ce, ox, oy, cs, false, std::min(GH, (int)((cy + r - oy) / cs)));
}
