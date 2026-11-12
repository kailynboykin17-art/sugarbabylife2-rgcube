// "Pixel Doll" art style: chibi dress-up-game pixel art with colored outlines,
// big sparkly eyes and shaded hair. Characters are rasterized into a small cell
// grid, outlined, then drawn as merged runs of rectangles (and cached).
#include "people.hpp"
#include "draw.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace col;

namespace {

enum : uint8_t {
    E_ = 0, SKIN, HAIR, HAIRHI, TOP, TOPALT, BOT, SHOE, SOLE, EYEW, IRIS, DARK, LIPS, BLUSH, GOLDC, WHITEC,
    PHONE, HAT, HATALT, BROW, BAG, SKIN2, ACC, ACC2, NEON2, SHADES, NCOL
};
constexpr int GW = 24, GH = 48;

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
    // only paint over cells that are already a given color (used for bangs over the face)
    void ellOver(float cx, float cy, float rx, float ry, uint8_t v, int maxY) {
        for (int y = (int)(cy - ry) - 1; y <= maxY; y++)
            for (int x = (int)(cx - rx) - 1; x <= (int)(cx + rx) + 1; x++) {
                float dx = (x + 0.5f - cx) / rx, dy = (y + 0.5f - cy) / ry;
                if (dx * dx + dy * dy <= 1.f) set(x, y, v);
            }
    }
};

struct Run { int16_t x, y, w; u32 col; };
struct CacheEntry { uint64_t key; std::vector<Run> runs; int footRow; int hipRow; };
std::vector<CacheEntry> g_cache;

uint64_t fnv(uint64_t h, uint32_t v) {
    for (int i = 0; i < 4; i++) { h ^= (v >> (i * 8)) & 0xff; h *= 1099511628211ull; }
    return h;
}
uint32_t pack(const RGB& c) { return (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b; }

const RGB PSKIN[6] = {{255, 228, 210}, {246, 204, 172}, {226, 176, 134}, {198, 140, 98}, {156, 104, 74}, {110, 72, 52}};
const RGB PHAIR[8] = {{40, 30, 44}, {96, 58, 40}, {170, 108, 58}, {240, 200, 120}, {250, 238, 210}, {170, 60, 44}, {245, 130, 180}, {210, 206, 214}};
const RGB PIRIS[5] = {{110, 70, 50}, {70, 150, 110}, {90, 130, 210}, {170, 110, 200}, {60, 60, 80}};

void build(Grid& g, const PersonDef& d, int outfit, int pose, int frame, int holiday, bool blink, int& footRow) {
    g.clear();
    const bool guy = d.guy;
    const int age = d.age;   // ADULT/TEEN/KID
    RGB top = d.top, bot = d.bottom, shoes = d.shoes;
    Bottom bt = d.bt;
    int pat = d.topPattern;
    if (outfit > 0) { const OutfitDef& o = OUTFITS[outfit]; top = o.top; bot = o.bottom; shoes = o.shoes; bt = o.bt; pat = o.topPattern; }
    if (guy && (bt == BT_MINI || bt == BT_GOWN)) bt = BT_PANTS;
    (void)top; (void)bot; (void)shoes;

    // body proportions per age (head stays big = chibi)
    const int torsoEnd = age == ADULT ? 28 : age == TEEN ? 27 : 25;
    const int legEnd = pose == PO_SIT ? torsoEnd + 7 : age == ADULT ? 42 : age == TEEN ? 40 : 35;   // first shoe row
    footRow = legEnd + 2;
    const int sh = guy ? 1 : 0;   // broader shoulders

    // ---- back hair ----
    const int hs = guy ? 5 : d.hairStyle;
    switch (hs) {
    case 0: g.ell(12, 11.5f, 7.6f, 7.6f, HAIR); g.rect(4, 12, 19, torsoEnd + 4, HAIR); g.rect(5, torsoEnd + 5, 18, torsoEnd + 5, HAIR); break;
    case 1:
        g.ell(12, 11.5f, 8.4f, 8.2f, HAIR); g.rect(3, 12, 20, torsoEnd + 1, HAIR);
        for (int y = 12; y <= torsoEnd + 1; y += 3) { g.set(2, y + 1, HAIR); g.set(21, y, HAIR); }
        g.rect(4, torsoEnd + 2, 8, torsoEnd + 2, HAIR); g.rect(15, torsoEnd + 2, 19, torsoEnd + 2, HAIR);
        break;
    case 2: {
        g.ell(12, 11.5f, 7.2f, 7.2f, HAIR);
        int sw = frame & 1;
        g.ell(19.5f, 5.5f, 2.6f, 2.6f, HAIR);
        g.rect(19 + sw, 7, 21 + sw, 22, HAIR); g.rect(20 + sw, 23, 21 + sw, 25, HAIR);
        g.set(18, 5, HATALT);   // scrunchie
        g.set(18, 6, HATALT);
        break;
    }
    case 3: g.ell(12, 11.5f, 8.2f, 8.0f, HAIR); g.rect(4, 12, 19, 19, HAIR); break;
    case 4: g.ell(12, 11.5f, 7.2f, 7.2f, HAIR); g.ell(12, 3.6f, 3.2f, 3.0f, HAIR); break;
    default: g.ell(12, 11.0f, 7.0f, 6.8f, HAIR); break;
    }

    // ---- legs, shoes, bottoms ----
    const bool girl = !guy;
    const int bStart = torsoEnd + 1;
    int legL0 = 9, legL1 = 10, legR0 = 13, legR1 = 14;
    if (guy) { legL0 = 9; legL1 = 11; legR0 = 12; legR1 = 14; }
    g.rect(legL0, bStart, legL1, legEnd - 1, SKIN);
    g.rect(legR0, bStart, legR1, legEnd - 1, SKIN);
    // shoes
    if (girl && age != KID) {
        g.rect(legL0 - 1, legEnd, legL1, legEnd, SHOE); g.set(legL0 - 1, legEnd + 1, SOLE); g.set(legL1, legEnd + 1, SOLE);
        g.rect(legR0, legEnd, legR1 + 1, legEnd, SHOE); g.set(legR0, legEnd + 1, SOLE); g.set(legR1 + 1, legEnd + 1, SOLE);
    } else {
        g.rect(legL0 - 1, legEnd, legL1, legEnd, SHOE); g.rect(legL0 - 1, legEnd + 1, legL1, legEnd + 1, SOLE);
        g.rect(legR0, legEnd, legR1 + 1, legEnd, SHOE); g.rect(legR0, legEnd + 1, legR1 + 1, legEnd + 1, SOLE);
    }
    switch (bt) {
    case BT_PANTS: g.rect(9, bStart, 14, bStart + 2, BOT); g.rect(legL0, bStart + 3, legL1 + (guy ? 0 : 1), legEnd - 1, BOT); g.rect(legR0 - (guy ? 0 : 1), bStart + 3, legR1, legEnd - 1, BOT); break;
    case BT_MINI: g.rect(9, bStart, 14, bStart, BOT); g.rect(8, bStart + 1, 15, bStart + (age == KID ? 3 : 4), BOT); g.rect(7, bStart + (age == KID ? 4 : 5), 16, bStart + (age == KID ? 4 : 5), BOT); break;
    case BT_SHORTS: g.rect(9, bStart, 14, bStart + 3, BOT); break;
    case BT_GOWN:
        for (int y = bStart; y <= legEnd; y++) { int w = 3 + (y - bStart) / 2; g.rect(12 - w, y, 11 + w, y, BOT); }
        g.set(9, bStart + 5, WHITEC); g.set(14, bStart + 9, WHITEC); g.set(11, legEnd - 3, WHITEC);
        break;
    }

    // ---- torso ----
    g.rect(8 - sh, 21, 15 + sh, 21, TOP);
    g.rect(9 - sh, 22, 14 + sh, torsoEnd, TOP);
    if (girl) { g.set(11, 21, SKIN); g.set(12, 21, SKIN); }
    if (pat == 1) { for (int y = 22; y <= torsoEnd - 1; y += 2) for (int x = 9; x <= 14; x++) if ((x + y / 2) % 3 != 0) g.set(x, y, TOPALT); }
    else if (pat == 2) { g.set(10, 23, TOPALT); g.set(13, 24, TOPALT); g.set(11, 26, TOPALT); g.set(14, 27, TOPALT); g.set(9, 26, TOPALT); }
    else if (pat == 3) { for (int y = 23; y <= torsoEnd - 1; y += 2) g.rect(9 - sh, y, 14 + sh, y, TOPALT); }
    else if (pat == 4) { for (int y = 23; y <= torsoEnd - 1; y += 3) g.rect(9 - sh, y, 14 + sh, y, TOPALT); for (int y = 22; y <= torsoEnd - 1; y++) { g.set(10, y, TOPALT); g.set(13, y, TOPALT); } }
    else if (pat == 5) { for (int y = 22; y <= torsoEnd - 1; y++) for (int x = 9 - sh; x <= 14 + sh; x++) if (((x + y) & 1) == 0) g.set(x, y, TOPALT); }
    else if (pat == 6) { g.set(10, 23, TOPALT); g.set(13, 25, TOPALT); g.set(11, 26, TOPALT); g.rect(9, 22, 14, 22, TOPALT); }
    else if (pat == 7) { g.set(10, 23, TOPALT); g.set(13, 23, TOPALT); g.set(12, 25, TOPALT); g.set(9, 26, TOPALT); g.set(14, 27, TOPALT); }
    if (girl && bt != BT_GOWN) g.rect(9, torsoEnd, 14, torsoEnd, GOLDC);

    // ---- arms ----
    const int armEnd = torsoEnd + 2;
    auto armDown = [&](int x) { g.rect(x, 21, x, 22, TOP); g.rect(x, 23, x, armEnd, SKIN); g.set(x, armEnd + 1, SKIN); };
    auto armUp = [&](int x, int hx) { g.rect(x, 21, x, 22, TOP); g.rect(hx, 14, hx, 20, SKIN); g.set(hx, 13, SKIN); };
    const int la = 7 - sh, ra = 16 + sh;
    if (pose == PO_DANCE) { armUp(la, la - (frame & 1)); armUp(ra, ra + ((frame + 1) & 1)); }
    else if (pose == PO_WAVE) { armDown(la); armUp(ra, ra + (frame & 1)); }
    else if (pose == PO_PHONE) {
        armDown(la);
        g.rect(ra, 21, ra, 22, TOP); g.rect(ra, 17, ra, 20, SKIN); g.set(ra, 16, SKIN);
        g.rect(ra - 1, 12, ra, 15, PHONE); g.set(ra - 1, 13, HATALT);   // pink flip phone
    } else {
        armDown(la); armDown(ra);
        if (girl && age != KID) { g.rect(ra + 1, armEnd - 1, ra + 2, armEnd + 1, BAG); g.set(ra + 1, armEnd - 2, GOLDC); g.set(ra + 2, armEnd - 2, GOLDC); }
    }
    if (girl) g.set(la, armEnd - 1, GOLDC);   // bangle

    // ---- head ----
    g.rect(11, 19, 12, 20, SKIN);   // neck
    g.ell(12, 12, 6.4f, 6.9f, SKIN);
    g.rect(6, 9, 17, 14, SKIN);     // fuller cheeks
    g.set(5, 12, SKIN); g.set(18, 12, SKIN);   // ears

    // ---- face ----
    const int ey = 10;
    const RGB iris = PIRIS[(d.skin + d.hairCol) % 5];
    (void)iris;
    for (int side = 0; side < 2; side++) {
        int x0 = side ? 14 : 7;           // 3 cells wide
        int out = side ? x0 + 3 : x0 - 1;  // outer corner
        if (pose == PO_SLEEP || blink) {
            g.rect(x0, ey + 2, x0 + 2, ey + 2, DARK);
            if (girl) g.set(out, ey + 1, DARK);
            continue;
        }
        g.rect(x0, ey, x0 + 2, ey, DARK);                          // lash line
        if (girl) { g.set(out, ey - 1, DARK); g.set(out, ey, DARK); }   // flick
        if (side == 0) { g.set(x0, ey + 1, EYEW); g.set(x0 + 1, ey + 1, WHITEC); g.set(x0 + 2, ey + 1, IRIS); g.set(x0, ey + 2, EYEW); g.set(x0 + 1, ey + 2, IRIS); g.set(x0 + 2, ey + 2, DARK); }
        else { g.set(x0, ey + 1, WHITEC); g.set(x0 + 1, ey + 1, IRIS); g.set(x0 + 2, ey + 1, EYEW); g.set(x0, ey + 2, DARK); g.set(x0 + 1, ey + 2, IRIS); g.set(x0 + 2, ey + 2, EYEW); }
        if (girl && age != KID) g.rect(x0, ey + 3, x0 + 2, ey + 3, SKIN2);   // soft lower lid
        g.rect(x0, ey - 2, x0 + 1 + (side ? 1 : 0) - (side ? 0 : 0), ey - 2, BROW);
    }
    g.set(6, ey + 4, BLUSH); g.set(17, ey + 4, BLUSH);
    if (girl) { g.set(11, ey + 5, LIPS); g.set(12, ey + 5, LIPS); }
    else g.set(12, ey + 5, SKIN2);
    if (girl && age != KID) { g.set(5, 14, GOLDC); g.set(18, 14, GOLDC); }   // earrings

    // ---- front hair ----
    for (int y = 4; y <= 8; y++) for (int x = 5; x <= 18; x++) if (g.get(x, y) == SKIN) g.set(x, y, HAIR);
    switch (hs) {
    case 0:   // side-swept fringe
        g.rect(6, 8, 13, 8, HAIR); g.rect(6, 9, 9, 9, HAIR); g.set(6, 10, HAIR);
        g.rect(5, 9, 5, 18, HAIR); g.rect(18, 8, 18, 18, HAIR);
        break;
    case 1:   // middle part, face-framing
        g.rect(6, 8, 10, 8, HAIR); g.rect(13, 8, 17, 8, HAIR); g.set(6, 9, HAIR); g.set(17, 9, HAIR);
        g.rect(4, 9, 5, 21, HAIR); g.rect(18, 9, 19, 21, HAIR);
        g.set(11, 5, HAIRHI); g.set(12, 5, HAIRHI);
        break;
    case 3:   // blunt bangs
        g.rect(6, 8, 17, 8, HAIR); g.rect(5, 9, 5, 17, HAIR); g.rect(18, 9, 18, 17, HAIR);
        break;
    case 5:   // guy: short + spikes + sideburns
        g.set(8, 4, HAIR); g.set(11, 3, HAIR); g.set(12, 3, HAIR); g.set(15, 4, HAIR);
        g.rect(6, 8, 9, 8, HAIR); g.set(5, 9, HAIR); g.set(18, 9, HAIR); g.set(5, 10, HAIR); g.set(18, 10, HAIR);
        break;
    default: break;   // ponytail & bun: sleek top
    }
    // glossy highlight band
    g.rect(8, 6, 10, 6, HAIRHI); g.set(7, 7, HAIRHI); g.set(14, 5, HAIRHI); g.set(15, 6, HAIRHI);

    // ---- aesthetic accessories ----
    if (outfit >= FIRST_AESTHETIC && outfit < NUM_OUTFITS) {
        const uint16_t a = OUTFITS[outfit].acc;
        if (a & ACC_STREAKS) { for (int y = 6; y <= 18; y++) { if (g.get(7, y) == HAIR) g.set(7, y, ACC); if (g.get(16, y) == HAIR) g.set(16, y, NEON2); if (g.get(5, y) == HAIR) g.set(5, y, ACC); } }
        if (a & ACC_BEANIE) { for (int y = 3; y <= 7; y++) for (int x = 5; x <= 18; x++) { float dx = (x + 0.5f - 12) / 7.2f, dy = (y + 0.5f - 7.5f) / 4.6f; if (dx * dx + dy * dy <= 1.f) g.set(x, y, ACC); } g.rect(5, 7, 18, 7, ACC2); g.set(12, 2, ACC2); }
        if (a & ACC_TIARA) { g.rect(8, 5, 16, 5, WHITEC); g.set(12, 4, WHITEC); g.set(12, 3, HATALT); g.set(9, 4, WHITEC); g.set(15, 4, WHITEC); }
        if (a & ACC_CLIPS) { g.set(6, 7, ACC); g.set(7, 6, ACC); g.set(6, 6, ACC2); g.set(17, 7, ACC); g.set(16, 6, ACC); g.set(17, 6, ACC2); }
        if (a & ACC_BOW) { g.rect(15, 3, 16, 5, ACC); g.rect(18, 3, 19, 5, ACC); g.set(17, 4, ACC2); g.set(16, 6, ACC); g.set(18, 6, ACC); }
        if (a & ACC_CHOKER) { g.rect(11, 20, 12, 20, DARK); }
        if (a & ACC_PEARLS) { g.set(10, 22, WHITEC); g.set(11, 23, WHITEC); g.set(12, 23, WHITEC); g.set(13, 22, WHITEC); }
        if (a & ACC_HOOPS) { g.set(5, 14, GOLDC); g.set(4, 15, GOLDC); g.set(5, 16, GOLDC); g.set(18, 14, GOLDC); g.set(19, 15, GOLDC); g.set(18, 16, GOLDC); }
        if (a & ACC_FLANNEL) { g.rect(8, torsoEnd, 15, torsoEnd + 1, ACC); g.set(10, torsoEnd, ACC2); g.set(13, torsoEnd + 1, ACC2); g.rect(8, torsoEnd + 2, 8, torsoEnd + 4, ACC); g.rect(15, torsoEnd + 2, 15, torsoEnd + 3, ACC2); }
        if ((a & ACC_SHADES) && pose != PO_SLEEP) { g.rect(7, ey, 9, ey + 2, SHADES); g.rect(14, ey, 16, ey + 2, SHADES); g.rect(10, ey + 1, 13, ey + 1, SHADES); g.set(7, ey, WHITEC); g.set(14, ey, WHITEC); }
    }

    // ---- hats & ears ----
    if (holiday == 1) {   // party hat
        for (int r = 0; r < 5; r++) g.rect(15 - r / 2, r, 15 + r / 2, r, r == 2 ? GOLDC : HAT);
        g.set(15, 0, GOLDC);
    } else if (holiday == 2) {   // witch hat
        for (int r = 0; r < 4; r++) g.rect(12 - r, r, 12 + r, r, DARK);
        g.rect(4, 4, 19, 4, DARK); g.rect(9, 3, 15, 3, HATALT);
    } else if (holiday == 3) {   // santa hat
        g.rect(8, 2, 16, 3, HAT); g.rect(14, 1, 17, 1, HAT); g.set(18, 1, WHITEC); g.set(19, 2, WHITEC);
        g.rect(7, 4, 17, 4, WHITEC);
    }
    if (outfit == 8 && girl && holiday == 0) {
        g.rect(6, 3, 7, 4, DARK); g.rect(16, 3, 17, 4, DARK); g.set(7, 4, HATALT); g.set(16, 4, HATALT);
    }
}


// ------------------------------------------------------------------ DS mini sprites
// Small top-down-friendly pixel people (16 x 36 cells) with walk cycles.

void buildMini(Grid& g, const PersonDef& d, int outfit, int pose, int frame, int walk, int holiday, bool blink, int& footRow) {
    g.clear();
    const bool guy = d.guy, girl = !guy;
    const int age = d.age;
    RGB dummy; (void)dummy;
    Bottom bt = d.bt; int pat = d.topPattern;
    if (outfit > 0) { bt = OUTFITS[outfit].bt; pat = OUTFITS[outfit].topPattern; }
    if (guy && (bt == BT_MINI || bt == BT_GOWN)) bt = BT_PANTS;
    const int torsoEnd = age == ADULT ? 20 : age == TEEN ? 19 : 17;
    const int legEnd = age == ADULT ? 31 : age == TEEN ? 29 : 25;
    footRow = legEnd + 2;
    const int hs = guy ? 5 : d.hairStyle;

    // back hair
    switch (hs) {
    case 0: g.ell(8, 8.5f, 5.6f, 5.6f, HAIR); g.rect(3, 9, 12, torsoEnd - 1, HAIR); break;
    case 1: g.ell(8, 8.5f, 6.2f, 6.0f, HAIR); g.rect(2, 9, 13, torsoEnd - 3, HAIR); g.set(1, 12, HAIR); g.set(14, 14, HAIR); g.set(1, 16, HAIR); break;
    case 2: g.ell(8, 8.5f, 5.2f, 5.2f, HAIR); g.set(13, 4, HAIR); g.rect(13, 5, 14, 13, HAIR); g.set(14, 14 + (frame & 1), HAIR); g.set(12, 4, HATALT); break;
    case 3: g.ell(8, 8.5f, 5.8f, 5.8f, HAIR); g.rect(3, 9, 12, 13, HAIR); break;
    case 4: g.ell(8, 8.5f, 5.2f, 5.2f, HAIR); g.ell(8, 2.6f, 2.2f, 2.0f, HAIR); break;
    default: g.ell(8, 8.2f, 5.0f, 5.0f, HAIR); break;
    }
    // legs (walk cycle lifts one foot)
    const int bStart = torsoEnd + 1;
    int lx0 = 6, lx1 = guy ? 7 : 6, rx0 = guy ? 8 : 9, rx1 = 9;
    int lEnd = legEnd - (walk == 1 ? 1 : 0), rEnd = legEnd - (walk == 2 ? 1 : 0);
    g.rect(lx0, bStart, lx1, lEnd - 1, SKIN); g.rect(rx0, bStart, rx1, rEnd - 1, SKIN);
    g.rect(lx0 - (girl ? 1 : 0), lEnd, lx1, lEnd, SHOE); g.rect(lx0 - (girl ? 1 : 0), lEnd + 1, lx1, lEnd + 1, SOLE);
    g.rect(rx0, rEnd, rx1 + (girl ? 1 : 0), rEnd, SHOE); g.rect(rx0, rEnd + 1, rx1 + (girl ? 1 : 0), rEnd + 1, SOLE);
    switch (bt) {
    case BT_PANTS: g.rect(6, bStart, 9, bStart + 1, BOT); g.rect(lx0, bStart + 2, guy ? lx1 : 7, lEnd - 1, BOT); g.rect(guy ? rx0 : 8, bStart + 2, rx1, rEnd - 1, BOT); break;
    case BT_MINI: g.rect(5, bStart, 10, bStart + (age == KID ? 2 : 3), BOT); break;
    case BT_SHORTS: g.rect(6, bStart, 9, bStart + 2, BOT); break;
    case BT_GOWN: for (int y = bStart; y <= legEnd; y++) { int w = 2 + (y - bStart) / 3; g.rect(8 - w, y, 7 + w, y, BOT); } g.set(7, bStart + 4, WHITEC); break;
    }
    // torso
    const int sh = guy ? 1 : 0;
    g.rect(5 - sh, 15, 10 + sh, 15, TOP);
    g.rect(6 - sh, 16, 9 + sh, torsoEnd, TOP);
    if (girl) g.set(7, 15, SKIN), g.set(8, 15, SKIN);
    switch (pat) {
    case 1: for (int y = 16; y < torsoEnd; y += 2) { g.set(6 + (y / 2) % 2, y, TOPALT); g.set(8 + (y / 2) % 2, y, TOPALT); } break;
    case 2: g.set(7, 17, TOPALT); g.set(9, 18, TOPALT); g.set(6, 19, TOPALT); break;
    case 3: for (int y = 17; y < torsoEnd; y += 2) g.rect(6 - sh, y, 9 + sh, y, TOPALT); break;
    case 4: g.rect(6 - sh, 18, 9 + sh, 18, TOPALT); g.rect(7, 16, 7, torsoEnd - 1, TOPALT); break;
    case 5: for (int y = 16; y < torsoEnd; y++) for (int x = 6 - sh; x <= 9 + sh; x++) if (((x + y) & 1) == 0) g.set(x, y, TOPALT); break;
    case 6: g.set(7, 17, TOPALT); g.set(8, 19, TOPALT); break;
    case 7: g.set(6, 17, TOPALT); g.set(9, 17, TOPALT); g.set(7, 19, TOPALT); break;
    default: break;
    }
    if (girl && bt != BT_GOWN) g.rect(6, torsoEnd, 9, torsoEnd, GOLDC);
    // arms
    const int la = 4 - sh, ra = 11 + sh, handY = torsoEnd + 2;
    auto armDown = [&](int x, int lift) { g.rect(x, 15, x, 16, TOP); g.rect(x, 17, x, handY - 1 - lift, SKIN); g.set(x, handY - lift, SKIN); };
    auto armUp = [&](int x, int hx) { g.rect(x, 15, x, 16, TOP); g.rect(hx, 9, hx, 14, SKIN); g.set(hx, 8, SKIN); };
    if (pose == PO_DANCE) { armUp(la, la - (frame & 1)); armUp(ra, ra + ((frame + 1) & 1)); }
    else if (pose == PO_WAVE) { armDown(la, 0); armUp(ra, ra + (frame & 1)); }
    else if (pose == PO_PHONE) { armDown(la, 0); g.rect(ra, 15, ra, 16, TOP); g.rect(ra, 12, ra, 14, SKIN); g.rect(ra, 8, ra, 11, PHONE); g.set(ra, 9, HATALT); }
    else {
        armDown(la, walk == 2 ? 1 : 0); armDown(ra, walk == 1 ? 1 : 0);
        if (girl && age != KID && walk == 0) { g.rect(ra + 1, handY - 2, ra + 2, handY, BAG); g.set(ra + 1, handY - 3, GOLDC); }
    }
    // head
    g.rect(7, 13, 8, 14, SKIN);
    g.ell(8, 9.2f, 4.4f, 4.6f, SKIN);
    g.set(3, 10, SKIN); g.set(12, 10, SKIN);
    // face
    const int ey = 9;
    if (pose == PO_SLEEP || blink) { g.rect(5, ey + 1, 6, ey + 1, DARK); g.rect(9, ey + 1, 10, ey + 1, DARK); }
    else {
        g.set(5, ey, DARK); g.set(6, ey, DARK); g.set(5, ey + 1, IRIS); g.set(6, ey + 1, DARK);
        g.set(9, ey, DARK); g.set(10, ey, DARK); g.set(10, ey + 1, IRIS); g.set(9, ey + 1, DARK);
        if (girl) { g.set(4, ey - 1, DARK); g.set(11, ey - 1, DARK); }
    }
    g.set(4, ey + 2, BLUSH); g.set(11, ey + 2, BLUSH);
    if (girl) { g.set(7, ey + 3, LIPS); g.set(8, ey + 3, LIPS); } else g.set(8, ey + 3, SKIN2);
    if (girl && age != KID) { g.set(3, 11, GOLDC); g.set(12, 11, GOLDC); }
    // front hair
    for (int y = 3; y <= 6; y++) for (int x = 3; x <= 12; x++) if (g.get(x, y) == SKIN) g.set(x, y, HAIR);
    switch (hs) {
    case 0: g.rect(4, 7, 8, 7, HAIR); g.set(4, 8, HAIR); g.rect(3, 8, 3, 14, HAIR); g.rect(12, 7, 12, 14, HAIR); break;
    case 1: g.rect(4, 7, 6, 7, HAIR); g.rect(9, 7, 11, 7, HAIR); g.rect(2, 8, 3, 16, HAIR); g.rect(12, 8, 13, 16, HAIR); g.set(7, 3, HAIRHI); g.set(8, 3, HAIRHI); break;
    case 3: g.rect(4, 7, 11, 7, HAIR); g.rect(3, 8, 3, 13, HAIR); g.rect(12, 8, 12, 13, HAIR); break;
    case 5: g.set(5, 2, HAIR); g.set(8, 2, HAIR); g.set(11, 3, HAIR); g.rect(4, 7, 6, 7, HAIR); g.set(3, 8, HAIR); g.set(12, 8, HAIR); break;
    default: break;
    }
    g.rect(5, 4, 6, 4, HAIRHI); g.set(4, 5, HAIRHI); g.set(10, 4, HAIRHI);
    // aesthetic accessories
    if (outfit >= FIRST_AESTHETIC && outfit < NUM_OUTFITS) {
        const uint16_t a = OUTFITS[outfit].acc;
        if (a & ACC_STREAKS) for (int y = 4; y <= 14; y++) { if (g.get(4, y) == HAIR) g.set(4, y, ACC); if (g.get(11, y) == HAIR) g.set(11, y, NEON2); }
        if (a & ACC_BEANIE) { for (int y = 2; y <= 6; y++) for (int x = 3; x <= 12; x++) { float dx = (x + 0.5f - 8) / 5.4f, dy = (y + 0.5f - 6.2f) / 4.0f; if (dx * dx + dy * dy <= 1.f) g.set(x, y, ACC); } g.rect(3, 6, 12, 6, ACC2); }
        if (a & ACC_TIARA) { g.rect(5, 4, 10, 4, WHITEC); g.set(8, 3, HATALT); }
        if (a & ACC_CLIPS) { g.set(4, 6, ACC); g.set(11, 6, ACC); }
        if (a & ACC_BOW) { g.set(10, 2, ACC); g.set(12, 2, ACC); g.set(11, 3, ACC2); g.set(10, 3, ACC); g.set(12, 3, ACC); }
        if (a & ACC_CHOKER) g.rect(7, 14, 8, 14, DARK);
        if (a & ACC_PEARLS) { g.set(7, 16, WHITEC); g.set(8, 16, WHITEC); }
        if (a & ACC_HOOPS) { g.set(2, 11, GOLDC); g.set(3, 12, GOLDC); g.set(13, 11, GOLDC); g.set(12, 12, GOLDC); }
        if (a & ACC_FLANNEL) { g.rect(5, torsoEnd, 10, torsoEnd, ACC); g.set(5, torsoEnd + 1, ACC); g.set(5, torsoEnd + 2, ACC2); }
        if ((a & ACC_SHADES) && pose != PO_SLEEP) { g.rect(4, ey, 11, ey, SHADES); g.rect(5, ey + 1, 6, ey + 1, SHADES); g.rect(9, ey + 1, 10, ey + 1, SHADES); }
    }
    // hats
    if (holiday == 1) { g.set(9, 0, GOLDC); g.rect(8, 1, 10, 1, HAT); g.rect(8, 2, 11, 2, GOLDC); }
    else if (holiday == 2) { g.set(8, 0, DARK); g.rect(7, 1, 9, 1, DARK); g.rect(2, 2, 13, 2, DARK); g.rect(6, 1, 10, 1, HATALT); }
    else if (holiday == 3) { g.rect(5, 1, 11, 2, HAT); g.set(12, 0, WHITEC); g.rect(4, 3, 12, 3, WHITEC); }
    if (outfit == 8 && girl && holiday == 0) { g.set(4, 2, DARK); g.set(11, 2, DARK); g.set(4, 3, DARK); g.set(11, 3, DARK); }
}

RGB palette(uint8_t i, const PersonDef& d, int outfit) {
    RGB top = d.top, bot = d.bottom, shoes = d.shoes;
    int pat = d.topPattern;
    if (outfit > 0) { top = OUTFITS[outfit].top; bot = OUTFITS[outfit].bottom; shoes = OUTFITS[outfit].shoes; pat = OUTFITS[outfit].topPattern; }
    switch (i) {
    case SKIN: return PSKIN[d.skin % 6];
    case SKIN2: return shade(PSKIN[d.skin % 6], 0.9f);
    case HAIR: return PHAIR[d.hairCol % 8];
    case HAIRHI: return shade(PHAIR[d.hairCol % 8], 1.45f);
    case TOP: return pat == 1 ? RGB{246, 242, 236} : top;
    case TOPALT:
        switch (pat) {
        case 1: return RGB{34, 28, 32};
        case 2: return RGB{90, 56, 36};
        case 4: return RGB{170, 40, 50};
        case 5: return HOT;
        case 6: return RGB{255, 255, 255};
        case 7: return RGB{255, 255, 255};
        default: return RGB{255, 255, 255};
        }
    case ACC: return outfit >= FIRST_AESTHETIC && outfit < NUM_OUTFITS ? OUTFITS[outfit].accCol : RGB{255, 255, 255};
    case ACC2: return shade(outfit >= FIRST_AESTHETIC && outfit < NUM_OUTFITS ? OUTFITS[outfit].accCol : RGB{255, 255, 255}, 0.7f);
    case NEON2: return RGB{80, 200, 255};
    case SHADES: return RGB{34, 26, 36};
    case BOT: return bot;
    case SHOE: return shoes;
    case SOLE: return shade(shoes, 0.6f);
    case EYEW: return RGB{255, 255, 255};
    case IRIS: return PIRIS[(d.skin + d.hairCol) % 5];
    case DARK: return RGB{44, 30, 40};
    case LIPS: return d.lips;
    case BLUSH: return RGB{255, 150, 170};
    case GOLDC: return GOLD;
    case WHITEC: return RGB{255, 255, 255};
    case PHONE: return RGB{214, 210, 220};
    case HAT: return RGB{232, 50, 80};
    case HATALT: return HOT;
    case BROW: return shade(PHAIR[d.hairCol % 8], d.hairCol >= 3 ? 0.6f : 0.85f);
    case BAG: return pat == 2 ? GOLD : HOT;
    default: return RGB{255, 0, 255};
    }
}

bool outlines(uint8_t i) { return i != E_ && i != BLUSH; }
bool shadeable(uint8_t i) { return i == SKIN || i == HAIR || i == TOP || i == BOT || i == SHOE; }

}   // namespace

int g_walkFrame = 0;
static void drawPixelImpl(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip, bool mini);
void drawPixelPerson(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip) {
    drawPixelImpl(d, outfit, x, footY, s, t, holiday, pose, flip, false);
}
void drawMiniPerson(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip) {
    drawPixelImpl(d, outfit, x, footY, s, t, holiday, pose, flip, true);
}
static void drawPixelImpl(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip, bool mini) {
    bool forceBlink = false;
    if (pose == PO_SITSLEEP) { pose = PO_SIT; forceBlink = true; }
    if (pose == PO_WALK || pose == PO_LIE) pose = PO_IDLE;
    // animation frame state (keeps the cache small)
    int frame = (pose == PO_DANCE || pose == PO_WAVE) ? ((int)(t * 4.f) & 1) : ((d.hairStyle == 2) ? ((int)(t * 1.5f) & 1) : 0);
    bool blink = forceBlink || (pose != PO_SLEEP && std::fmod(t * 0.9f + d.skin * 0.37f, 3.7f) < 0.12f);
    uint64_t key = 1469598103934665603ull;
    key = fnv(key, d.guy | d.age << 1 | d.skin << 4 | d.hairCol << 8 | d.hairStyle << 12 | d.bt << 16 | d.topPattern << 20);
    key = fnv(key, pack(d.top)); key = fnv(key, pack(d.bottom)); key = fnv(key, pack(d.shoes)); key = fnv(key, pack(d.lips));
    int walk = mini ? g_walkFrame : 0;
    key = fnv(key, outfit | pose << 8 | frame << 12 | holiday << 16 | blink << 20 | mini << 24 | walk << 25);

    const CacheEntry* ce = nullptr;
    for (const CacheEntry& e : g_cache) if (e.key == key) { ce = &e; break; }
    if (!ce) {
        static Grid g;
        int footRow = 44;
        if (mini) buildMini(g, d, outfit, pose, frame, walk, holiday, blink, footRow);
        else build(g, d, outfit, pose, frame, holiday, blink, footRow);
        // outline pass: empty cells touching the figure take a dark shade of their neighbor
        static uint8_t outl[GH][GW];
        memset(outl, 0, sizeof outl);
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++) {
                if (g.c[y][x]) continue;
                const int nx[4] = {x, x - 1, x + 1, x}, ny[4] = {y + 1, y, y, y - 1};
                for (int k = 0; k < 4; k++) { uint8_t n = g.get(nx[k], ny[k]); if (outlines(n)) { outl[y][x] = n; break; } }
            }
        CacheEntry e;
        e.key = key;
        e.footRow = footRow;
        e.hipRow = (d.age == ADULT ? 28 : d.age == TEEN ? 27 : 25) + 1;
        for (int y = 0; y < GH; y++) {
            int x = 0;
            while (x < GW) {
                uint8_t v = g.c[y][x];
                RGB colr;
                bool filled = true;
                if (v) {
                    colr = palette(v, d, outfit);
                    if (shadeable(v) && (x + 1 >= GW || g.c[y][x + 1] == 0)) colr = shade(colr, 0.82f);
                } else if (outl[y][x]) {
                    colr = shade(palette(outl[y][x], d, outfit), 0.42f);
                } else filled = false;
                int run = 1;
                if (filled) {
                    while (x + run < GW) {
                        int xx = x + run;
                        uint8_t v2 = g.c[y][xx];
                        RGB c2;
                        if (v2) { c2 = palette(v2, d, outfit); if (shadeable(v2) && (xx + 1 >= GW || g.c[y][xx + 1] == 0)) c2 = shade(c2, 0.82f); }
                        else if (outl[y][xx]) c2 = shade(palette(outl[y][xx], d, outfit), 0.42f);
                        else break;
                        if (c2.r != colr.r || c2.g != colr.g || c2.b != colr.b) break;
                        run++;
                    }
                    e.runs.push_back({(int16_t)x, (int16_t)y, (int16_t)run, C(colr)});
                }
                x += run;
            }
        }
        if (g_cache.size() >= 64) g_cache.clear();
        g_cache.push_back(e);
        ce = &g_cache.back();
    }

    float cs = 2.5f * s;
    if (mini) cs = std::fmax(2.f, std::round(2.f * s));
    float bob = pose == PO_DANCE ? -(float)((int)(t * 4.f) & 1) * cs : 0.f;
    const int gw = mini ? 16 : GW;
    float ox = x - gw * cs / 2.f, oy = (pose == PO_SIT && !mini ? footY - ce->hipRow * cs : footY - (ce->footRow) * cs) + bob;
    if (mini) { ox = std::floor(ox / cs) * cs; oy = std::floor(oy / cs) * cs; }
    for (const Run& r : ce->runs) {
        float rx = flip ? ox + (gw - r.x - r.w) * cs : ox + r.x * cs;
        rect(std::floor(rx), std::floor(oy + r.y * cs), std::ceil(r.w * cs), std::ceil(cs), r.col);
    }
    if (mini) return;   // the DS overlay draws its own extras
    if (pose == PO_SLEEP) {
        float zt = std::fmod(t, 2.f);
        text("z", x + 10 * s + zt * 4, oy + 2 * cs - zt * 6, 0.45f * s + 0.1f, C(255, 255, 255, (uint8_t)(220 - zt * 100)));
    }
    float sp = std::fmod(t * 0.5f + d.skin * 0.21f + d.hairCol * 0.13f, 3.f);
    if (sp < 0.5f) sparkle(ox + 4 * cs, oy + 6 * cs, 2.5f + sp * 3, C(255, 255, 255, (uint8_t)(220 - sp * 400)));
}
