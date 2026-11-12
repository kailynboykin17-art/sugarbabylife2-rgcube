// ============================================================ Stick Figure styles
// Flash-era browser-game look: big round heads, dot eyes, rosy blush, skin-tone
// stick limbs and simple chunky clothes, all with bold dark outlines.
//   7 Flash Stick - the default: coloured limbs, crisp outlines
//   8 Doodle      - notebook doodle: thin black stick limbs, marker clothes
//   9 Chibi       - kawaii: giant heads, tiny bodies
// Every shape is drawn twice: once grown by the outline width in ink, then filled.
// Shapes inside one "group" (all the hair, one piece of clothing) share a single
// merged outline, so hair reads as one clean shape instead of a pile of circles.
#include "people.hpp"
#include "draw.hpp"
#include <cmath>
#include <algorithm>

using namespace col;

RGB skinRGB(int i);
RGB hairRGB(int i);

int g_stickMood = 0;   // set by drawSim: 0 smile, 1 grin, 2 sad, 3 angry, 4 flirty, 5 surprised, 6 bored, 7 asleep

const char* HAIRSTYLE_NAMES[NUM_HAIRSTYLES] = {
    "Sleek Long", "Big Waves", "Ponytail", "Bob", "Top Bun", "Short Crop", "Curls", "Box Braids", "Afro", "Afro Puffs",
    "Pixie", "Locs", "Pigtails", "Space Buns", "Cornrows", "Twists", "Fade", "Buzz Cut", "Curly Top", "Side Part",
    "Mohawk", "Emo Fringe", "High Puff", "Bantu Knots", "Bald", "Shag", "Half-Up", "Spiky",
    "Y2K Flip", "Wispy Bangs", "Claw Clip", "Pigtail Puffs", "Braided Pony", "Butterfly Layers", "Wolf Cut", "Twin Tails",
    "Hime Cut", "Curly Low Bun", "Frohawk", "Twists",
    "Goddess Locs", "Knotless Braids", "Fulani Braids", "Half-Up Knot", "Bandana Pony", "Pixie Cut", "Bantu Knots", "Body Wave",
    "Bubble Pony", "Side Curls", "Mermaid Waves", "Half-Up Buns", "Sleek Top Bun", "Crimped Y2K", "Curly Top Fade", "Loc Bun",
    "Cornrows", "Soft Mullet",
    "Claw Clip Half-Up", "Braided Half-Up", "Ribbon Braid", "Blowout Curls", "Twin French Braids", "Messy Low Bun",
    "Velvet Bow Waves", "Curly Half-Up", "Sleek Stitch Braids", "Fulani Flower Braids",
    "Mia Curls", "Eli Curls", "Sunday Curls",
    "Bouncy High Pony", "Wavy Low Pigtails", "360 Waves", "Short Twists", "Curly Taper Fade",
    "Glam Blowout", "Swoop Spiral Curls", "Curly Pigtail Bunches", "Big Curly Mane", "Layered Flip Waves",
    "Sleek Slick-Back", "Voluminous Curls", "Afro & Headband", "Curly Fro Bangs",
    "Long Copper Locs", "Messy Curly Bun", "Sleek Blunt Bangs", "Curly Bob", "Side-Part Glam Waves",
    "Spiky Space Buns", "Sleek Side Braid", "Top Knot & Swoop", "Twin Braids", "Curly Cloud"};
const char* TOP_NAMES[NUM_TOPS] = {"Tee", "Tank Top", "Crop Top", "Hoodie", "Jersey", "Button-Up", "Sweater", "Sundress",
                                   "Denim Jacket", "Blazer", "Tube Top", "Cardigan", "Polo", "Raincoat", "Shirtless"};
const char* BOTTOM_NAMES[NUM_BOTTOMS] = {"Pants", "Mini Skirt", "Shorts", "Gown", "Jeans", "Maxi Skirt", "Overalls", "Joggers", "Pleated Skirt", "Swim"};
const char* SHOE_NAMES[NUM_SHOES] = {"Sneakers", "Sandals", "Boots", "Heels", "Platforms", "Flip-Flops", "Loafers", "Slippers", "Barefoot"};
const char* HAT_NAMES[NUM_HATS] = {"None", "Straw Hat", "Cap", "Beanie", "Bucket Hat", "Headband", "Big Bow", "Flower Crown",
                                   "Bandana", "Headwrap", "Tiara", "Cowboy Hat"};

// ---------------------------------------------------------------- outlined pen
struct SP {
    Pen p;
    u32 inkc;
    float g;           // outline width in local units
    bool ink = false;
    float cutY = 1e9f; // bust portraits stop here
    SP(const Pen& pp, u32 ic, float gg) : p(pp), inkc(ic), g(gg) {}
    float cy(float y) const { return y < cutY ? y : cutY; }
    void ci(float x, float y, float r, u32 c) const { if (y - r > cutY) return; if (ink) p.Ci(x, y, r + g, inkc); else p.Ci(x, y, r, c); }
    void el(float x, float y, float w, float h, u32 c) const { if (y - h / 2 > cutY) return; if (ink) p.E(x, y, w + 2 * g, h + 2 * g, inkc); else p.E(x, y, w, h, c); }
    void ln(float x0, float y0, float x1, float y1, float th, u32 c) const {
        if (y0 > cutY && y1 > cutY) return;
        if (y1 > cutY) { float k = (cutY - y0) / (y1 - y0); x1 = x0 + (x1 - x0) * k; y1 = cutY; }
        if (y0 > cutY) { float k = (cutY - y1) / (y0 - y1); x0 = x1 + (x0 - x1) * k; y0 = cutY; }
        if (ink) p.L(x0, y0, x1, y1, th + 2 * g, inkc); else p.L(x0, y0, x1, y1, th, c);
    }
    void dot(float x, float y, float r, u32 c) const { if (!ink && y - r <= cutY) p.Ci(x, y, r, c); }   // detail: fill pass only
    void dl(float x0, float y0, float x1, float y1, float th, u32 c) const { if (!ink && y0 <= cutY && y1 <= cutY) p.L(x0, y0, x1, y1, th, c); }
    void qd(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, u32 c) const {
        y0 = cy(y0); y1 = cy(y1); y2 = cy(y2); y3 = cy(y3);
        if (y0 == y3 && y1 == y2 && y0 == y1) return;
        if (ink) {
            p.Q(x0, y0, x1, y1, x2, y2, x3, y3, inkc);
            float t = 2 * g;
            p.L(x0, y0, x1, y1, t, inkc); p.L(x1, y1, x2, y2, t, inkc); p.L(x2, y2, x3, y3, t, inkc); p.L(x3, y3, x0, y0, t, inkc);
            p.Ci(x0, y0, g, inkc); p.Ci(x1, y1, g, inkc); p.Ci(x2, y2, g, inkc); p.Ci(x3, y3, g, inkc);
        } else p.Q(x0, y0, x1, y1, x2, y2, x3, y3, c);
    }
    void tr(float x0, float y0, float x1, float y1, float x2, float y2, u32 c) const {
        y0 = cy(y0); y1 = cy(y1); y2 = cy(y2);
        if (ink) {
            p.T(x0, y0, x1, y1, x2, y2, inkc);
            float t = 2 * g;
            p.L(x0, y0, x1, y1, t, inkc); p.L(x1, y1, x2, y2, t, inkc); p.L(x2, y2, x0, y0, t, inkc);
            p.Ci(x0, y0, g, inkc); p.Ci(x1, y1, g, inkc); p.Ci(x2, y2, g, inkc);
        } else p.T(x0, y0, x1, y1, x2, y2, c);
    }
    void rc(float x, float y, float w, float h, u32 c) const { qd(x, y, x + w, y, x + w, y + h, x, y + h, c); }
    // fill-pass-only details that respect the bust cut
    void fq(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, u32 c) const {
        if (ink || (y0 > cutY && y1 > cutY && y2 > cutY && y3 > cutY)) return;
        p.Q(x0, cy(y0), x1, cy(y1), x2, cy(y2), x3, cy(y3), c);
    }
    void ft(float x0, float y0, float x1, float y1, float x2, float y2, u32 c) const { if (!ink && !(y0 > cutY && y1 > cutY && y2 > cutY)) p.T(x0, cy(y0), x1, cy(y1), x2, cy(y2), c); }
    void fr(float x, float y, float w, float h, u32 c) const { fq(x, y, x + w, y, x + w, y + h, x, y + h, c); }
    // a bent limb: two segments with rounded joints and ends
    void limb(float x0, float y0, float x1, float y1, float x2, float y2, float th, u32 c) const {
        ln(x0, y0, x1, y1, th, c); ln(x1, y1, x2, y2, th, c);
        if (y1 <= cutY) ci(x1, y1, th / 2, c);
        if (y2 <= cutY) ci(x2, y2, th / 2, c);
        if (y0 <= cutY) ci(x0, y0, th / 2, c);
    }
};
template <class F> static void grp(SP& sp, F f) { sp.ink = true; f(); sp.ink = false; f(); }

// ---------------------------------------------------------------- what they're wearing
struct Wear {
    RGB top, bot, shoes, accCol;
    uint8_t ts, bt, ss, hat, pat;
    uint16_t acc;
};
static Wear resolveWear(const PersonDef& d, int outfit) {
    Wear w{d.top, d.bottom, d.shoes, d.accCol, d.topStyle, (uint8_t)d.bt, d.shoeStyle, d.hat, d.topPattern, d.acc};
    if (outfit > 0 && outfit < NUM_OUTFITS) {
        const OutfitDef& o = OUTFITS[outfit];
        w.top = o.top; w.bot = o.bottom; w.shoes = o.shoes; w.bt = o.bt; w.pat = o.topPattern; w.acc = o.acc; w.accCol = o.accCol;
        w.ts = o.topStyle; w.ss = o.shoeStyle; w.hat = o.hat;
        if (d.guy) {   // preset looks adapt to the guys
            if (w.bt == BT_MINI || w.bt == BT_PLEATED) w.bt = BT_SHORTS;
            if (w.bt == BT_GOWN) { w.bt = BT_PANTS; if (w.ts == TS_TUBE || w.ts == TS_DRESS) w.ts = TS_BLAZER; }
            if (w.bt == BT_MAXI) w.bt = BT_PANTS;
            if (w.ts == TS_DRESS) w.ts = TS_BUTTON;
            if (w.ts == TS_TUBE || w.ts == TS_CROP) w.ts = TS_TEE;
            if (w.ss == SS_HEELS) w.ss = SS_LOAFERS;
            if (w.ss == SS_PLATFORMS) w.ss = SS_SNEAKERS;
            if (w.hat == HT_TIARA || w.hat == HT_BOW || w.hat == HT_FLOWERS) w.hat = HT_NONE;
            w.acc &= (uint16_t)~(ACC_BOW | ACC_TIARA | ACC_CLIPS | ACC_PEARLS | ACC_LEGWARM);
        }
    }
    if (d.age == KID) {   // kid-friendly versions
        if (w.ss == SS_HEELS || w.ss == SS_PLATFORMS) w.ss = SS_SNEAKERS;
        if (w.ts == TS_BLAZER) w.ts = TS_BUTTON;
        if (w.ts == TS_TUBE || w.ts == TS_CROP) w.ts = TS_TEE;
        if (w.bt == BT_GOWN) { w.ts = TS_DRESS; w.bot = w.top; }
    }
    if (w.ts >= NUM_TOPS) w.ts = 0;
    if (w.bt >= NUM_BOTTOMS) w.bt = 0;
    if (w.ss >= NUM_SHOES) w.ss = 0;
    if (w.hat >= NUM_HATS) w.hat = 0;
    return w;
}

// ---------------------------------------------------------------- skeleton
struct Skel { float H, hc, sh, hip, L, T, shW, hipW, limb; };
static Skel skelFor(Age a, bool guy, int style) {
    float H, L, T;
    const bool chibi = style == 9 || style == 11;   // 11 = Cutie Pop
    if (chibi) { if (a == KID) { H = 20; L = 30; T = 21; } else if (a == TEEN) { H = 19; L = 35; T = 23; } else { H = 19; L = 36; T = 24; } }
    else { if (a == KID) { H = 15.5f; L = 48; T = 27; } else if (a == TEEN) { H = 12.8f; L = 54; T = 30; } else { H = 12.5f; L = 56; T = 31; } }
    Skel k;
    k.H = H; k.L = L; k.T = T;
    k.hip = 120 - L;
    k.sh = k.hip - T;
    k.hc = k.sh - 3.2f - H * 0.93f;
    k.shW = guy ? 7.6f : 6.8f;
    k.hipW = guy ? 5.8f : 6.4f;
    if (chibi) { k.shW *= 0.85f; k.hipW *= 0.85f; }
    k.limb = style == 8 ? 1.6f : (a == KID ? 3.5f : 3.2f);
    return k;
}
static float ageScale(Age a) { return a == ADULT ? 1.0f : a == TEEN ? 0.94f : a == KID ? 0.7f : 0.62f; }

float stickHeight(Age a, int style) {
    if (a == BABY) return 34;
    if (style == 10) return (120 - 4.5f) * ageScale(a);
    Skel k = skelFor(a, false, style);
    return (120 - (k.hc - k.H)) * ageScale(a);
}

// ---------------------------------------------------------------- hair
// back = drawn behind the body; front = drawn over the head
static void hairBack(SP& sp, int st, float cx, float hc, float H, u32 HR, float t) {
    switch (st) {
    case 0:   // sleek long
        sp.el(cx, hc - 0.05f * H, 2.3f * H, 2.2f * H, HR);
        sp.qd(cx - 1.12f * H, hc, cx + 1.12f * H, hc, cx + 1.18f * H, hc + 2.4f * H, cx - 1.18f * H, hc + 2.4f * H, HR);
        for (int i = 0; i < 5; i++) { float x0 = cx - 1.18f * H + i * 0.472f * H; sp.tr(x0, hc + 2.38f * H, x0 + 0.472f * H, hc + 2.38f * H, x0 + 0.236f * H + (i - 2) * 0.03f * H, hc + (2.62f + (i % 2) * 0.12f) * H, HR); }
        break;
    case 1:   // big waves
        sp.el(cx, hc - 0.05f * H, 2.5f * H, 2.3f * H, HR);
        sp.qd(cx - 1.25f * H, hc, cx + 1.25f * H, hc, cx + 1.45f * H, hc + 2.1f * H, cx - 1.45f * H, hc + 2.1f * H, HR);
        for (int i = 0; i < 3; i++) { sp.ci(cx - 1.3f * H + i * 0.1f * H, hc + 0.6f * H + i * 0.6f * H, 0.42f * H, HR); sp.ci(cx + 1.3f * H - i * 0.1f * H, hc + 0.6f * H + i * 0.6f * H, 0.42f * H, HR); }
        break;
    case 2: {  // ponytail
        float sw = std::sin(t * 2.2f) * 0.12f * H;
        sp.ci(cx + 0.95f * H, hc - 0.7f * H, 0.38f * H, HR);
        sp.qd(cx + 0.95f * H, hc - 0.85f * H, cx + 1.45f * H, hc - 0.6f * H, cx + 1.5f * H + sw, hc + 1.4f * H, cx + 1.05f * H + sw, hc + 1.2f * H, HR);
        break;
    }
    case 3:   // bob
        sp.el(cx, hc, 2.4f * H, 2.25f * H, HR);
        sp.qd(cx - 1.2f * H, hc, cx + 1.2f * H, hc, cx + 1.22f * H, hc + 0.95f * H, cx - 1.22f * H, hc + 0.95f * H, HR);
        break;
    case 6: case 18:   // curls / curly top: a rounded curly mass
        if (st == 6) {
            sp.el(cx, hc + 0.1f * H, 2.8f * H, 2.7f * H, HR);
            for (int i = 0; i < 9; i++) { float a = 3.14159f * (0.1f + i * 0.1f); sp.ci(cx - std::cos(a) * 1.35f * H, hc + 0.2f * H - std::sin(a) * 1.25f * H, 0.38f * H, HR); }
            for (int i = 0; i < 3; i++) { sp.ci(cx - 1.35f * H, hc + 0.5f * H + i * 0.45f * H, 0.36f * H, HR); sp.ci(cx + 1.35f * H, hc + 0.5f * H + i * 0.45f * H, 0.36f * H, HR); }
        }
        break;
    case 7: case 11: {  // box braids / locs: strands hanging down
        sp.el(cx, hc - 0.05f * H, 2.3f * H, 2.2f * H, HR);
        int n = st == 7 ? 7 : 6;
        float len = st == 7 ? 2.6f : 2.3f, th = st == 7 ? 0.24f * H : 0.32f * H;
        for (int i = 0; i < n; i++) {
            float u = (i / (float)(n - 1)) - 0.5f;
            float x0 = cx + u * 2.0f * H, sw = std::sin(t * 1.8f + i) * 0.05f * H;
            sp.ln(x0, hc - 0.2f * H, x0 + u * 0.5f * H + sw, hc + len * H, th, HR);
            sp.ci(x0 + u * 0.5f * H + sw, hc + len * H, th * 0.6f, HR);
        }
        break;
    }
    case 8:   // afro
        sp.ci(cx, hc - 0.2f * H, 1.55f * H, HR);
        break;
    case 12: {  // pigtails
        float sw = std::sin(t * 2.4f) * 0.08f * H;
        for (int s = -1; s <= 1; s += 2) {
            sp.ci(cx + s * 1.05f * H, hc + 0.05f * H, 0.3f * H, HR);
            sp.qd(cx + s * 1.0f * H, hc, cx + s * 1.5f * H, hc + 0.1f * H, cx + s * 1.45f * H + sw, hc + 1.5f * H, cx + s * 1.1f * H + sw, hc + 1.35f * H, HR);
        }
        break;
    }
    case 14:   // cornrows: little braid tails at the back
        for (int i = 0; i < 4; i++) { float x0 = cx - 0.6f * H + i * 0.4f * H; sp.ln(x0, hc + 0.3f * H, x0 + (i - 1.5f) * 0.1f * H, hc + 1.5f * H, 0.2f * H, HR); }
        break;
    case 21:   // emo fringe: shaggy back to the chin
        sp.el(cx, hc - 0.05f * H, 2.3f * H, 2.25f * H, HR);
        sp.qd(cx - 1.15f * H, hc, cx + 1.15f * H, hc, cx + 1.1f * H, hc + 1.15f * H, cx - 1.1f * H, hc + 1.15f * H, HR);
        for (int i = 0; i < 4; i++) sp.tr(cx - 1.1f * H + i * 0.6f * H, hc + 1.1f * H, cx - 0.6f * H + i * 0.6f * H, hc + 1.1f * H, cx - 0.85f * H + i * 0.6f * H, hc + 1.45f * H, HR);
        break;
    case 25:   // shag
        sp.el(cx, hc - 0.05f * H, 2.4f * H, 2.3f * H, HR);
        sp.qd(cx - 1.2f * H, hc, cx + 1.2f * H, hc, cx + 1.25f * H, hc + 1.5f * H, cx - 1.25f * H, hc + 1.5f * H, HR);
        for (int i = 0; i < 5; i++) sp.tr(cx - 1.25f * H + i * 0.5f * H, hc + 1.45f * H, cx - 0.75f * H + i * 0.5f * H, hc + 1.45f * H, cx - 1.0f * H + i * 0.5f * H, hc + 1.85f * H, HR);
        break;
    case 26:   // half-up: long back
        sp.el(cx, hc - 0.05f * H, 2.3f * H, 2.2f * H, HR);
        sp.qd(cx - 1.12f * H, hc, cx + 1.12f * H, hc, cx + 1.2f * H, hc + 2.2f * H, cx - 1.2f * H, hc + 2.2f * H, HR);
        break;
    default: break;
    }
}

// Where the hairline sits (fraction of the head radius above the centre), or 9 = no hair cap.
static float hairlineOf(int st) {
    switch (st) {
    case 24: case 20: return 9.f;             // bald, mohawk (shaved sides)
    case 16: case 17: return -0.66f;          // fade, buzz
    case 5: case 10: case 19: case 27: case 18: return -0.62f;
    case 8: case 9: case 22: case 23: case 13: case 4: case 2: return -0.6f;
    default: return -0.58f;
    }
}
static float g_hl = -0.58f;
// the face: a skin-coloured oval laid over the hair cap, leaving a clean hairline
static void faceOval(SP& sp, float cx, float hc, float H, float hl, u32 SK, u32 INK) {
    const float top = hc + hl * H, bot = hc + 0.985f * H;
    sp.p.E(cx, (top + bot) * 0.5f, 1.86f * H, bot - top, SK);
    // hairline: a thin ink arc across the top of the face
    const float ry = (bot - top) * 0.5f, rx = 0.93f * H, ccy = (top + bot) * 0.5f;
    for (int i = 0; i < 8; i++) {
        float a0 = 3.14159f * (1.12f + i * 0.0975f), a1 = a0 + 3.14159f * 0.0975f;
        sp.dl(cx + std::cos(a0) * rx, ccy + std::sin(a0) * ry, cx + std::cos(a1) * rx, ccy + std::sin(a1) * ry, 0.07f * H, INK);
    }
}

static bool hairOnTop(int st) {   // pieces that merge with the hair cap (drawn before the face)
    switch (st) { case 4: case 13: case 22: case 23: case 5: case 18: case 15: case 20: case 27: case 9: return true; default: return false; }
}
static void hairFront(SP& sp, int st, float cx, float hc, float H, u32 HR, u32 HRD, float t, int layer) {
    if ((layer == 0) != hairOnTop(st) && st != 26) return;
    if (st == 26 && layer == 0) { sp.ci(cx + 0.1f * H, hc - 1.12f * H, 0.38f * H, HR); return; }
    switch (st) {
    case 0: case 26:   // sleek / half-up: side-swept fringe + face-framing strands
        sp.qd(cx - 0.95f * H, hc - 0.55f * H, cx + 0.3f * H, hc - 1.0f * H, cx + 0.1f * H, hc - 0.48f * H, cx - 0.9f * H, hc - 0.2f * H, HR);
        sp.qd(cx - 1.12f * H, hc - 0.3f * H, cx - 0.84f * H, hc - 0.3f * H, cx - 0.86f * H, hc + 0.95f * H, cx - 1.16f * H, hc + 1.0f * H, HR);
        sp.qd(cx + 0.84f * H, hc - 0.3f * H, cx + 1.12f * H, hc - 0.3f * H, cx + 1.16f * H, hc + 1.0f * H, cx + 0.86f * H, hc + 0.95f * H, HR);
        break;
    case 1:   // waves: middle part, curtains sweeping off the temples
        sp.tr(cx - 0.04f * H, hc - 0.62f * H, cx - 0.98f * H, hc - 0.55f * H, cx - 0.92f * H, hc + 0.05f * H, HR);
        sp.tr(cx + 0.04f * H, hc - 0.62f * H, cx + 0.98f * H, hc - 0.55f * H, cx + 0.92f * H, hc + 0.05f * H, HR);
        sp.qd(cx - 1.14f * H, hc - 0.35f * H, cx - 0.86f * H, hc - 0.2f * H, cx - 0.9f * H, hc + 0.95f * H, cx - 1.2f * H, hc + 1.0f * H, HR);
        sp.qd(cx + 0.86f * H, hc - 0.2f * H, cx + 1.14f * H, hc - 0.35f * H, cx + 1.2f * H, hc + 1.0f * H, cx + 0.9f * H, hc + 0.95f * H, HR);
        break;
    case 3:   // bob with blunt bangs, one piece
        sp.qd(cx - 0.92f * H, hc - 0.66f * H, cx + 0.92f * H, hc - 0.66f * H, cx + 0.86f * H, hc - 0.36f * H, cx - 0.86f * H, hc - 0.36f * H, HR);
        sp.qd(cx - 1.18f * H, hc - 0.6f * H, cx - 0.84f * H, hc - 0.6f * H, cx - 0.86f * H, hc + 0.92f * H, cx - 1.2f * H, hc + 0.92f * H, HR);
        sp.qd(cx + 0.84f * H, hc - 0.6f * H, cx + 1.18f * H, hc - 0.6f * H, cx + 1.2f * H, hc + 0.92f * H, cx + 0.86f * H, hc + 0.92f * H, HR);
        break;
    case 4: sp.ci(cx, hc - 1.22f * H, 0.5f * H, HR); break;   // top bun
    case 13: sp.ci(cx - 0.75f * H, hc - 1.02f * H, 0.42f * H, HR); sp.ci(cx + 0.75f * H, hc - 1.02f * H, 0.42f * H, HR); break;
    case 22: sp.ci(cx, hc - 1.42f * H, 0.78f * H, HR); break;   // high puff
    case 23: for (int i = 0; i < 5; i++) { float a = 3.14159f * (0.15f + i * 0.175f); sp.ci(cx - std::cos(a) * 0.95f * H, hc - 0.12f * H - std::sin(a) * 0.98f * H, 0.27f * H, HR); } break;
    case 5:   // short crop with a little texture
        sp.tr(cx - 0.7f * H, hc - 0.8f * H, cx - 0.1f * H, hc - 0.98f * H, cx - 0.5f * H, hc - 1.18f * H, HR);
        sp.tr(cx - 0.2f * H, hc - 0.98f * H, cx + 0.5f * H, hc - 0.9f * H, cx + 0.15f * H, hc - 1.22f * H, HR);
        break;
    case 6:   // curls: curly fringe
        for (int i = 0; i < 5; i++) sp.ci(cx - 0.72f * H + i * 0.36f * H, hc - 0.66f * H + (i % 2) * 0.06f * H, 0.24f * H, HR);
        break;
    case 7: case 11: sp.dl(cx, hc - 1.05f * H, cx, hc - 0.62f * H, 0.08f * H, HRD); break;   // centre part
    case 14: for (int i = 0; i < 4; i++) sp.dl(cx - 0.55f * H + i * 0.37f * H, hc - 1.0f * H, cx - 0.55f * H + i * 0.37f * H + 0.04f * H, hc - 0.62f * H, 0.08f * H, HRD); break;
    case 9: sp.ci(cx - 0.95f * H, hc - 0.95f * H, 0.55f * H, HR); sp.ci(cx + 0.95f * H, hc - 0.95f * H, 0.55f * H, HR); break;   // puffs
    case 10: sp.tr(cx - 0.95f * H, hc - 0.55f * H, cx + 0.6f * H, hc - 1.0f * H, cx - 0.62f * H, hc - 0.25f * H, HR); break;   // pixie sweep
    case 12:   // pigtails: centre part + bobbles
        sp.dl(cx, hc - 1.05f * H, cx, hc - 0.6f * H, 0.08f * H, HRD);
        sp.dot(cx - 1.05f * H, hc + 0.05f * H, 0.16f * H, C(HOT)); sp.dot(cx + 1.05f * H, hc + 0.05f * H, 0.16f * H, C(HOT));
        break;
    case 15:   // short twists sticking up and out
        for (int i = 0; i < 9; i++) {
            float a = 3.14159f * (0.08f + i * 0.105f), ex = cx - std::cos(a) * 1.42f * H, ey = hc - 0.25f * H - std::sin(a) * 1.15f * H;
            sp.ln(cx - std::cos(a) * 0.85f * H, hc - 0.15f * H - std::sin(a) * 0.8f * H, ex, ey, 0.26f * H, HR);
            sp.ci(ex, ey, 0.15f * H, HR);
        }
        break;
    case 18: for (int i = 0; i < 5; i++) sp.ci(cx - 0.72f * H + i * 0.36f * H, hc - 1.0f * H - (i % 2) * 0.14f * H, 0.32f * H, HR); break;   // curly top
    case 19:   // side part swoop
        sp.qd(cx - 0.4f * H, hc - 1.1f * H, cx + 1.06f * H, hc - 0.78f * H, cx + 0.95f * H, hc - 0.5f * H, cx - 0.2f * H, hc - 0.72f * H, HR);
        sp.dl(cx - 0.45f * H, hc - 1.02f * H, cx - 0.55f * H, hc - 0.66f * H, 0.07f * H, HRD);
        break;
    case 20:   // mohawk strip
        sp.rc(cx - 0.24f * H, hc - 1.06f * H, 0.48f * H, 0.45f * H, HR);
        for (int i = 0; i < 4; i++) sp.tr(cx - 0.24f * H, hc - 0.95f * H, cx + 0.24f * H, hc - 0.95f * H, cx + (i - 1.5f) * 0.22f * H, hc - 1.72f * H + std::fabs(i - 1.5f) * 0.22f * H, HR);
        break;
    case 21: sp.qd(cx - 0.98f * H, hc - 0.55f * H, cx + 0.85f * H, hc - 0.95f * H, cx + 0.5f * H, hc + 0.2f * H, cx + 0.08f * H, hc + 0.02f * H, HR); break;   // emo fringe
    case 25: for (int i = 0; i < 4; i++) sp.tr(cx - 0.92f * H + i * 0.46f * H, hc - 0.66f * H, cx - 0.46f * H + i * 0.46f * H, hc - 0.66f * H, cx - 0.69f * H + i * 0.46f * H, hc - 0.32f * H, HR); break;   // shag bangs
    case 27:   // spiky
        for (int i = 0; i < 6; i++) {
            float a = 3.14159f * (0.12f + i * 0.15f);
            float bx = cx - std::cos(a) * 0.88f * H, by = hc - 0.3f * H - std::sin(a) * 0.78f * H;
            sp.tr(bx - 0.26f * H, by + 0.12f * H, bx + 0.26f * H, by + 0.12f * H, cx - std::cos(a) * 1.55f * H, hc - 0.35f * H - std::sin(a) * 1.45f * H, HR);
        }
        break;
    case 24: sp.dl(cx - 0.45f * H, hc - 0.75f * H, cx - 0.15f * H, hc - 0.88f * H, 0.12f * H, C(255, 255, 255, 120)); break;   // bald shine
    default: break;
    }
    (void)t;
}

// ---------------------------------------------------------------- hats
static void drawHatS(SP& sp, int hat, float cx, float hc, float H, RGB c, float t) {
    const float top = hc - H;
    switch (hat) {
    case HT_STRAW: {
        u32 S = C(232, 198, 120), SD = C(196, 156, 84);
        sp.el(cx, top + 0.25f * H, 3.3f * H, 0.7f * H, S);
        sp.el(cx, top - 0.05f * H, 1.7f * H, 1.0f * H, S);
        sp.dl(cx - 0.82f * H, top + 0.15f * H, cx + 0.82f * H, top + 0.15f * H, 0.2f * H, C(c));
        sp.dl(cx - 1.4f * H, top + 0.3f * H, cx + 1.4f * H, top + 0.3f * H, 0.06f * H, SD);
        break;
    }
    case HT_CAP:
        sp.el(cx - 0.05f * H, top + 0.25f * H, 2.05f * H, 1.3f * H, C(c));
        sp.qd(cx + 0.6f * H, top + 0.35f * H, cx + 1.75f * H, top + 0.45f * H, cx + 1.7f * H, top + 0.65f * H, cx + 0.6f * H, top + 0.6f * H, C(shade(c, 0.8f)));
        sp.dot(cx - 0.05f * H, top - 0.38f * H, 0.12f * H, C(shade(c, 0.7f)));
        break;
    case HT_BEANIE:
        sp.el(cx, top + 0.2f * H, 2.2f * H, 1.6f * H, C(c));
        sp.rc(cx - 1.1f * H, top + 0.25f * H, 2.2f * H, 0.38f * H, C(shade(c, 0.82f)));
        sp.ci(cx, top - 0.65f * H, 0.3f * H, C(255, 255, 255));
        break;
    case HT_BUCKET:
        sp.qd(cx - 0.85f * H, top - 0.3f * H, cx + 0.85f * H, top - 0.3f * H, cx + 1.0f * H, top + 0.35f * H, cx - 1.0f * H, top + 0.35f * H, C(c));
        sp.qd(cx - 1.0f * H, top + 0.3f * H, cx + 1.0f * H, top + 0.3f * H, cx + 1.45f * H, top + 0.62f * H, cx - 1.45f * H, top + 0.62f * H, C(shade(c, 0.88f)));
        break;
    case HT_HEADBAND:
        sp.qd(cx - 1.0f * H, top + 0.45f * H, cx + 1.0f * H, top + 0.45f * H, cx + 0.95f * H, top + 0.65f * H, cx - 0.95f * H, top + 0.65f * H, C(c));
        break;
    case HT_BOW: {
        float bx = cx + 0.55f * H, by = top + 0.05f * H, s = 0.55f * H;
        sp.tr(bx, by, bx - s * 1.3f, by - s * 0.9f, bx - s * 1.3f, by + s * 0.8f, C(c));
        sp.tr(bx, by, bx + s * 1.3f, by - s * 0.9f, bx + s * 1.3f, by + s * 0.8f, C(c));
        sp.ci(bx, by, s * 0.35f, C(shade(c, 0.8f)));
        sp.dot(bx - s * 0.75f, by - s * 0.2f, s * 0.18f, C(255, 255, 255, 200)); sp.dot(bx + s * 0.75f, by - s * 0.2f, s * 0.18f, C(255, 255, 255, 200));
        break;
    }
    case HT_FLOWERS:
        for (int i = 0; i < 6; i++) {
            float a = 3.14159f * (0.12f + i * 0.152f);
            float fx = cx - std::cos(a) * 1.0f * H, fy = top + 0.65f * H - std::sin(a) * 0.65f * H;
            static const RGB FC[3] = {{255, 140, 190}, {255, 230, 120}, {190, 160, 255}};
            sp.ci(fx, fy, 0.26f * H, C(FC[i % 3]));
            sp.dot(fx, fy, 0.09f * H, C(255, 250, 230));
        }
        break;
    case HT_BANDANA:
        sp.el(cx, top + 0.35f * H, 2.15f * H, 1.15f * H, C(c));
        sp.tr(cx - 1.0f * H, top + 0.5f * H, cx - 1.6f * H, top + 0.75f * H, cx - 1.35f * H, top + 0.25f * H, C(c));
        for (int i = 0; i < 3; i++) sp.dot(cx - 0.6f * H + i * 0.6f * H, top + 0.2f * H, 0.08f * H, C(255, 255, 255));
        break;
    case HT_HEADWRAP:
        sp.el(cx, top - 0.25f * H, 2.3f * H, 1.8f * H, C(c));
        sp.el(cx + 0.3f * H, top - 0.85f * H, 1.4f * H, 1.1f * H, C(shade(c, 1.15f)));
        sp.dl(cx - 0.9f * H, top + 0.2f * H, cx + 0.6f * H, top - 0.9f * H, 0.08f * H, C(shade(c, 0.7f)));
        break;
    case HT_TIARA:
        sp.qd(cx - 0.75f * H, top + 0.32f * H, cx + 0.75f * H, top + 0.32f * H, cx + 0.7f * H, top + 0.45f * H, cx - 0.7f * H, top + 0.45f * H, C(230, 230, 245));
        sp.tr(cx - 0.25f * H, top + 0.33f * H, cx + 0.25f * H, top + 0.33f * H, cx, top - 0.25f * H, C(230, 230, 245));
        sp.dot(cx, top + 0.1f * H, 0.1f * H, C(HOT));
        break;
    case HT_COWBOY:
        sp.qd(cx - 1.8f * H, top + 0.35f * H, cx + 1.8f * H, top + 0.35f * H, cx + 1.4f * H, top + 0.6f * H, cx - 1.4f * H, top + 0.6f * H, C(c));
        sp.qd(cx - 0.8f * H, top - 0.55f * H, cx + 0.8f * H, top - 0.55f * H, cx + 0.95f * H, top + 0.4f * H, cx - 0.95f * H, top + 0.4f * H, C(c));
        sp.dl(cx - 0.9f * H, top + 0.25f * H, cx + 0.9f * H, top + 0.25f * H, 0.15f * H, C(shade(c, 0.6f)));
        break;
    }
    (void)t;
}
static void holidayHat(SP& sp, int holiday, float cx, float hc, float H) {
    const float top = hc - H;
    if (holiday == 1) {   // party hat
        sp.tr(cx - 0.55f * H, top + 0.2f * H, cx + 0.55f * H, top + 0.2f * H, cx + 0.1f * H, top - 1.2f * H, C(HOT));
        sp.ci(cx + 0.1f * H, top - 1.2f * H, 0.2f * H, C(GOLD));
        sp.dl(cx - 0.3f * H, top - 0.3f * H, cx + 0.35f * H, top - 0.45f * H, 0.1f * H, C(GOLD));
    } else if (holiday == 2) {   // witch hat
        sp.el(cx, top + 0.25f * H, 2.8f * H, 0.5f * H, C(40, 30, 50));
        sp.tr(cx - 0.7f * H, top + 0.25f * H, cx + 0.7f * H, top + 0.25f * H, cx + 0.5f * H, top - 1.5f * H, C(40, 30, 50));
        sp.dl(cx - 0.6f * H, top + 0.05f * H, cx + 0.62f * H, top + 0.05f * H, 0.18f * H, C(PURPLE));
    } else if (holiday == 3) {   // santa hat
        sp.qd(cx - 0.95f * H, top + 0.25f * H, cx + 0.95f * H, top + 0.25f * H, cx + 1.25f * H, top - 0.2f * H, cx + 0.1f * H, top - 0.9f * H, C(RED));
        sp.ci(cx + 1.3f * H, top - 0.2f * H, 0.26f * H, C(WHITE));
        sp.rc(cx - 1.05f * H, top + 0.12f * H, 2.1f * H, 0.32f * H, C(WHITE));
    }
}

// ---------------------------------------------------------------- face
static void drawFace(SP& sp, const PersonDef& d, float cx, float hc, float H, int mood, bool blink, float lookX, bool doodle) {
    if (mood == 8) mood = 0;      // confident: the stick smile
    if (mood == 9) mood = 5;      // embarrassed: wide-eyed
    const FaceDef& f = d.f;
    const u32 INK = sp.inkc;
    const float ey = hc - 0.05f * H, ex = 0.36f * H, ox = lookX * 0.1f * H;
    const bool closed = blink || mood == 7;
    const RGB hair = hairRGB(d.hairCol);
    const u32 BROW = C(shade(hair, d.hairCol >= 3 && d.hairCol != 5 ? 0.5f : 0.85f));
    // blush first (under everything)
    static const RGB BL[5] = {{255, 140, 170}, {255, 120, 160}, {255, 160, 130}, {220, 80, 130}, {200, 120, 80}};
    if (f.blush) {   // blush is opt-in (Create-a-Sim > Glam)
        RGB b = blushRGB(f.blush);
        uint8_t a = d.guy ? 90 : 140;
        sp.dot(cx - ex - 0.12f * H + ox, ey + 0.32f * H, 0.2f * H, C(b, a)); sp.dot(cx + ex + 0.12f * H + ox, ey + 0.32f * H, 0.2f * H, C(b, a));
    }
    if (f.marks == 1 || f.marks == 3) for (int i = 0; i < 3; i++) { sp.dot(cx - ex + ox - 0.1f * H + i * 0.1f * H, ey + 0.24f * H + (i % 2) * 0.06f * H, 0.035f * H, C(150, 90, 60)); sp.dot(cx + ex + ox - 0.1f * H + i * 0.1f * H, ey + 0.24f * H + (i % 2) * 0.06f * H, 0.035f * H, C(150, 90, 60)); }
    if (f.marks >= 2) sp.dot(cx + ex + ox + 0.18f * H, ey + 0.48f * H, 0.045f * H, C(70, 40, 30));
    // eyes
    const bool cutie = g_artStyle == 11;
    const bool bigEyes = cutie || f.eyes == 1 || f.eyes == 3;
    for (int s = -1; s <= 1; s += 2) {
        float x = cx + s * ex + ox;
        if (mood == 4 && s == 1 && !blink) {   // a wink
            sp.dl(x - 0.12f * H, ey, x + 0.12f * H, ey - 0.06f * H, 0.07f * H, INK);
            continue;
        }
        if (closed) {
            sp.dl(x - 0.13f * H, ey + 0.02f * H, x, ey + 0.08f * H, 0.07f * H, INK); sp.dl(x, ey + 0.08f * H, x + 0.13f * H, ey + 0.02f * H, 0.07f * H, INK);
        } else if (mood == 1 && !doodle) {   // happy ^ ^ eyes
            sp.dl(x - 0.13f * H, ey + 0.05f * H, x, ey - 0.08f * H, 0.08f * H, INK); sp.dl(x, ey - 0.08f * H, x + 0.13f * H, ey + 0.05f * H, 0.08f * H, INK);
        } else {
            float w = cutie ? 0.27f * H : bigEyes ? 0.24f * H : 0.17f * H, h = cutie ? 0.34f * H : bigEyes ? 0.32f * H : 0.26f * H;
            if (mood == 5) { w *= 1.15f; h *= 1.15f; }
            sp.p.E(x, ey, w, h, INK);   // (fill pass only - the eyes are ink already)
            if (!doodle) {
                static const RGB IR[7] = {{112, 70, 40}, {58, 36, 28}, {142, 112, 54}, {70, 142, 92}, {72, 124, 204}, {128, 140, 152}, {146, 92, 186}};
                if (cutie) { sp.p.E(x, ey + 0.05f * H, w * 0.7f, h * 0.6f, C(eyeRGB(f.eyeCol))); sp.p.E(x, ey + 0.1f * H, w * 0.45f, h * 0.25f, C(shade(eyeRGB(f.eyeCol), 1.35f))); }
                else if (bigEyes && f.eyeCol >= 2) sp.p.E(x, ey + 0.04f * H, w * 0.62f, h * 0.55f, C(eyeRGB(f.eyeCol)));
                sp.p.Ci(x + 0.04f * H + lookX * 0.02f * H, ey - 0.07f * H, (cutie ? 0.07f : 0.055f) * H, C(WHITE));
                if (cutie) sp.p.Ci(x - 0.05f * H, ey + 0.08f * H, 0.03f * H, C(WHITE));
            }
            if (f.liner || (!d.guy && f.eyes != 5)) {   // lashes flick outward
                sp.dl(x + s * w * 0.4f, ey - h * 0.3f, x + s * (w * 0.4f + 0.11f * H), ey - h * 0.55f, 0.06f * H, INK);
                if (f.liner == 2) sp.dl(x + s * w * 0.2f, ey - h * 0.45f, x + s * (w * 0.2f + 0.08f * H), ey - h * 0.75f, 0.05f * H, INK);
            }
        }
        // brows
        float by = ey - 0.36f * H - (mood == 5 ? 0.06f * H : 0);
        float inner = mood == 3 ? 0.08f * H : mood == 2 ? -0.07f * H : 0.f;
        float thk = (f.brows == 2 || f.brows == 4) ? 0.1f * H : 0.065f * H;
        if (f.brows != 1) sp.dl(x - s * 0.14f * H, by + inner, x + s * 0.14f * H, by - 0.04f * H, thk, BROW);
        else sp.dl(x - s * 0.14f * H, by + inner * 0.5f, x + s * 0.14f * H, by, thk, BROW);
    }
    // nose
    if (f.nose == 1) sp.dot(cx + ox * 1.6f, ey + 0.2f * H, 0.045f * H, C(0, 0, 0, 120));
    else if (f.nose == 2) sp.dl(cx + ox * 1.6f, ey + 0.12f * H, cx + ox * 1.6f + 0.06f * H, ey + 0.25f * H, 0.05f * H, C(0, 0, 0, 110));
    else if (f.nose == 3) { sp.dl(cx + ox * 1.6f - 0.06f * H, ey + 0.24f * H, cx + ox * 1.6f + 0.06f * H, ey + 0.24f * H, 0.05f * H, C(0, 0, 0, 110)); }
    // mouth
    const float my = hc + 0.4f * H, mx = cx + ox * 1.3f;
    const RGB lip = d.guy ? RGB{120, 40, 50} : (f.shadow || f.liner ? d.lips : RGB{140, 50, 70});
    switch (mood) {
    case 1: case 5: {   // grin / surprised: open mouth
        if (mood == 5) { sp.p.E(mx, my + 0.02f * H, 0.2f * H, 0.24f * H, INK); break; }
        sp.p.Q(mx - 0.22f * H, my - 0.04f * H, mx + 0.22f * H, my - 0.04f * H, mx + 0.12f * H, my + 0.2f * H, mx - 0.12f * H, my + 0.2f * H, INK);
        sp.p.E(mx, my + 0.13f * H, 0.16f * H, 0.08f * H, C(255, 120, 140));
        break;
    }
    case 2: case 3:   // sad / angry frown
        sp.dl(mx - 0.15f * H, my + 0.08f * H, mx, my, 0.07f * H, INK); sp.dl(mx, my, mx + 0.15f * H, my + 0.08f * H, 0.07f * H, INK);
        if (mood == 2 && !doodle) sp.dot(cx + ex + ox + 0.05f * H, ey + 0.22f * H, 0.06f * H, C(120, 190, 255, 220));   // a tear
        break;
    case 6:   // bored
        sp.dl(mx - 0.13f * H, my + 0.04f * H, mx + 0.13f * H, my + 0.04f * H, 0.07f * H, INK);
        break;
    default:   // the classic little smile
        sp.dl(mx - 0.17f * H, my - 0.02f * H, mx - 0.07f * H, my + 0.06f * H, 0.07f * H, INK);
        sp.dl(mx - 0.07f * H, my + 0.06f * H, mx + 0.07f * H, my + 0.06f * H, 0.07f * H, INK);
        sp.dl(mx + 0.07f * H, my + 0.06f * H, mx + 0.17f * H, my - 0.02f * H, 0.07f * H, INK);
        if (!d.guy && !doodle && (f.shadow || f.liner)) sp.dot(mx, my + 0.07f * H, 0.06f * H, C(lip, 200));
        break;
    }
    // facial hair
    if (d.guy && f.beard) {
        u32 BR = C(shade(hair, 0.9f), f.beard == 1 ? 110 : 255);
        if (f.beard == 1) for (int i = 0; i < 7; i++) sp.dot(cx - 0.4f * H + i * 0.13f * H, hc + 0.62f * H + (i % 2) * 0.06f * H, 0.035f * H, BR);
        else if (f.beard == 2) sp.p.Q(mx - 0.12f * H, my + 0.14f * H, mx + 0.12f * H, my + 0.14f * H, mx + 0.08f * H, my + 0.34f * H, mx - 0.08f * H, my + 0.34f * H, BR);
        else { sp.p.Q(cx - 0.8f * H, hc + 0.25f * H, cx + 0.8f * H, hc + 0.25f * H, cx + 0.45f * H, hc + 0.95f * H, cx - 0.45f * H, hc + 0.95f * H, BR); sp.p.E(mx, my + 0.03f * H, 0.3f * H, 0.1f * H, C(skinRGB(d.skin))); sp.dl(mx - 0.1f * H, my + 0.03f * H, mx + 0.1f * H, my + 0.03f * H, 0.06f * H, INK); }
    }
    // glasses
    if (f.glasses) {
        u32 GL = f.glasses == 4 ? C(200, 200, 210) : f.glasses == 2 ? C(HOT) : INK;
        float r = 0.2f * H;
        for (int s = -1; s <= 1; s += 2) {
            float x = cx + s * ex + ox;
            if (f.glasses == 3) { sp.dl(x - r, ey - r * 0.8f, x + r, ey - r * 0.8f, 0.05f * H, GL); sp.dl(x - r, ey + r * 0.8f, x + r, ey + r * 0.8f, 0.05f * H, GL); sp.dl(x - r, ey - r * 0.8f, x - r, ey + r * 0.8f, 0.05f * H, GL); sp.dl(x + r, ey - r * 0.8f, x + r, ey + r * 0.8f, 0.05f * H, GL); }
            else for (int i = 0; i < 10; i++) { float a0 = i * 0.628f, a1 = a0 + 0.628f; sp.dl(x + std::cos(a0) * r, ey + std::sin(a0) * r, x + std::cos(a1) * r, ey + std::sin(a1) * r, 0.05f * H, GL); }
        }
        sp.dl(cx - ex + ox + r, ey, cx + ex + ox - r, ey, 0.05f * H, GL);
    }
}

// ---------------------------------------------------------------- the figure
static void drawFigure(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip, float bustCut) {
    const int style = g_artStyle;
    const bool doodle = style == 8;
    const bool sitSleep = pose == PO_SITSLEEP;
    if (sitSleep) pose = PO_SLEEP;
    if (pose == PO_LIE) pose = PO_IDLE;
    const bool sitting = pose == PO_SIT || sitSleep;
    const bool walking = pose == PO_WALK;
    const Skel k = skelFor(d.age, d.guy, style);
    const float ph = g_walkPhase * 6.2832f;
    float bob = pose == PO_DANCE ? -std::fabs(std::sin(t * 7.f)) * 3.f : walking ? -std::fabs(std::sin(ph)) * 1.4f : std::sin(t * 1.7f + d.skin) * 0.5f;
    if (sitting || bustCut < 1e8f) bob = 0;
    Pen pen(x, (sitting ? footY + (120 - k.hip) * s - 4 * s : footY) + bob * s, 40, 120, s, flip);
    const u32 INK = doodle ? C(28, 24, 30) : C(34, 26, 44);
    SP sp(pen, INK, (doodle ? 0.75f : 1.05f) / std::fmax(0.3f, s));
    sp.cutY = bustCut;
    const Wear w = resolveWear(d, outfit);
    const RGB skin = skinRGB(d.skin), hair = hairRGB(d.hairCol);
    const u32 SK = C(skin), HR = C(hair), HRD = C(shade(hair, 0.7f));
    const u32 LIMB = doodle ? INK : SK;
    const float cx = 20, H = k.H, hc = k.hc, sh = k.sh, hip = k.hip;
    const float lt = k.limb;
    const int mood = pose == PO_SLEEP ? 7 : pose == PO_DANCE && g_stickMood == 0 ? 1 : g_stickMood;
    const bool blink = std::fmod(t * 0.9f + d.skin * 0.37f + d.hairCol * 0.19f, 3.9f) < 0.12f;
    const int hs = d.hairStyle % NUM_HAIRSTYLES;
    const bool kid = d.age == KID;

    // ---- pose: joints
    const float shL = cx - k.shW, shR = cx + k.shW, shY = sh + 1.5f;
    const float armLen = k.T * 0.92f;
    float eLx = shL - 2.2f, eLy = shY + armLen * 0.5f, hLx = shL - 2.6f, hLy = shY + armLen;
    float eRx = shR + 2.2f, eRy = shY + armLen * 0.5f, hRx = shR + 2.6f, hRy = shY + armLen;
    if (walking) {
        float sw = std::sin(ph) * armLen * 0.32f;
        hLx += sw; eLx += sw * 0.45f; hRx -= sw; eRx -= sw * 0.45f;
        hLy -= std::fabs(sw) * 0.2f; hRy -= std::fabs(sw) * 0.2f;
    } else if (pose == PO_WAVE) {
        float wv = std::sin(t * 9.f) * 2.6f;
        eRx = shR + 5.5f; eRy = shY - 2.f; hRx = shR + 7.f + wv; hRy = shY - armLen * 0.45f;
    } else if (pose == PO_DANCE) {
        float a = std::sin(t * 7.f);
        eLx = shL - 5.5f; eLy = shY - 1 + a * 2; hLx = shL - 6.5f; hLy = shY - armLen * 0.45f + a * 4;
        eRx = shR + 5.5f; eRy = shY - 1 - a * 2; hRx = shR + 6.5f; hRy = shY - armLen * 0.45f - a * 4;
    } else if (pose == PO_PHONE) {
        eRx = shR + 4.5f; eRy = shY + armLen * 0.42f; hRx = cx + H * 0.55f; hRy = hc + H * 0.55f;
    } else if (sitting) {
        eLx = shL - 2.f; eLy = shY + armLen * 0.48f; hLx = cx - 3.5f; hLy = hip + 1.5f;
        eRx = shR + 2.f; eRy = shY + armLen * 0.48f; hRx = cx + 3.5f; hRy = hip + 1.5f;
    }
    // legs
    const float hjL = cx - 3.0f, hjR = cx + 3.0f;
    float kLx = hjL - 0.6f, kLy = hip + k.L * 0.5f, fLx = hjL - 1.0f, fLy = 118.f;
    float kRx = hjR + 0.6f, kRy = hip + k.L * 0.5f, fRx = hjR + 1.0f, fRy = 118.f;
    if (walking) {
        float sw = std::sin(ph), liftL = std::fmax(0.f, sw) * 4.f, liftR = std::fmax(0.f, -sw) * 4.f;
        fLx = hjL + sw * 2.6f; fLy = 118.f - liftL; fRx = hjR - sw * 2.6f; fRy = 118.f - liftR;
        kLx = (hjL + fLx) / 2 + liftL * 0.6f; kLy = (hip + fLy) / 2 - liftL * 0.3f; kRx = (hjR + fRx) / 2 + liftR * 0.6f; kRy = (hip + fRy) / 2 - liftR * 0.3f;
    } else if (pose == PO_DANCE) {
        float a = std::sin(t * 7.f) * 1.5f;
        kLx = hjL - 2.5f; kLy = hip + k.L * 0.5f + a; kRx = hjR + 2.5f; kRy = hip + k.L * 0.5f - a;
        fLx = hjL - 3.f; fRx = hjR + 3.f;
    } else if (sitting) {   // thighs toward us (short), shins straight down
        kLx = hjL - 0.8f; kLy = hip + 5.f; fLx = hjL - 0.8f; fLy = hip + 5.f + k.L * 0.45f;
        kRx = hjR + 0.8f; kRy = hip + 5.f; fRx = hjR + 0.8f; fRy = hip + 5.f + k.L * 0.45f;
    } else if (pose == PO_IDLE || pose == PO_PHONE || pose == PO_SLEEP) {
        fLx = hjL - 1.6f; fRx = hjR + 1.6f;
    }
    const bool bust = bustCut < 1e8f;

    // ---- back hair, hood
    grp(sp, [&] {
        hairBack(sp, hs, cx, hc, H, HR, t);
        if (w.ts == TS_HOODIE || w.ts == TS_RAINCOAT) sp.el(cx, sh - 1.5f, 2.0f * H, 0.8f * H, C(shade(w.top, 0.85f)));
    });
    // ---- legs
    const bool dress = w.ts == TS_DRESS;
    const int bt = w.bt;
    const bool longBottom = !dress && (bt == BT_PANTS || bt == BT_JEANS || bt == BT_SWEATS || bt == BT_OVERALLS);
    if (!bust) {
        const bool cover = longBottom || bt == BT_GOWN || bt == BT_MAXI;
        if (!cover || walking) {
            grp(sp, [&] { sp.limb(hjL, hip, kLx, kLy, fLx, fLy, lt, LIMB); });
            grp(sp, [&] { sp.limb(hjR, hip, kRx, kRy, fRx, fRy, lt, LIMB); });
        }
        // bottoms
        const RGB B = w.bot;
        const u32 BC = C(B), BCD = C(shade(B, 0.82f));
        auto legCloth = [&](float frac, float th) {   // pants legs follow the leg joints down to `frac` of the leg
            auto seg = [&](float x0, float y0, float x1, float y1, float x2, float y2) {
                float len1 = std::hypot(x1 - x0, y1 - y0), len2 = std::hypot(x2 - x1, y2 - y1), want = (len1 + len2) * frac;
                if (want <= len1) { float u = want / len1; sp.ln(x0, y0, x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, th, BC); }
                else { sp.ln(x0, y0, x1, y1, th, BC); float u = (want - len1) / len2; sp.ln(x1, y1, x1 + (x2 - x1) * u, y1 + (y2 - y1) * u, th, BC); sp.ci(x1, y1, th / 2, BC); }
            };
            seg(hjL, hip, kLx, kLy, fLx, fLy); seg(hjR, hip, kRx, kRy, fRx, fRy);
            sp.qd(cx - k.hipW - 0.5f, hip - 2.5f, cx + k.hipW + 0.5f, hip - 2.5f, cx + k.hipW, hip + 5, cx - k.hipW, hip + 5, BC);
        };
        const float lw = lt + (kid ? 3.2f : 3.6f);
        grp(sp, [&] {
            if (dress) return;
            switch (bt) {
            case BT_PANTS: case BT_JEANS: case BT_OVERALLS: legCloth(0.9f, lw); break;
            case BT_SWEATS: legCloth(0.88f, lw + 1.2f); break;
            case BT_SHORTS: legCloth(0.4f, lw + 0.6f); break;
            case BT_SWIM: legCloth(d.guy ? 0.32f : 0.14f, lw + 0.6f); break;
            case BT_MINI: sp.qd(cx - k.hipW - 0.5f, hip - 2.5f, cx + k.hipW + 0.5f, hip - 2.5f, cx + k.hipW + 4.f, hip + k.L * 0.25f, cx - k.hipW - 4.f, hip + k.L * 0.25f, BC); break;
            case BT_PLEATED: sp.qd(cx - k.hipW - 0.5f, hip - 2.5f, cx + k.hipW + 0.5f, hip - 2.5f, cx + k.hipW + 5.f, hip + k.L * 0.32f, cx - k.hipW - 5.f, hip + k.L * 0.32f, BC); break;
            case BT_MAXI: sp.qd(cx - k.hipW - 0.5f, hip - 2.5f, cx + k.hipW + 0.5f, hip - 2.5f, cx + k.hipW + 6.f, 116.f, cx - k.hipW - 6.f, 116.f, BC); break;
            case BT_GOWN:
                sp.qd(cx - k.hipW - 0.5f, hip - 2.5f, cx + k.hipW + 0.5f, hip - 2.5f, cx + k.hipW + 9.f, 120.f, cx - k.hipW - 9.f, 120.f, BC);
                break;
            }
        });
        // bottom details
        if (!dress) {
            if (bt == BT_JEANS) { sp.dl(cx - k.hipW + 1, hip, cx - k.hipW + 2.5f, hip + 3, 0.5f, C(255, 230, 160, 160)); sp.dl(cx + k.hipW - 1, hip, cx + k.hipW - 2.5f, hip + 3, 0.5f, C(255, 230, 160, 160)); }
            if (bt == BT_SWEATS) { sp.dl(hjL - lw * 0.45f, hip + 2, kLx - lw * 0.45f, kLy, 0.7f, C(255, 255, 255, 200)); sp.dl(hjR + lw * 0.45f, hip + 2, kRx + lw * 0.45f, kRy, 0.7f, C(255, 255, 255, 200)); }
            if (bt == BT_PLEATED) for (int i = -1; i <= 1; i++) sp.dl(cx + i * 3.5f, hip, cx + i * 5.f, hip + k.L * 0.3f, 0.5f, BCD);
            if (bt == BT_GOWN) for (int i = 0; i < 5; i++) sp.dot(cx - 5 + (i * 7) % 11, hip + 12 + i * 8.f, 0.6f, C(255, 255, 255, 200));
            if (bt == BT_SWIM && d.guy) for (int i = 0; i < 2; i++) sp.dl(cx - k.hipW, hip + 2 + i * 3.f, cx + k.hipW, hip + 2 + i * 3.f, 0.6f, C(255, 255, 255, 150));
        }
        // shoes
        const u32 SH = C(w.shoes), SHD = C(shade(w.shoes, 0.75f));
        const float tw = sitting ? 0.f : 1.f;   // toes point toward the facing side
        auto shoe = [&](float fx, float fy) {
            switch (w.ss) {
            case SS_SNEAKERS: sp.el(fx + 1.4f * tw, fy - 0.5f, 7.f + kid, 4.2f, SH); break;
            case SS_SANDALS: case SS_FLIPFLOPS: sp.el(fx + 1.2f * tw, fy + 0.8f, 7.f, 2.0f, SH); break;
            case SS_BOOTS: sp.rc(fx - 2.6f, fy - 9.f, 5.2f, 9.f, SH); sp.el(fx + 1.2f * tw, fy - 0.5f, 7.f, 4.f, SH); break;
            case SS_HEELS: sp.tr(fx - 2.f, fy - 2.5f, fx + 3.6f * tw + 0.5f, fy + 0.6f, fx - 2.f, fy + 0.6f, SH); sp.ln(fx - 2.f, fy - 1.f, fx - 2.f, fy + 1.5f, 1.f, SH); break;
            case SS_PLATFORMS: sp.rc(fx - 2.8f, fy - 0.2f, 6.f + tw, 3.2f, C(shade(w.shoes, 0.85f))); sp.el(fx + 0.8f * tw, fy - 1.2f, 6.4f, 3.6f, SH); break;
            case SS_LOAFERS: sp.el(fx + 1.2f * tw, fy - 0.2f, 6.6f, 3.2f, SH); break;
            case SS_SLIPPERS: sp.el(fx + 1.2f * tw, fy - 0.8f, 8.f, 5.f, SH); break;
            case SS_BAREFOOT: sp.el(fx + 1.0f * tw, fy - 0.2f, 5.f, 2.6f, SK); break;
            }
        };
        grp(sp, [&] { shoe(fLx, fLy); });
        grp(sp, [&] { shoe(fRx, fRy); });
        if (w.ss == SS_SNEAKERS) { sp.dl(fLx - 2.2f + 1.4f * tw, fLy + 1.1f, fLx + 3.8f * tw + 0.8f, fLy + 1.1f, 0.8f, C(255, 255, 255)); sp.dl(fRx - 2.2f + 1.4f * tw, fRy + 1.1f, fRx + 3.8f * tw + 0.8f, fRy + 1.1f, 0.8f, C(255, 255, 255)); }
        if (w.ss == SS_SANDALS || w.ss == SS_FLIPFLOPS) { sp.dl(fLx - 0.5f, fLy - 1.2f, fLx + 2.4f * tw, fLy + 0.3f, 0.9f, SHD); sp.dl(fRx - 0.5f, fRy - 1.2f, fRx + 2.4f * tw, fRy + 0.3f, 0.9f, SHD); }
        if (w.acc & ACC_LEGWARM) { grp(sp, [&] { sp.ln(kLx, kLy + 8, fLx, fLy - 3, lt + 3.5f, C(w.accCol)); sp.ln(kRx, kRy + 8, fRx, fRy - 3, lt + 3.5f, C(w.accCol)); }); }
    }

    // ---- torso (the top)
    const RGB TC = w.top;
    const u32 T = C(TC), TD = C(shade(TC, 0.8f)), TL = C(shade(TC, 1.3f));
    const bool longSleeve = w.ts == TS_HOODIE || w.ts == TS_SWEATER || w.ts == TS_JACKET || w.ts == TS_BLAZER || w.ts == TS_CARDIGAN || w.ts == TS_RAINCOAT;
    const bool noSleeve = w.ts == TS_TANK || w.ts == TS_TUBE || w.ts == TS_JERSEY || w.ts == TS_DRESS || w.ts == TS_CROP;
    const float baggy = (w.ts == TS_HOODIE || w.ts == TS_RAINCOAT || w.ts == TS_JERSEY) ? 1.4f : 0.f;
    const float tTop = (w.ts == TS_TANK || w.ts == TS_TUBE) ? sh + 2.5f : sh - 0.5f;
    float tBot = hip + 1.f;
    if (w.ts == TS_CROP) tBot = sh + k.T * 0.62f;
    if (w.ts == TS_RAINCOAT) tBot = hip + k.L * 0.3f;
    const float waist = d.guy ? k.hipW + 0.3f : k.hipW - 0.4f;
    if (w.ts == TS_CROP && !bust) grp(sp, [&] { sp.qd(cx - waist + 0.8f, tBot - 1, cx + waist - 0.8f, tBot - 1, cx + k.hipW - 0.2f, hip, cx - k.hipW + 0.2f, hip, SK); });
    grp(sp, [&] {
        if (dress) {   // sundress: bodice + flared skirt to the knee
            sp.qd(cx - k.shW + 1.2f, tTop + 2, cx + k.shW - 1.2f, tTop + 2, cx + waist, hip - 3, cx - waist, hip - 3, T);
            float hem = bust ? hip : hip + k.L * (d.age == KID ? 0.4f : 0.45f);
            sp.qd(cx - waist, hip - 4, cx + waist, hip - 4, cx + waist + 7, hem, cx - waist - 7, hem, T);
        } else {
            sp.qd(cx - k.shW - baggy, tTop, cx + k.shW + baggy, tTop, cx + waist + baggy, tBot, cx - waist - baggy, tBot, T);
            if (w.ts == TS_RAINCOAT) sp.qd(cx - waist - baggy, hip - 2, cx + waist + baggy, hip - 2, cx + waist + baggy + 2.5f, tBot, cx - waist - baggy - 2.5f, tBot, T);
        }
        if (!noSleeve) {   // sleeves hug the upper arm (and the forearm for long sleeves)
            float st = lt + 3.8f + baggy;
            auto sleeve = [&](float sx, float ex2, float ey2, float hx, float hy) {
                if (longSleeve) { sp.ln(sx, shY, ex2, ey2, st, T); sp.ci(ex2, ey2, st / 2, T); sp.ln(ex2, ey2, hx + (ex2 - hx) * 0.12f, hy + (ey2 - hy) * 0.12f, st - 0.6f, T); }
                else { float u = w.ts == TS_BUTTON ? 0.95f : 0.55f; sp.ln(sx, shY, sx + (ex2 - sx) * u, shY + (ey2 - shY) * u, st, T); }
                sp.ci(sx, shY + 0.5f, st / 2, T);
            };
            sleeve(shL, eLx, eLy, hLx, hLy);
            sleeve(shR, eRx, eRy, hRx, hRy);
        }
    });
    // top details
    if (w.ts == TS_TANK || w.ts == TS_JERSEY || w.ts == TS_DRESS) { sp.dl(cx - 4.2f, tTop + 0.5f, cx - 3.f, sh - 1.5f, 1.4f, T); sp.dl(cx + 4.2f, tTop + 0.5f, cx + 3.f, sh - 1.5f, 1.4f, T); }
    if (w.ts == TS_HOODIE) { sp.dl(cx - 1.2f, sh + 1, cx - 1.5f, sh + 8, 0.5f, C(255, 255, 255)); sp.dl(cx + 1.2f, sh + 1, cx + 1.5f, sh + 8, 0.5f, C(255, 255, 255)); sp.p.Q(cx - 4.5f, hip - 8, cx + 4.5f, hip - 8, cx + 5.5f, hip - 2, cx - 5.5f, hip - 2, TD); }
    if (w.ts == TS_JERSEY && !g_ink && s >= 0.6f && !bust) { sp.dl(cx - 2.6f, sh + 7, cx - 2.6f, sh + 15, 1.6f, C(255, 255, 255)); sp.dl(cx + 0.4f, sh + 7, cx + 3.2f, sh + 7, 1.4f, C(255, 255, 255)); sp.dl(cx + 3.2f, sh + 7, cx + 0.6f, sh + 15, 1.4f, C(255, 255, 255)); sp.dl(cx - k.shW, tBot - 2, cx + k.shW, tBot - 2, 1.f, TL); }
    if (w.ts == TS_BUTTON || w.ts == TS_POLO) {
        sp.p.T(cx - 3.2f, sh - 0.8f, cx, sh + 2.4f, cx - 1.2f, sh - 1.2f, C(255, 255, 255)); sp.p.T(cx + 3.2f, sh - 0.8f, cx, sh + 2.4f, cx + 1.2f, sh - 1.2f, C(255, 255, 255));
        for (int i = 0; i < (w.ts == TS_POLO ? 2 : 4); i++) sp.dot(cx, sh + 4 + i * 5.5f, 0.55f, TD);
    }
    if (w.ts == TS_SWEATER) { sp.dl(cx - waist, tBot - 1.5f, cx + waist, tBot - 1.5f, 1.4f, TD); sp.dl(cx - 3, sh + 0.2f, cx + 3, sh + 0.2f, 1.2f, TD); }
    if (w.ts == TS_JACKET || w.ts == TS_BLAZER || w.ts == TS_CARDIGAN) {
        RGB inner = w.ts == TS_BLAZER ? RGB{255, 255, 255} : w.ts == TS_JACKET ? RGB{255, 255, 255} : RGB{250, 236, 216};
        sp.p.Q(cx - 2.6f, sh, cx + 2.6f, sh, cx + 2.2f, tBot - 0.8f, cx - 2.2f, tBot - 0.8f, C(inner));
        sp.dl(cx - 2.6f, sh, cx - 0.6f, sh + 9, 0.8f, TD); sp.dl(cx + 2.6f, sh, cx + 0.6f, sh + 9, 0.8f, TD);
        if (w.ts == TS_BLAZER && d.guy) { sp.p.Q(cx - 0.8f, sh + 1, cx + 0.8f, sh + 1, cx + 1.1f, sh + 12, cx - 1.1f, sh + 12, C(HOT)); }
        if (w.ts == TS_JACKET) for (int i = 0; i < 2; i++) sp.dot(cx + (i ? 4.2f : -4.2f), sh + 6, 0.7f, TL);
    }
    if (w.ts == TS_RAINCOAT) for (int i = 0; i < 4; i++) sp.dot(cx + 1.2f, sh + 4 + i * 6.f, 0.7f, TD);
    if (w.ts == TS_CROP || w.ts == TS_TUBE) sp.dl(cx - waist - 0.5f, tBot - 1, cx + waist + 0.5f, tBot - 1, 0.6f, TL);
    if (dress && !bust) sp.dl(cx - waist, hip - 3.5f, cx + waist, hip - 3.5f, 1.6f, C(w.bot));
    // prints on the shirt
    if (w.pat && !dress) {
        const float y0 = tTop + 2, y1 = tBot - 2;
        switch (w.pat) {
        case 1: for (int i = 0; i < 4; i++) sp.dl(cx - waist, y0 + i * 4.5f, cx + waist - 1, y0 + i * 4.5f + 1.5f, 1.1f, C(BLACK)); break;
        case 2: for (int i = 0; i < 6; i++) sp.dot(cx - 4 + (i % 3) * 4.f, y0 + 2 + (i / 3) * 7.f + (i % 2), 1.0f, C(80, 50, 30)); break;
        case 3: for (int i = 0; i < 4; i++) sp.dl(cx - waist, y0 + i * 4.5f, cx + waist, y0 + i * 4.5f, 1.4f, C(255, 255, 255, 170)); break;
        case 4: for (int i = 0; i < 3; i++) { sp.dl(cx - waist, y0 + i * 6.f, cx + waist, y0 + i * 6.f, 0.6f, C(BLACK, 110)); sp.dl(cx - 4 + i * 4.f, y0 - 1, cx - 4 + i * 4.f, y1, 0.6f, C(200, 40, 50, 160)); } break;
        case 5: for (int r = 0; r < 4; r++) for (int c = 0; c < 3; c++) if ((r + c) % 2) sp.p.R(cx - 4.5f + c * 3.f, y0 + r * 3.f, 3.f, 3.f, C(BLACK, 200)); break;
        case 6: for (int i = 0; i < 4; i++) sp.p.H(cx - 2.5f + (i % 2) * 5.f, y0 + 3 + (i / 2) * 7.f, 1.1f, C(255, 255, 255, 220)); break;
        case 7: for (int i = 0; i < 6; i++) sp.dot(cx - 4 + (i % 3) * 4.f, y0 + 2 + (i / 3) * 6.f, 0.5f, C(255, 255, 255, 240)); break;
        case 8: if (!sp.ink) { sp.p.H(cx, y0 + 6, 2.6f, C(HOT)); sp.p.Sp(cx + 3, y0 + 2, 1.4f, C(GOLD)); } break;
        }
    }
    // overalls bib
    if (bt == BT_OVERALLS && !dress) {
        grp(sp, [&] { sp.rc(cx - 4.2f, sh + 6, 8.4f, hip - sh - 4, C(w.bot)); });
        sp.dl(cx - 3.8f, sh + 6.5f, cx - 4.2f, sh - 0.5f, 1.2f, C(w.bot)); sp.dl(cx + 3.8f, sh + 6.5f, cx + 4.2f, sh - 0.5f, 1.2f, C(w.bot));
        sp.dot(cx - 3, sh + 7.5f, 0.6f, C(GOLD)); sp.dot(cx + 3, sh + 7.5f, 0.6f, C(GOLD));
    }
    if (w.acc & ACC_FLANNEL) grp(sp, [&] { sp.rc(cx - waist - 0.5f, hip - 2, waist * 2 + 1, 3, C(w.accCol)); sp.tr(cx - waist, hip, cx - waist + 4, hip, cx - waist + 1, hip + 10, C(w.accCol)); });

    // ---- arms
    auto arm = [&](float sx, float ex2, float ey2, float hx, float hy) {
        bool skinFore = !longSleeve, skinUpper = noSleeve;
        if (doodle) { sp.dl(sx, shY, ex2, ey2, lt, INK); sp.dl(ex2, ey2, hx, hy, lt, INK); sp.dot(hx, hy, lt * 0.9f, INK); return; }
        grp(sp, [&] {
            if (skinUpper) { sp.ln(sx, shY, ex2, ey2, lt, SK); sp.ci(ex2, ey2, lt / 2, SK); }
            if (skinFore) sp.ln(ex2, ey2, hx, hy, lt, SK);
            sp.ci(hx, hy, lt * 0.85f, SK);
        });
        if (!skinUpper && !longSleeve) {   // short sleeve: re-cover the shoulder end
            float u = w.ts == TS_BUTTON ? 0.95f : 0.55f, st = lt + 3.8f + baggy;
            sp.dl(sx, shY, sx + (ex2 - sx) * u, shY + (ey2 - shY) * u, st, T);
            sp.dot(sx, shY + 0.5f, st / 2, T);
        }
    };
    arm(shL, eLx, eLy, hLx, hLy);
    if (pose != PO_PHONE) arm(shR, eRx, eRy, hRx, hRy);
    // ---- neck & head
    grp(sp, [&] { if (doodle) sp.ln(cx, hc + H * 0.8f, cx, sh + 1, lt, INK); else sp.ln(cx, hc + H * 0.8f, cx, sh + 1.5f, lt + 0.6f, SK); });
    if ((w.acc & ACC_CHOKER) && !sp.ink) { sp.dl(cx - 1.8f, sh - 1.5f, cx + 1.8f, sh - 1.5f, 1.1f, C(BLACK)); sp.dot(cx, sh - 0.6f, 0.6f, C(HOT)); }
    if (w.acc & ACC_PEARLS) for (int i = 0; i < 7; i++) { float u = i / 6.f - 0.5f; sp.dot(cx + u * 6.f, sh + 1.5f + (0.25f - u * u) * 5.f, 0.55f, C(255, 252, 240)); }
    grp(sp, [&] { sp.ci(cx, hc, H, SK); });
    const float hl = hairlineOf(hs);
    grp(sp, [&] {   // hair cap (the top of the head, down to the cheeks) + anything that sits on top
        if (hl < 5.f) {
            const float R = 1.07f * H, ccy = hc - 0.03f * H, fx = cx, fy = hc + 0.3f * H;
            const int N = 10;
            for (int i = 0; i < N; i++) {
                float a0 = 3.14159f * (-0.1f + 1.2f * i / N), a1 = 3.14159f * (-0.1f + 1.2f * (i + 1) / N);
                sp.tr(fx, fy, cx - std::cos(a0) * R, ccy - std::sin(a0) * R, cx - std::cos(a1) * R, ccy - std::sin(a1) * R, HR);
            }
        }
        hairFront(sp, hs, cx, hc, H, HR, HRD, t, 0);
    });
    if (hl < 5.f) faceOval(sp, cx, hc, H, hl, SK, INK);
    // earrings
    if ((d.f.pierce == 2 || d.f.pierce == 4 || (w.acc & ACC_HOOPS)) && !kid) { for (int s2 = -1; s2 <= 1; s2 += 2) for (int i = 0; i < 8; i++) { float a0 = i * 0.785f, a1 = a0 + 0.785f; float ex = cx + s2 * 0.98f * H, ey = hc + 0.45f * H; sp.dl(ex + std::cos(a0) * 0.18f * H, ey + std::sin(a0) * 0.18f * H, ex + std::cos(a1) * 0.18f * H, ey + std::sin(a1) * 0.18f * H, 0.05f * H, C(GOLD)); } }
    else if (d.f.pierce == 1 || d.f.pierce == 3) { sp.dot(cx - 0.98f * H, hc + 0.32f * H, 0.07f * H, C(GOLD)); sp.dot(cx + 0.98f * H, hc + 0.32f * H, 0.07f * H, C(GOLD)); }
    drawFace(sp, d, cx, hc, H, mood, blink, walking ? 1.f : 0.4f, doodle);
    if (d.f.pierce >= 3) sp.dot(cx + 0.12f * H + 0.06f * H, hc + 0.22f * H, 0.045f * H, C(230, 230, 240));
    // ---- front hair & hats
    grp(sp, [&] { hairFront(sp, hs, cx, hc, H, HR, HRD, t, 1); });
    (void)g_hl;
    if ((w.acc & ACC_STREAKS) && !sp.ink) { sp.dl(cx - 0.5f * H, hc - 0.95f * H, cx - 0.75f * H, hc - 0.2f * H, 0.12f * H, C(w.accCol)); sp.dl(cx + 0.45f * H, hc - 0.95f * H, cx + 0.7f * H, hc - 0.3f * H, 0.1f * H, C(80, 200, 255)); }
    if (hs != 24 && !doodle && hs != 17 && hs != 16) { sp.dl(cx - 0.55f * H, hc - 0.82f * H, cx - 0.25f * H, hc - 0.93f * H, 0.09f * H, C(255, 255, 255, 110)); }   // hair shine
    int hat = w.hat;
    if (!hat && (w.acc & ACC_BEANIE)) hat = HT_BEANIE;
    if (!hat && (w.acc & ACC_BOW)) hat = HT_BOW;
    if (!hat && (w.acc & ACC_TIARA)) hat = HT_TIARA;
    RGB hatCol = (w.acc & (ACC_BEANIE | ACC_BOW)) ? w.accCol : hat == HT_BOW || hat == HT_HEADBAND ? RGB{255, 90, 160} : shade(w.top, 0.95f);
    if (holiday) grp(sp, [&] { holidayHat(sp, holiday, cx, hc, H); });
    else if (hat) grp(sp, [&] { drawHatS(sp, hat, cx, hc, H, hatCol, t); });
    if (w.acc & ACC_CLIPS) for (int s2 = -1; s2 <= 1; s2 += 2) { float bx = cx + s2 * 0.55f * H, by = hc - 0.75f * H; sp.p.T(bx, by, bx - 1.6f, by - 1.2f, bx - 1.4f, by + 1.f, C(w.accCol)); sp.p.T(bx, by, bx + 1.6f, by - 1.2f, bx + 1.4f, by + 1.f, C(w.accCol)); }
    if (w.acc & ACC_SHADES) { for (int s2 = -1; s2 <= 1; s2 += 2) sp.p.E(cx + s2 * 0.36f * H + 0.04f * H, hc - 0.05f * H, 0.42f * H, 0.3f * H, C(30, 22, 30, 240)); sp.dl(cx - 0.15f * H, hc - 0.1f * H, cx + 0.15f * H, hc - 0.1f * H, 0.06f * H, C(30, 22, 30)); }
    if (outfit == 8 && !d.guy) { grp(sp, [&] { sp.tr(cx - 0.85f * H, hc - 0.7f * H, cx - 0.25f * H, hc - 0.98f * H, cx - 0.75f * H, hc - 1.5f * H, C(40, 30, 40)); sp.tr(cx + 0.85f * H, hc - 0.7f * H, cx + 0.25f * H, hc - 0.98f * H, cx + 0.75f * H, hc - 1.5f * H, C(40, 30, 40)); }); }
    // ---- phone arm last (in front of the face)
    if (pose == PO_PHONE) {
        arm(shR, eRx, eRy, hRx, hRy);
        grp(sp, [&] { sp.rc(hRx - 1.6f, hRy - 4.f, 3.4f, 5.6f, C(255, 110, 180)); });
    }
    if (!bust && (pose == PO_SLEEP) && !g_ink) {
        float zt = std::fmod(t, 2.f);
        text("z", pen.X(cx + H) + zt * 4, pen.Y(hc - H) - zt * 6, 0.4f * s + 0.14f, C(255, 255, 255, (uint8_t)(220 - zt * 100)));
    }
}

// ---------------------------------------------------------------- babies
static void drawStickBaby(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip) {
    const bool doodle = g_artStyle == 8;
    s *= 0.62f;
    float bob = std::sin(t * 3.f) * 0.6f;
    Pen pen(x, footY + bob * s, 40, 40, s, flip);
    const u32 INK = doodle ? C(28, 24, 30) : C(34, 26, 44);
    SP sp(pen, INK, (doodle ? 0.75f : 1.05f) / std::fmax(0.3f, s));
    const RGB skin = skinRGB(d.skin), hair = hairRGB(d.hairCol);
    RGB on = outfit > 0 && outfit < NUM_OUTFITS ? OUTFITS[outfit].top : d.top;
    if (outfit == 8) on = RGB{40, 30, 40};
    const u32 SK = C(skin), ON = C(on);
    const float wave = pose == PO_WAVE ? std::sin(t * 9) * 3 : 0;
    // stubby legs + arms
    grp(sp, [&] { sp.ln(15, 30, 13, 38, 3.2f, SK); sp.ln(25, 30, 27, 38, 3.2f, SK); sp.ci(13, 38, 2.2f, SK); sp.ci(27, 38, 2.2f, SK); });
    grp(sp, [&] { sp.el(20, 29, 17, 15, ON); });   // onesie
    grp(sp, [&] { sp.ln(13, 25, 7, 30 + wave, 2.8f, SK); sp.ln(27, 25, 33, 30, 2.8f, SK); sp.ci(7, 30 + wave, 1.8f, SK); sp.ci(33, 30, 1.8f, SK); });
    sp.p.H(20, 29, 2.2f, C(255, 255, 255, 160));
    grp(sp, [&] { sp.ci(20, 12, 11.5f, SK); });
    grp(sp, [&] { sp.el(20, 2.5f, 9, 4, C(hair)); sp.ln(20, 1, 23, -2.5f, 1.6f, C(hair)); });
    if (!d.guy) grp(sp, [&] { sp.tr(25, 2, 30, -1, 30, 5, C(HOT)); sp.tr(25, 2, 20.5f, -1, 20.5f, 5, C(HOT)); });
    const bool closed = pose == PO_SLEEP || std::fmod(t + d.skin, 4.f) < 0.12f;
    if (closed) { sp.dl(14.5f, 12.5f, 17.5f, 13.2f, 0.9f, INK); sp.dl(22.5f, 13.2f, 25.5f, 12.5f, 0.9f, INK); }
    else if (artIsDoll(g_artStyle)) {
        static const RGB IR[7] = {{112, 70, 40}, {58, 36, 28}, {142, 112, 54}, {70, 142, 92}, {72, 124, 204}, {128, 140, 152}, {146, 92, 186}};
        for (int s2 = -1; s2 <= 1; s2 += 2) { float ex = 20 + s2 * 4.f; pen.E(ex, 12, 4.2f, 3.4f, C(252, 250, 248)); pen.Ci(ex + 0.2f, 12.2f, 1.45f, C(eyeRGB(d.f.eyeCol))); pen.Ci(ex + 0.2f, 12.2f, 0.7f, C(22, 16, 22)); pen.Ci(ex + 0.7f, 11.6f, 0.45f, C(WHITE)); pen.L(ex - 2.1f, 11.6f, ex + 2.1f, 11.2f, 0.6f, INK); pen.L(ex + s2 * 2.1f, 11.3f, ex + s2 * 2.9f, 10.5f, 0.35f, INK); }
    } else { pen.Ci(16, 12, 1.6f, INK); pen.Ci(24, 12, 1.6f, INK); pen.Ci(16.5f, 11.4f, 0.5f, C(WHITE)); pen.Ci(24.5f, 11.4f, 0.5f, C(WHITE)); }
    sp.dl(18.2f, 17.f, 20.f, 18.f, 0.8f, INK); sp.dl(20.f, 18.f, 21.8f, 17.f, 0.8f, INK);
    if (holiday) grp(sp, [&] { holidayHat(sp, holiday, 20, 12, 11.5f); });
}


// ============================================================ Fashion Doll style
// Early-2000s fashion-illustration dolls: long legs, small waists, long necks and
// slightly oversized heads; almond eyes with liner & lashes, illustrated lips, big
// glossy hair, cel-shaded clothes with folds & seams, jewellery, bags and shades.
struct DollSkel {
    float hx, hy, rx, ry, H;      // head centre, radii, hair unit
    float sh;                     // shoulder line
    float rowsY[14], rowsW[14];   // torso silhouette (half widths), neck base -> crotch
    int nRows;
    float waistY, hipY, hipJ;     // hip joint y
    float kneeY, ankleY;
    float thighW, kneeW, calfW, ankleW;
    float shJ, upperW, foreW;     // shoulder joint offset, arm widths
    float armLen;
};
static DollSkel dollSkel(Age a, bool guy) {
    DollSkel k{};
    const bool kid = a == KID, teen = a == TEEN;
    if (kid) {
        k.hx = 20; k.hy = 16.5f; k.rx = 8.6f; k.ry = 9.8f; k.H = 9.6f;
        k.sh = 30.5f;
        float ry[] = {29.5f, 30.8f, 33.f, 40.f, 46.f, 51.f, 54.f};
        float rw[] = {2.4f, 6.2f, 6.8f, 6.0f, 5.8f, 6.4f, 3.6f};
        k.nRows = 7; for (int i = 0; i < 7; i++) { k.rowsY[i] = ry[i]; k.rowsW[i] = rw[i]; }
        k.waistY = 46; k.hipY = 51; k.hipJ = 52.5f; k.kneeY = 85; k.ankleY = 114.5f;
        k.thighW = 5.2f; k.kneeW = 3.6f; k.calfW = 3.8f; k.ankleW = 2.4f;
        k.shJ = 6.0f; k.upperW = 3.0f; k.foreW = 2.5f; k.armLen = 26;
        return k;
    }
    const float hs = teen ? 1.04f : 1.f;
    k.hx = 20; k.hy = 14.6f; k.rx = 7.9f * hs; k.ry = 9.4f * hs; k.H = 9.3f * hs;
    k.sh = 31.f;
    if (guy) {
        float ry[] = {29.6f, 30.6f, 32.f, 34.f, 40.f, 47.f, 53.f, 59.f, 63.f, 65.5f};
        float rw[] = {3.0f, 5.8f, 8.8f, 9.8f, 8.6f, 7.0f, 6.6f, 7.0f, 6.9f, 4.4f};
        k.nRows = 10; for (int i = 0; i < 10; i++) { k.rowsY[i] = ry[i]; k.rowsW[i] = rw[i] * (teen ? 0.93f : 1.f); }
        k.thighW = 6.2f; k.kneeW = 3.9f; k.calfW = 4.0f; k.ankleW = 2.4f;
        k.shJ = 8.4f; k.upperW = 3.7f; k.foreW = 2.9f;
    } else {
        const float b = teen ? 0.6f : 1.f;   // teens: a smaller bust
        float ry[] = {29.6f, 30.6f, 31.8f, 33.8f, 37.5f, 40.5f, 44.5f, 49.f, 54.f, 58.5f, 63.f, 65.5f};
        float rw[] = {2.2f, 4.6f, 6.9f, 7.7f, 6.9f, 6.6f + 0.6f * b, 5.5f, 4.6f, 6.0f, 7.5f, 7.2f, 3.9f};
        k.nRows = 12; for (int i = 0; i < 12; i++) { k.rowsY[i] = ry[i]; k.rowsW[i] = rw[i]; }
        k.thighW = 6.6f; k.kneeW = 3.5f; k.calfW = 3.8f; k.ankleW = 2.0f;
        k.shJ = 6.9f; k.upperW = 3.1f; k.foreW = 2.3f;
    }
    k.waistY = guy ? 52 : 49; k.hipY = 58.5f; k.hipJ = 62; k.kneeY = 89.5f; k.ankleY = 114;
    k.armLen = 33;
    return k;
}
static float rowW(const DollSkel& k, float y) {   // torso half-width at height y
    if (y <= k.rowsY[0]) return k.rowsW[0];
    for (int i = 1; i < k.nRows; i++) if (y <= k.rowsY[i]) { float u = (y - k.rowsY[i - 1]) / (k.rowsY[i] - k.rowsY[i - 1]); return k.rowsW[i - 1] + (k.rowsW[i] - k.rowsW[i - 1]) * u; }
    return k.rowsW[k.nRows - 1];
}

// a tapered limb segment with round ends
static void taper(SP& sp, float x0, float y0, float w0, float x1, float y1, float w1, u32 c) {
    float dx = x1 - x0, dy = y1 - y0, L = std::sqrt(dx * dx + dy * dy);
    if (L < 0.01f) { sp.ci(x0, y0, w0 / 2, c); return; }
    float nx = -dy / L, ny = dx / L;
    sp.qd(x0 + nx * w0 / 2, y0 + ny * w0 / 2, x1 + nx * w1 / 2, y1 + ny * w1 / 2, x1 - nx * w1 / 2, y1 - ny * w1 / 2, x0 - nx * w0 / 2, y0 - ny * w0 / 2, c);
    sp.ci(x0, y0, w0 / 2, c); sp.ci(x1, y1, w1 / 2, c);
}
// cel shadow along one side of a tapered segment (fill pass only)
static void taperShade(SP& sp, float x0, float y0, float w0, float x1, float y1, float w1, u32 c, float side = -1) {
    if (sp.ink) return;
    float dx = x1 - x0, dy = y1 - y0, L = std::sqrt(dx * dx + dy * dy);
    if (L < 0.01f) return;
    float nx = -dy / L * side, ny = dx / L * side;
    sp.p.Q(x0 + nx * w0 * 0.5f, sp.cy(y0 + ny * w0 * 0.5f), x1 + nx * w1 * 0.5f, sp.cy(y1 + ny * w1 * 0.5f), x1 + nx * w1 * 0.1f, sp.cy(y1 + ny * w1 * 0.1f), x0 + nx * w0 * 0.1f, sp.cy(y0 + ny * w0 * 0.1f), c);
}
// a torso-shaped band following the silhouette from y0 to y1, grown by `grow`
static void band(SP& sp, const DollSkel& k, float y0, float y1, float grow, u32 c, float cx = 20) {
    float ys[16]; int n = 0;
    ys[n++] = y0;
    for (int i = 0; i < k.nRows; i++) if (k.rowsY[i] > y0 && k.rowsY[i] < y1 && n < 14) ys[n++] = k.rowsY[i];
    ys[n++] = y1;
    for (int i = 0; i + 1 < n; i++) {
        float a = rowW(k, ys[i]) + grow, b = rowW(k, ys[i + 1]) + grow;
        sp.qd(cx - a, ys[i], cx + a, ys[i], cx + b, ys[i + 1], cx - b, ys[i + 1], c);
    }
}
static void bandShade(SP& sp, const DollSkel& k, float y0, float y1, float grow, float frac, u32 c, float cx = 20) {
    if (sp.ink) return;
    const int N = 6;
    for (int i = 0; i < N; i++) {
        float ya = y0 + (y1 - y0) * i / N, yb = y0 + (y1 - y0) * (i + 1) / N;
        if (ya >= sp.cutY) break;
        float a = rowW(k, ya) + grow, b = rowW(k, yb) + grow;
        yb = sp.cy(yb);
        sp.p.Q(cx - a, ya, cx - a + 2 * a * frac, ya, cx - b + 2 * b * frac, yb, cx - b, yb, c);
    }
}

static void dollFace(SP& sp, const PersonDef& d, const DollSkel& k, int mood, bool blink, float look, float t) {
    const FaceDef& f = d.f;
    const u32 INK = sp.inkc;
    const RGB skin = skinRGB(d.skin), hair = hairRGB(d.hairCol);
    const bool guy = d.guy, kid = d.age == KID;
    const float cx = k.hx, hy = k.hy, sc = k.rx / 7.0f;
    const float ey = hy + 0.8f * sc, ex = 3.15f * sc;
    const float ew = (kid ? 3.9f : guy ? 3.5f : 3.9f) * sc, eh = (kid ? 2.3f : guy ? 1.6f : 1.95f) * sc;
    const bool bold = !guy && !kid && (f.liner == 2 || f.shadow == 2);   // dramatic makeup is the player's choice
    const float lx = look * 0.35f * sc;
    static const RGB IR[7] = {{112, 70, 40}, {58, 36, 28}, {142, 112, 54}, {70, 142, 92}, {72, 124, 204}, {128, 140, 152}, {146, 92, 186}};
    static const RGB SHADOW[7] = {{0, 0, 0}, {255, 120, 186}, {74, 62, 84}, {232, 190, 98}, {192, 142, 232}, {110, 162, 232}, {170, 108, 70}};
    static const RGB BL[5] = {{255, 130, 160}, {255, 116, 156}, {255, 150, 118}, {204, 60, 112}, {172, 100, 60}};
    const u32 BROW = C(shade(hair, d.hairCol >= 3 && d.hairCol != 5 ? 0.45f : 0.8f));
    // cheek blush & contour
    {
        uint8_t a = !f.blush ? 0 : guy ? 40 : bold ? 90 : 55;   // blush is opt-in
        if (a) { RGB b = mix(blushRGB(f.blush), skin, 0.35f); for (int s2 = -1; s2 <= 1; s2 += 2) { sp.p.E(cx + s2 * 4.0f * sc, hy + 3.5f * sc, 4.0f * sc, 2.0f * sc, C(b, (uint8_t)(a * 0.55f))); sp.p.E(cx + s2 * 4.0f * sc, hy + 3.5f * sc, 2.6f * sc, 1.2f * sc, C(b, (uint8_t)(a * 0.7f))); } }
        sp.p.Q(cx - 6.4f * sc, hy + 1.8f * sc, cx - 5.4f * sc, hy + 2.6f * sc, cx - 4.2f * sc, hy + 6.6f * sc, cx - 5.6f * sc, hy + 5.6f * sc, C(shade(skin, 0.82f), 34));
    }
    if (f.marks == 1 || f.marks == 3) for (int i = 0; i < 5; i++) { float fx = (i - 2) * 0.55f * sc; sp.p.Ci(cx - 3.6f * sc + fx, hy + 2.7f * sc + (i % 2) * 0.35f * sc, 0.18f * sc, C(150, 90, 60, 200)); sp.p.Ci(cx + 3.6f * sc + fx, hy + 2.7f * sc + (i % 2) * 0.35f * sc, 0.18f * sc, C(150, 90, 60, 200)); }
    if (f.marks >= 2) sp.p.Ci(cx + 3.1f * sc, hy + 5.6f * sc, 0.25f * sc, C(60, 34, 30));
    // eyes
    for (int s = -1; s <= 1; s += 2) {
        const float x = cx + s * ex, dir = (float)s;   // dir points to the outer corner
        if (mood == 4 && s == 1 && !blink) {   // wink: a curved closed lid with lashes
            sp.dl(x - ew * 0.5f, ey + 0.1f, x, ey + 0.6f * sc, 0.45f * sc, INK); sp.dl(x, ey + 0.6f * sc, x + ew * 0.5f, ey, 0.45f * sc, INK);
            sp.dl(x + ew * 0.5f, ey, x + ew * 0.5f + 0.9f * sc, ey - 0.6f * sc, 0.3f * sc, INK);
        } else if (blink || mood == 7) {
            sp.dl(x - ew * 0.5f, ey + 0.2f * sc, x, ey + 0.55f * sc, 0.4f * sc, INK); sp.dl(x, ey + 0.55f * sc, x + ew * 0.5f, ey + 0.2f * sc, 0.4f * sc, INK);
            if (!guy) for (int i = 0; i < 3; i++) sp.dl(x + dir * (0.2f + i * 0.6f) * sc, ey + 0.55f * sc, x + dir * (0.4f + i * 0.75f) * sc, ey + 1.3f * sc, 0.22f * sc, INK);
        } else {
            float h = eh * (mood == 5 ? 1.15f : 1.f);
            // a soft almond with only a slight lift at the outer corner
            const float Lx = x - dir * ew * 0.5f, Ly = ey + 0.15f * sc, Rx = x + dir * ew * 0.5f, Ry = ey - 0.1f * sc;
            const float U1x = x - dir * ew * 0.2f, U1y = ey - h * 0.55f, U2x = x + dir * ew * 0.16f, U2y = ey - h * 0.58f;
            const float D1x = x - dir * ew * 0.18f, D1y = ey + h * 0.45f, D2x = x + dir * ew * 0.2f, D2y = ey + h * 0.4f;
            if (f.shadow && !guy) { RGB sh = shadowRGB(f.shadow); sp.p.Q(Lx + dir * 0.3f * sc, Ly - 0.4f * sc, U1x, U1y - 0.7f * sc, U2x, U2y - 0.8f * sc, Rx + dir * 0.3f * sc, Ry - 0.4f * sc, C(sh, bold ? 100 : 38)); }
            const u32 WH = C(252, 250, 248);
            sp.p.T(Lx, Ly, U1x, U1y, D1x, D1y, WH); sp.p.T(U1x, U1y, U2x, U2y, D1x, D1y, WH); sp.p.T(U2x, U2y, D2x, D2y, D1x, D1y, WH); sp.p.T(U2x, U2y, Rx, Ry, D2x, D2y, WH);
            // iris centred in the eye, pupil centred in the iris, one catchlight
            float ix = x + lx, iy = ey + 0.05f * sc, ir = h * 0.47f;
            if (mood == 9) ix = x + 0.45f * sc;   // embarrassed: a glance away
            sp.p.Ci(ix, iy, ir, C(eyeRGB(f.eyeCol)));
            sp.p.Ci(ix, iy, ir * 0.45f, C(26, 18, 24));
            sp.p.Ci(ix + ir * 0.35f, iy - ir * 0.35f, ir * 0.28f, C(WHITE));
            // gentle lids for moods
            float lidDrop = mood == 3 ? h * 0.3f : mood == 6 ? h * 0.36f : mood == 8 ? h * 0.15f : 0.f;
            if (lidDrop > 0) { sp.p.Q(Lx, Ly - 0.3f * sc, U1x, U1y - 0.3f * sc, U2x, U2y - 0.3f * sc, Rx, Ry - 0.3f * sc, C(skin)); sp.p.Q(Lx, Ly, U1x, U1y + lidDrop, U2x, U2y + lidDrop, Rx, Ry, C(skin)); }
            if (mood == 1) sp.p.Q(D1x - dir * 0.5f * sc, D1y + 0.4f * sc, D2x + dir * 0.4f * sc, D2y + 0.4f * sc, D2x, D2y - h * 0.12f, D1x, D1y - h * 0.14f, C(skin));   // smiling eyes
            // a fine upper lash line; lashes & wings only with makeup on
            float lw = (guy ? 0.32f : bold ? 0.55f : 0.38f) * sc;
            sp.dl(Lx, Ly, U1x, U1y + lidDrop, lw * 0.8f, INK); sp.dl(U1x, U1y + lidDrop, U2x, U2y + lidDrop, lw, INK); sp.dl(U2x, U2y + lidDrop, Rx, Ry, lw, INK);
            sp.dl(D1x, D1y, D2x, D2y, 0.18f * sc, C(shade(skin, 0.6f), 120));
            if (!guy && f.liner >= 1) {
                int n = bold ? 3 : 2;
                for (int i = 0; i < n; i++) { float u = 0.7f + i * 0.13f; float bx = U2x + (Rx - U2x) * (u - 0.5f) * 2.f * 0.5f + (Rx - U2x) * 0.5f, by = U2y + (Ry - U2y) * u + lidDrop * (1 - u); sp.dl(bx, by, bx + dir * (0.35f + i * 0.12f) * sc, by - (0.55f + (bold ? 0.2f : 0.f)) * sc, 0.18f * sc, INK); }
                if (bold && f.liner == 2) sp.dl(Rx, Ry, Rx + dir * 0.9f * sc, Ry - 0.55f * sc, lw * 0.85f, INK);   // the wing
            }
        }
        // brows: soft, relaxed arcs; only moods move them
        float bIn = ey - 2.55f * sc, bPk = ey - 2.95f * sc, bTl = ey - 2.7f * sc;
        if (mood == 3) { bIn += 0.45f * sc; }
        if (mood == 2 || mood == 9) { bIn -= 0.35f * sc; bTl += 0.2f * sc; }
        if (mood == 1 || mood == 5) { bIn -= 0.3f * sc; bPk -= 0.3f * sc; bTl -= 0.25f * sc; }
        if (mood == 8 && s == 1) { bPk -= 0.35f * sc; }
        const u32 BRW = C(mix(shade(hair, d.hairCol >= 3 && d.hairCol != 5 ? 0.5f : 0.85f), skin, 0.2f));
        float bw = (f.brows == 2 || f.brows == 4) ? 0.46f : guy ? 0.42f : 0.32f;
        sp.dl(x - dir * ew * 0.45f, bIn, x + dir * ew * 0.08f, bPk, bw * sc, BRW);
        sp.dl(x + dir * ew * 0.08f, bPk, x + dir * ew * 0.58f, bTl, bw * 0.7f * sc, BRW);
        (void)BROW;
    }
    if (mood == 9) { sp.p.Ci(cx + 6.2f * sc, hy - 3.6f * sc, 0.7f * sc, C(150, 210, 255, 220)); sp.p.T(cx + 5.6f * sc, hy - 3.7f * sc, cx + 6.8f * sc, hy - 3.7f * sc, cx + 6.2f * sc, hy - 5.0f * sc, C(150, 210, 255, 220)); }
    // nose: a stylised line + nostril, with a soft shadow
    const u32 NS = C(shade(skin, 0.62f), 210);
    sp.dl(cx + 0.55f * sc, hy + 2.9f * sc, cx + 0.8f * sc, hy + 4.0f * sc, 0.24f * sc, C(shade(skin, 0.7f), 120));
    sp.dl(cx - 0.55f * sc, hy + 4.35f * sc, cx + 0.15f * sc, hy + 4.6f * sc, 0.3f * sc, NS);
    sp.dl(cx + 0.15f * sc, hy + 4.6f * sc, cx + 0.75f * sc, hy + 4.25f * sc, 0.26f * sc, NS);
    // lips
    const float my = hy + 6.0f * sc, mw = (guy ? 3.1f : kid ? 2.8f : 3.4f) * sc;
    const RGB L = guy || kid ? mix(skin, RGB{190, 90, 100}, guy ? 0.3f : 0.45f) : bold ? d.lips : mix(d.lips, mix(skin, RGB{200, 100, 110}, 0.55f), 0.5f);
    const u32 LIP = C(L), LIPD = C(shade(L, 0.72f)), LINE = C(shade(L, 0.45f));
    switch (mood) {
    case 1: case 4: case 0: case 8: {   // smiles
        const bool open = mood == 1;
        const float up = mood == 8 ? 0.6f : 0.75f;
        if (open) {   // open, excited smile with a row of teeth
            sp.p.Q(cx - mw, my - 0.5f * sc, cx + mw, my - 0.5f * sc, cx + mw * 0.55f, my + 1.7f * sc, cx - mw * 0.55f, my + 1.7f * sc, C(110, 30, 50));
            sp.p.Q(cx - mw * 0.85f, my - 0.45f * sc, cx + mw * 0.85f, my - 0.45f * sc, cx + mw * 0.7f, my + 0.25f * sc, cx - mw * 0.7f, my + 0.25f * sc, C(WHITE));
            sp.p.E(cx, my + 1.2f * sc, mw * 0.8f, 0.7f * sc, C(240, 110, 130));
            sp.dl(cx - mw, my - 0.5f * sc, cx + mw, my - 0.5f * sc, 0.3f * sc, LIPD);
            sp.p.E(cx, my + 1.85f * sc, mw * 1.2f, 0.8f * sc, LIP);
        } else {
            const float rs = mood == 8 ? 0.3f : 0.f;   // confident smirk: one corner higher
            if (!guy) { sp.p.E(cx - mw * 0.3f, my - 0.25f * sc, mw * 0.8f, 0.65f * sc, LIP); sp.p.E(cx + mw * 0.3f, my - 0.25f * sc - rs, mw * 0.8f, 0.65f * sc, LIP); }
            sp.p.E(cx, my + 0.5f * sc, mw * 1.2f, (guy ? 0.7f : 1.05f) * sc, LIP);
            sp.p.E(cx, my + 0.62f * sc, mw * 0.8f, 0.45f * sc, LIPD);
            sp.p.E(cx, my + 0.48f * sc, mw * 1.05f, (guy ? 0.5f : 0.75f) * sc, LIP);
            sp.dl(cx - mw * 0.7f, my + 0.05f * sc, cx + mw * 0.7f, my + 0.05f * sc - rs, 0.28f * sc, LINE);
            sp.dl(cx - mw * 0.7f, my + 0.05f * sc, cx - mw * 0.95f, my - up * sc * 0.6f, 0.26f * sc, LINE);
            sp.dl(cx + mw * 0.7f, my + 0.05f * sc - rs, cx + mw * 0.98f, my - up * sc * 0.6f - rs * 1.5f, 0.26f * sc, LINE);
            if (!guy) sp.p.E(cx + mw * 0.25f, my + 0.45f * sc, mw * 0.4f, 0.3f * sc, C(255, 255, 255, 190));   // gloss
        }
        break;
    }
    case 5:   // surprised: a little "o"
        sp.p.E(cx, my + 0.4f * sc, mw * 0.75f, 1.6f * sc, LIP); sp.p.E(cx, my + 0.4f * sc, mw * 0.45f, 1.0f * sc, C(110, 30, 50));
        break;
    case 9:   // embarrassed: a wobbly little smile
        sp.p.E(cx, my + 0.4f * sc, mw * 1.1f, 1.1f * sc, LIP);
        sp.dl(cx - mw * 0.55f, my + 0.3f * sc, cx - mw * 0.2f, my + 0.05f * sc, 0.26f * sc, LINE); sp.dl(cx - mw * 0.2f, my + 0.05f * sc, cx + mw * 0.15f, my + 0.35f * sc, 0.26f * sc, LINE); sp.dl(cx + mw * 0.15f, my + 0.35f * sc, cx + mw * 0.55f, my + 0.05f * sc, 0.26f * sc, LINE);
        break;
    default:   // sad / annoyed / bored: pressed lips, corners down
        sp.p.E(cx - mw * 0.3f, my - 0.2f * sc, mw * 0.8f, 0.8f * sc, LIP); sp.p.E(cx + mw * 0.3f, my - 0.2f * sc, mw * 0.8f, 0.8f * sc, LIP);
        sp.p.E(cx + (mood == 3 ? 0.4f * sc : 0), my + 0.6f * sc, mw * 1.2f, 1.2f * sc, LIP);
        sp.dl(cx - mw * 0.75f, my + 0.35f * sc, cx + mw * 0.75f, my + 0.2f * sc, 0.28f * sc, LINE);
        if (mood != 6) { sp.dl(cx - mw * 0.75f, my + 0.35f * sc, cx - mw * 0.95f, my + 0.9f * sc, 0.24f * sc, LINE); sp.dl(cx + mw * 0.75f, my + 0.2f * sc, cx + mw * 0.95f, my + 0.8f * sc, 0.24f * sc, LINE); }
        if (mood == 2 && !blink) sp.p.Ci(cx + ex + 1.2f * sc, ey + 1.6f * sc + std::fmod(t, 1.5f) * 1.5f * sc, 0.45f * sc, C(150, 210, 255, 220));
        break;
    }
    // facial hair
    if (guy && f.beard) {
        u32 BR = C(shade(hair, 0.9f), f.beard == 1 ? 90 : 235);
        if (f.beard == 1) for (int i = 0; i < 14; i++) { float u = (i % 7) / 6.f - 0.5f; sp.p.Ci(cx + u * 8.f * sc, hy + 5.6f * sc + (i / 7) * 1.4f * sc + (0.25f - u * u) * 4.f * sc, 0.16f * sc, BR); }
        else if (f.beard == 2) { sp.p.Q(cx - 1.4f * sc, my + 1.3f * sc, cx + 1.4f * sc, my + 1.3f * sc, cx + 1.0f * sc, my + 2.9f * sc, cx - 1.0f * sc, my + 2.9f * sc, BR); sp.dl(cx - mw, my - 0.9f * sc, cx + mw, my - 0.9f * sc, 0.6f * sc, BR); }
        else {
            sp.p.Q(cx - 6.6f * sc, hy + 2.6f * sc, cx - 4.8f * sc, hy + 3.4f * sc, cx - 2.2f * sc, hy + 9.2f * sc, cx - 4.6f * sc, hy + 7.6f * sc, BR);
            sp.p.Q(cx + 4.8f * sc, hy + 3.4f * sc, cx + 6.6f * sc, hy + 2.6f * sc, cx + 4.6f * sc, hy + 7.6f * sc, cx + 2.2f * sc, hy + 9.2f * sc, BR);
            sp.p.Q(cx - 2.6f * sc, my + 1.4f * sc, cx + 2.6f * sc, my + 1.4f * sc, cx + 2.0f * sc, hy + 9.4f * sc, cx - 2.0f * sc, hy + 9.4f * sc, BR);
            sp.dl(cx - mw * 1.05f, my - 0.75f * sc, cx + mw * 1.05f, my - 0.75f * sc, 0.55f * sc, BR);
        }
    }
    // glasses
    if (f.glasses) {
        u32 GL = f.glasses == 4 ? C(190, 190, 205) : f.glasses == 2 ? C(HOT) : C(40, 30, 40);
        float r = ew * 0.62f;
        for (int s = -1; s <= 1; s += 2) {
            float x = cx + s * ex;
            for (int i = 0; i < 12; i++) { float a0 = i * 0.5236f, a1 = a0 + 0.5236f; float ry2 = f.glasses == 3 ? r * 0.75f : r * 0.85f; sp.dl(x + std::cos(a0) * r, ey + std::sin(a0) * ry2, x + std::cos(a1) * r, ey + std::sin(a1) * ry2, 0.35f * sc, GL); }
            if (f.glasses == 2) sp.dl(x + s * r, ey - 0.6f * sc, x + s * (r + 0.9f * sc), ey - 1.6f * sc, 0.45f * sc, GL);
        }
        sp.dl(cx - ex + r, ey, cx + ex - r, ey, 0.35f * sc, GL);
    }
}

// hair strands & gloss over the hair shapes (fill pass only)
static void hairDetail(SP& sp, int st, float cx, float hc, float H, RGB hair, bool back, float t) {
    const u32 STR = C(shade(hair, 0.68f), 200), GLO = C(shade(hair, 1.55f), 210);
    switch (st) {
    case 0: case 1: case 26: case 25: case 21: case 3:
        if (back) {
            float bot = st == 3 ? 0.9f : st == 21 ? 1.1f : st == 25 ? 1.6f : st == 1 ? 2.0f : 2.4f;
            for (int s = -1; s <= 1; s += 2) for (int i = 0; i < 3; i++) {
                float x0 = cx + s * (0.98f + i * 0.08f) * H, x1 = cx + s * (1.0f + i * 0.12f) * H;
                sp.dl(x0, hc + 0.2f * H, x1, hc + bot * H * (0.75f + i * 0.08f), 0.12f * H * 0.6f, STR);
            }
        } else if (st == 0 || st == 26 || st == 1) {
            for (int s = -1; s <= 1; s += 2) { sp.dl(cx + s * 0.98f * H, hc - 0.1f * H, cx + s * 1.02f * H, hc + 0.85f * H, 0.07f * H, STR); }
        }
        break;
    case 7: case 11:   // braid / loc texture
        if (back) {
            int n = st == 7 ? 7 : 6;
            float len = st == 7 ? 2.6f : 2.3f;
            for (int i = 0; i < n; i++) {
                float u = (i / (float)(n - 1)) - 0.5f, x0 = cx + u * 2.0f * H, sw = std::sin(t * 1.8f + i) * 0.05f * H, x1 = x0 + u * 0.5f * H + sw;
                for (int j = 1; j < 8; j++) { float v = j / 8.f; float bx = x0 + (x1 - x0) * v, by = hc - 0.2f * H + (len + 0.2f) * H * v; sp.dl(bx - 0.1f * H, by - 0.06f * H, bx + 0.1f * H, by + 0.06f * H, 0.05f * H, STR); }
            }
        }
        break;
    case 6: case 8: case 18: case 9: case 22: case 15:   // curls / coils: little highlight flecks
        if (!back || st == 6 || st == 8) for (int i = 0; i < 7; i++) { float a = 3.14159f * (0.15f + i * 0.12f); float r = (st == 8 ? 1.25f : 1.0f) * H; sp.dl(cx - std::cos(a) * r, hc - 0.2f * H - std::sin(a) * r, cx - std::cos(a) * r + 0.12f * H, hc - 0.2f * H - std::sin(a) * r + 0.05f * H, 0.08f * H, GLO); }
        break;
    default: break;
    }
    // the classic glossy shine band across the crown
    if (!back && st != 24 && st != 20 && st != 16 && st != 17 && st != 6 && st != 8) {
        const u32 G2 = C(shade(hair, 1.3f), 95);
        for (int i = 0; i < 7; i++) {
            if (i == 3) continue;
            float a0 = 3.14159f * (0.22f + i * 0.05f), a1 = a0 + 3.14159f * 0.05f, r = 0.86f * H;
            sp.dl(cx - std::cos(a0) * r, hc - 0.05f * H - std::sin(a0) * r, cx - std::cos(a1) * r, hc - 0.05f * H - std::sin(a1) * r, 0.11f * H, G2);
        }
        (void)GLO;
    }
}

static void drawDoll(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip, float bustCut) {
    const bool sitSleep = pose == PO_SITSLEEP;
    if (sitSleep) pose = PO_SLEEP;
    if (pose == PO_LIE) pose = PO_IDLE;
    const bool sitting = pose == PO_SIT || sitSleep;
    const bool walking = pose == PO_WALK;
    const bool kid = d.age == KID, guy = d.guy;
    const DollSkel k = dollSkel(d.age, guy);
    const float ph = g_walkPhase * 6.2832f;
    float bob = pose == PO_DANCE ? -std::fabs(std::sin(t * 7.f)) * 2.5f : walking ? -std::fabs(std::sin(ph)) * 1.2f : std::sin(t * 1.6f + d.skin) * 0.4f;
    const bool bust = bustCut < 1e8f;
    if (sitting || bust) bob = 0;
    Pen pen(x, (sitting ? footY + (120 - k.hipJ) * s - 2 * s : footY) + bob * s, 40, 120, s, flip);
    const u32 INK = C(52, 30, 46);
    SP sp(pen, INK, 0.62f / std::fmax(0.3f, s));
    sp.cutY = bustCut;
    const Wear w = resolveWear(d, outfit);
    const RGB skin = skinRGB(d.skin), hair = hairRGB(d.hairCol);
    const u32 SK = C(skin), SKD = C(shade(skin, 0.86f)), SKH = C(shade(skin, 1.12f)), HR = C(hair), HRD = C(shade(hair, 0.7f));
    const float cx = 20;
    const int mood = pose == PO_SLEEP ? 7 : pose == PO_DANCE && g_stickMood == 0 ? 1 : g_stickMood;
    const bool blink = std::fmod(t * 0.9f + d.skin * 0.37f + d.hairCol * 0.19f, 3.9f) < 0.12f;
    const int hs = d.hairStyle % NUM_HAIRSTYLES;
    const float H = k.H, hc = k.hy - 0.4f;
    const bool heels = !kid && (w.ss == SS_HEELS || w.ss == SS_PLATFORMS || (w.ss == SS_BOOTS && !guy));

    // ---- joints & pose
    const float shY = k.sh + 2.f, shL = cx - k.shJ, shR = cx + k.shJ;
    const float up = k.armLen * 0.48f, fo = k.armLen * 0.46f;
    float eLx = shL - 1.6f, eLy = shY + up, hLx = shL - 2.2f, hLy = shY + up + fo;
    float eRx = shR + 1.6f, eRy = shY + up, hRx = shR + 2.2f, hRy = shY + up + fo;
    bool handOnHip = false;
    if (walking) {
        float sw = std::sin(ph) * fo * 0.42f;
        hLx += sw; eLx += sw * 0.4f; hRx -= sw; eRx -= sw * 0.4f;
    } else if (pose == PO_WAVE) {
        float wv = std::sin(t * 9.f) * 2.2f;
        eRx = shR + 6.f; eRy = shY + 4.f; hRx = shR + 7.f + wv; hRy = shY - up * 0.7f;
    } else if (pose == PO_DANCE) {
        float a = std::sin(t * 7.f);
        eLx = shL - 6.f; eLy = shY + 2 + a * 2; hLx = shL - 6.5f; hLy = shY - up * 0.6f + a * 4;
        eRx = shR + 6.f; eRy = shY + 2 - a * 2; hRx = shR + 6.5f; hRy = shY - up * 0.6f - a * 4;
    } else if (pose == PO_PHONE) {
        eRx = shR + 4.f; eRy = shY + up * 0.85f; hRx = cx + 5.0f; hRy = k.hy + 4.f;
    } else if (sitting) {
        eLx = shL - 1.f; eLy = shY + up; hLx = cx - 4.f; hLy = k.hipJ + 1.f;
        eRx = shR + 1.f; eRy = shY + up; hRx = cx + 4.f; hRy = k.hipJ + 1.f;
    } else if ((pose == PO_IDLE) && !kid) {   // elegant fashion pose: hand on the hip
        handOnHip = true;
        eLx = shL - 5.5f; eLy = shY + up * 0.82f; hLx = cx - rowW(k, k.waistY + 3) - 0.8f; hLy = k.waistY + 3.f;
    }
    const float hjL = cx - 3.9f * (kid ? 0.8f : 1.f), hjR = cx + 3.9f * (kid ? 0.8f : 1.f);
    float kLx = hjL - 0.3f, kLy = k.kneeY, aLx = hjL + 0.8f, aLy = k.ankleY;
    float kRx = hjR + 0.3f, kRy = k.kneeY, aRx = hjR - 0.8f, aRy = k.ankleY;
    if (walking) {   // a runway step: feet alternate, the lifted knee comes forward
        float sw = std::sin(ph), liftL = std::fmax(0.f, sw) * 4.f, liftR = std::fmax(0.f, -sw) * 4.f;
        aLx = hjL + 0.8f + sw * 2.2f; aLy = k.ankleY - liftL; aRx = hjR - 0.8f - sw * 2.2f; aRy = k.ankleY - liftR;
        kLx = (hjL + aLx) / 2 + liftL * 0.7f; kLy = k.kneeY - liftL * 0.6f; kRx = (hjR + aRx) / 2 + liftR * 0.7f; kRy = k.kneeY - liftR * 0.6f;
    } else if (pose == PO_DANCE) {
        float a = std::sin(t * 7.f) * 1.4f;
        kLx = hjL - 2.f; kLy = k.kneeY + a; kRx = hjR + 2.f; kRy = k.kneeY - a; aLx = hjL - 2.f; aRx = hjR + 2.f;
    } else if (sitting) {
        kLx = hjL - 0.5f; kLy = k.hipJ + 4.5f; aLx = hjL - 0.2f; aLy = k.hipJ + 4.5f + (k.ankleY - k.kneeY);
        kRx = hjR + 0.5f; kRy = k.hipJ + 4.5f; aRx = hjR + 0.2f; aRy = k.hipJ + 4.5f + (k.ankleY - k.kneeY);
    } else if (!kid && pose == PO_IDLE) {   // a model's stance: one knee crossing in
        kRx = hjR - 0.8f; aRx = hjR - 2.6f;
    }

    // ---- back hair (big & glossy)
    grp(sp, [&] {
        hairBack(sp, hs, cx, hc, H * 1.08f, HR, t);
        if (w.ts == TS_HOODIE || w.ts == TS_RAINCOAT) sp.el(cx, k.sh - 1.f, 2.0f * H, 0.75f * H, C(shade(w.top, 0.85f)));
    });
    hairDetail(sp, hs, cx, hc, H * 1.08f, hair, true, t);

    // ---- torso skin + neck
    grp(sp, [&] {
        taper(sp, cx, k.hy + k.ry * 0.6f, (guy ? 3.8f : 3.0f) * (kid ? 1.2f : 1.f), cx, k.sh + 0.5f, (guy ? 4.4f : 3.4f) * (kid ? 1.2f : 1.f), SK);
        band(sp, k, k.rowsY[0], k.rowsY[k.nRows - 1], 0, SK);
        sp.ci(shL + 0.4f, shY - 0.4f, k.upperW * 0.72f, SK); sp.ci(shR - 0.4f, shY - 0.4f, k.upperW * 0.72f, SK);
    });
    if (!sp.ink) { sp.fq(cx - 1.6f, k.hy + k.ry * 0.62f, cx + 1.8f, k.hy + k.ry * 0.62f, cx + 1.6f, k.hy + k.ry * 0.95f, cx - 1.6f, k.hy + k.ry * 0.95f, SKD); }   // shadow under the chin
    if (!guy && !kid) sp.dl(cx - 2.2f, k.sh + 1.0f, cx - 3.6f, k.sh + 1.6f, 0.3f, C(shade(skin, 0.75f), 150));   // collarbones
    if (!guy && !kid) sp.dl(cx + 2.2f, k.sh + 1.0f, cx + 3.6f, k.sh + 1.6f, 0.3f, C(shade(skin, 0.75f), 150));

    const bool dress = w.ts == TS_DRESS;
    const int bt = w.bt;
    // ---- legs, bottoms, shoes
    if (!bust) {
        const RGB B = w.bot;
        const u32 BC = C(B), BCD = C(shade(B, 0.8f)), BCL = C(shade(B, 1.2f));
        auto leg = [&](float hx, float kx, float ky, float ax, float ay, u32 c, float grow) {
            float cy2 = ky + (ay - ky) * 0.28f, cx2 = kx + (ax - kx) * 0.28f;
            taper(sp, hx, k.hipJ - 1, k.thighW + grow, kx, ky, k.kneeW + grow, c);
            taper(sp, kx, ky, k.kneeW + grow, cx2, cy2, k.calfW + grow, c);
            taper(sp, cx2, cy2, k.calfW + grow, ax, ay, k.ankleW + grow, c);
        };
        auto legShade = [&](float hx, float kx, float ky, float ax, float ay, u32 c, float grow) {
            taperShade(sp, hx, k.hipJ - 1, k.thighW + grow, kx, ky, k.kneeW + grow, c);
            taperShade(sp, kx, ky, k.kneeW + grow, ax, ay, k.ankleW + grow, c);
        };
        const bool cover = !dress && (bt == BT_PANTS || bt == BT_JEANS || bt == BT_SWEATS || bt == BT_OVERALLS || bt == BT_GOWN || bt == BT_MAXI);
        if (!cover || walking) {
            grp(sp, [&] { leg(hjL, kLx, kLy, aLx, aLy, SK, 0); });
            legShade(hjL, kLx, kLy, aLx, aLy, SKD, 0);
            grp(sp, [&] { leg(hjR, kRx, kRy, aRx, aRy, SK, 0); });
            legShade(hjR, kRx, kRy, aRx, aRy, SKD, 0);
            if (w.acc & ACC_LEGWARM) { grp(sp, [&] { taper(sp, kLx + (aLx - kLx) * 0.45f, kLy + (aLy - kLy) * 0.45f, k.calfW + 1.6f, aLx, aLy - 1, k.ankleW + 2.2f, C(w.accCol)); taper(sp, kRx + (aRx - kRx) * 0.45f, kRy + (aRy - kRy) * 0.45f, k.calfW + 1.6f, aRx, aRy - 1, k.ankleW + 2.2f, C(w.accCol)); }); }
        }
        // bottoms
        const float hipTop = k.waistY - (bt == BT_JEANS || bt == BT_PANTS ? 0.5f : 1.0f);
        if (!dress) {
            const float wide = bt == BT_SWEATS ? 1.6f : bt == BT_JEANS ? 0.6f : 0.9f;
            if (bt == BT_PANTS || bt == BT_JEANS || bt == BT_SWEATS || bt == BT_OVERALLS) {
                grp(sp, [&] {
                    band(sp, k, hipTop, k.hipJ + 3, 0.4f, BC);
                    leg(hjL, kLx, kLy, aLx, aLy - 0.5f, BC, wide); leg(hjR, kRx, kRy, aRx, aRy - 0.5f, BC, wide);
                    if (bt == BT_PANTS && !kid) { taper(sp, kLx, kLy, k.kneeW + wide, aLx - 0.4f, aLy - 1, k.ankleW + wide + 2.2f, BC); taper(sp, kRx, kRy, k.kneeW + wide, aRx + 0.4f, aRy - 1, k.ankleW + wide + 2.2f, BC); }   // flares
                });
                legShade(hjL, kLx, kLy, aLx, aLy, BCD, wide); legShade(hjR, kRx, kRy, aRx, aRy, BCD, wide);
                bandShade(sp, k, hipTop, k.hipJ + 3, 0.4f, 0.22f, BCD);
                // folds, seams & highlights
                sp.dl(kLx - 0.8f, kLy - 1.5f, kLx + 1.2f, kLy - 0.6f, 0.3f, BCD); sp.dl(kRx - 1.2f, kRy - 1.5f, kRx + 0.8f, kRy - 0.6f, 0.3f, BCD);
                sp.dl(hjR + 1.2f, k.hipJ + 4, kRx + 1.3f, kRy - 4, 0.45f, BCL);
                if (bt == BT_JEANS || bt == BT_OVERALLS) {
                    u32 ST = C(240, 200, 120, 210);
                    sp.dl(cx - rowW(k, k.hipY) + 1.2f, hipTop + 2, cx - rowW(k, k.hipY) + 3.2f, hipTop + 5.5f, 0.3f, ST); sp.dl(cx + rowW(k, k.hipY) - 1.2f, hipTop + 2, cx + rowW(k, k.hipY) - 3.2f, hipTop + 5.5f, 0.3f, ST);
                    sp.dl(cx, hipTop + 0.5f, cx, k.hipJ + 2, 0.3f, ST);
                    sp.dl(hjL - k.thighW * 0.5f - wide * 0.5f + 0.4f, k.hipJ + 2, kLx - k.kneeW * 0.5f - wide * 0.5f + 0.3f, kLy, 0.25f, ST);
                    sp.dl(hjR + k.thighW * 0.5f + wide * 0.5f - 0.4f, k.hipJ + 2, kRx + k.kneeW * 0.5f + wide * 0.5f - 0.3f, kRy, 0.25f, ST);
                }
                if (bt == BT_SWEATS) { sp.dl(hjL - k.thighW * 0.5f - 0.6f, k.hipJ + 1, aLx - k.ankleW * 0.5f - 1.2f, aLy - 2, 0.5f, C(255, 255, 255, 220)); sp.dl(hjR + k.thighW * 0.5f + 0.6f, k.hipJ + 1, aRx + k.ankleW * 0.5f + 1.2f, aRy - 2, 0.5f, C(255, 255, 255, 220)); }
                if (bt != BT_SWEATS) sp.dl(cx - rowW(k, hipTop), hipTop + 0.8f, cx + rowW(k, hipTop), hipTop + 0.8f, 0.45f, BCD);   // waistband
            } else if (bt == BT_SHORTS || bt == BT_SWIM) {
                float frac = bt == BT_SWIM ? (guy ? 0.3f : 0.12f) : 0.27f;
                float lx2 = hjL + (kLx - hjL) * frac, ly2 = k.hipJ + (kLy - k.hipJ) * frac, rx2 = hjR + (kRx - hjR) * frac, ry2 = k.hipJ + (kRy - k.hipJ) * frac;
                grp(sp, [&] { band(sp, k, hipTop, k.hipJ + 3, 0.4f, BC); taper(sp, hjL, k.hipJ, k.thighW + 1.0f, lx2, ly2, k.thighW + 0.4f, BC); taper(sp, hjR, k.hipJ, k.thighW + 1.0f, rx2, ry2, k.thighW + 0.4f, BC); });
                bandShade(sp, k, hipTop, k.hipJ + 3, 0.4f, 0.22f, BCD);
                if (bt == BT_SHORTS) { sp.dl(lx2 - 2.6f, ly2 - 0.6f, lx2 + 2.6f, ly2 - 0.6f, 0.3f, BCD); sp.dl(rx2 - 2.6f, ry2 - 0.6f, rx2 + 2.6f, ry2 - 0.6f, 0.3f, BCD); }
            } else {   // skirts
                float hem = bt == BT_MINI ? k.hipJ + 9 : bt == BT_PLEATED ? k.hipJ + 12 : bt == BT_MAXI ? k.ankleY - 1 : 120.5f;
                float flare = bt == BT_GOWN ? 11.f : bt == BT_MAXI ? 7.f : bt == BT_PLEATED ? 4.5f : 3.f;
                float hw = rowW(k, k.hipY) + 0.6f;
                grp(sp, [&] { band(sp, k, hipTop, k.hipY, 0.4f, BC); sp.qd(cx - hw, k.hipY - 0.2f, cx + hw, k.hipY - 0.2f, cx + hw + flare, hem, cx - hw - flare, hem, BC); });
                if (!sp.ink) {
                    sp.fq(cx - hw, k.hipY, cx - hw * 0.4f, k.hipY, cx - (hw + flare) * 0.45f, hem, cx - hw - flare, hem, BCD);
                    int nf = bt == BT_PLEATED ? 5 : bt == BT_GOWN ? 4 : 3;
                    for (int i = 0; i < nf; i++) { float u = (i + 1.f) / (nf + 1.f) - 0.5f; sp.dl(cx + u * hw * 1.6f, k.hipY + 1.5f, cx + u * (hw + flare) * 1.9f, hem - 0.6f, bt == BT_PLEATED ? 0.35f : 0.3f, BCD); }
                    sp.dl(cx + hw * 0.55f, k.hipY + 1.5f, cx + (hw + flare) * 0.7f, hem - 1.5f, 0.5f, BCL);
                    if (bt == BT_GOWN) for (int i = 0; i < 7; i++) sp.p.Sp(cx - 6 + (i * 5) % 13, k.hipY + 8 + i * 7.f, 0.9f, C(255, 255, 255, 210));
                }
            }
        }
        // shoes
        const u32 SHC = C(w.shoes), SHD = C(shade(w.shoes, 0.72f)), SHL = C(shade(w.shoes, 1.3f));
        const float tw = sitting ? 0.f : 1.f;
        auto shoe = [&](float ax, float ay, bool front) {
            const float gy = ay + (120 - k.ankleY);   // ground under this foot
            switch (heels ? (w.ss == SS_BOOTS ? (int)SS_BOOTS : w.ss == SS_PLATFORMS ? (int)SS_PLATFORMS : (int)SS_HEELS) : (int)w.ss) {
            case SS_HEELS:
                sp.qd(ax - 1.4f, ay - 0.6f, ax + 1.6f, ay - 0.2f, ax + 4.2f * tw + 0.6f, gy, ax + 0.4f, gy - 0.4f, SHC);
                sp.ln(ax - 1.2f, ay, ax - 1.1f, gy, 0.8f, SHC);
                break;
            case SS_PLATFORMS:
                sp.rc(ax - 2.2f, gy - 2.4f, 5.8f + 1.6f * tw, 2.8f, C(shade(w.shoes, 0.9f)));
                sp.qd(ax - 1.6f, ay - 1.2f, ax + 1.6f, ay - 1.2f, ax + 3.6f * tw + 0.6f, gy - 2.2f, ax - 2.f, gy - 2.2f, SHC);
                break;
            case SS_BOOTS: {
                float top = guy || kid ? ay - 7.f : ay - 17.f;
                float tx2 = ax + (front ? 0 : 0);
                sp.qd(tx2 - 2.4f, top, tx2 + 2.4f, top, tx2 + 1.8f, ay + 0.6f, tx2 - 1.8f, ay + 0.6f, SHC);
                if (heels) { sp.qd(ax - 1.6f, ay - 0.6f, ax + 1.8f, ay - 0.2f, ax + 4.0f * tw + 0.6f, gy, ax + 0.2f, gy - 0.4f, SHC); sp.ln(ax - 1.4f, ay, ax - 1.3f, gy, 1.0f, SHC); }
                else sp.el(ax + 1.4f * tw, gy - 1.3f, 6.4f, 3.2f, SHC);
                break;
            }
            case SS_SNEAKERS:
                sp.el(ax + 1.3f * tw, gy - 1.6f, 6.6f, 3.6f, SHC);
                sp.rc(ax - 2.0f + 1.3f * tw, gy - 0.9f, 6.6f, 1.3f, C(250, 250, 250));
                break;
            case SS_SANDALS: case SS_FLIPFLOPS:
                sp.el(ax + 1.2f * tw, gy - 0.9f, 5.0f, 2.2f, SK);
                sp.rc(ax - 1.8f + 1.2f * tw * 0.5f, gy - 0.6f, 5.4f + tw, 0.9f, SHC);
                break;
            case SS_LOAFERS: sp.el(ax + 1.2f * tw, gy - 1.2f, 6.f, 2.8f, SHC); break;
            case SS_SLIPPERS: sp.el(ax + 1.2f * tw, gy - 1.6f, 7.f, 4.2f, SHC); break;
            default: sp.el(ax + 1.0f * tw, gy - 1.0f, 5.f, 2.4f, SK); break;
            }
        };
        grp(sp, [&] { shoe(aLx, aLy, false); });
        grp(sp, [&] { shoe(aRx, aRy, true); });
        if (!heels && w.ss == SS_SNEAKERS) for (int s2 = 0; s2 < 2; s2++) { float ax = s2 ? aRx : aLx, gy = (s2 ? aRy : aLy) + (120 - k.ankleY); sp.dl(ax + 0.2f, gy - 2.6f, ax + 1.4f, gy - 2.0f, 0.3f, SHD); sp.dl(ax + 0.8f, gy - 2.9f, ax + 2.0f, gy - 2.3f, 0.3f, SHD); sp.p.E(ax + 2.6f * tw, gy - 2.4f, 1.4f, 0.7f, SHL); }
        if (!heels && (w.ss == SS_SANDALS || w.ss == SS_FLIPFLOPS)) for (int s2 = 0; s2 < 2; s2++) { float ax = s2 ? aRx : aLx, gy = (s2 ? aRy : aLy) + (120 - k.ankleY); sp.dl(ax - 0.6f, gy - 2.2f, ax + 2.0f * tw, gy - 0.6f, 0.6f, SHC); }
    }

    // ---- the top
    const RGB TC = w.top;
    const u32 T = C(TC), TD = C(shade(TC, 0.8f)), TL = C(shade(TC, 1.25f));
    const bool longSleeve = w.ts == TS_HOODIE || w.ts == TS_SWEATER || w.ts == TS_JACKET || w.ts == TS_BLAZER || w.ts == TS_CARDIGAN || w.ts == TS_RAINCOAT || w.ts == TS_BUTTON;
    const bool noSleeve = w.ts == TS_TANK || w.ts == TS_TUBE || w.ts == TS_JERSEY || w.ts == TS_DRESS || w.ts == TS_CROP;
    const float baggy = (w.ts == TS_HOODIE || w.ts == TS_RAINCOAT || w.ts == TS_JERSEY || w.ts == TS_SWEATER) ? 1.1f : 0.35f;
    float tTop = (w.ts == TS_TANK || w.ts == TS_TUBE || w.ts == TS_DRESS) ? (kid ? k.sh + 2.f : k.rowsY[3] - 1.2f) : k.rowsY[0];
    if (w.ts == TS_TANK || w.ts == TS_JERSEY) tTop = k.sh + 2.6f;
    float tBot = k.waistY + (guy ? 6.f : 4.5f);
    if (w.ts == TS_CROP) tBot = k.rowsY[std::min(5, k.nRows - 1)] + 0.5f;
    if (w.ts == TS_TUBE) tBot = k.waistY + 1.f;
    if (w.ts == TS_RAINCOAT || w.ts == TS_CARDIGAN || w.ts == TS_BLAZER) tBot = k.hipJ + (w.ts == TS_RAINCOAT ? 12.f : 3.f);
    if (w.ts == TS_HOODIE || w.ts == TS_SWEATER) tBot = k.hipY + 1.5f;
    const float tb = std::fmin(tBot, k.rowsY[k.nRows - 1] - 0.5f);
    grp(sp, [&] {
        if (dress) {
            band(sp, k, tTop, k.waistY + 1, 0.35f, T);
            float hw = rowW(k, k.waistY) + 0.4f, hem = bust ? k.hipJ : (kid ? k.kneeY - 4 : k.kneeY - 7);
            sp.qd(cx - hw, k.waistY, cx + hw, k.waistY, cx + hw + 8.5f, hem, cx - hw - 8.5f, hem, T);
        } else {
            band(sp, k, tTop, tb, baggy, T);
            if (tBot > tb) { float a = rowW(k, tb) + baggy; sp.qd(cx - a, tb - 0.5f, cx + a, tb - 0.5f, cx + a + 1.2f, tBot, cx - a - 1.2f, tBot, T); }
        }
    });
    // straps / sleeves base, shading, details
    if (w.ts == TS_TANK || w.ts == TS_JERSEY || (w.ts == TS_DRESS && !kid)) { sp.dl(cx - 3.6f, tTop + 0.6f, cx - 3.3f, k.sh + 0.2f, 0.9f, T); sp.dl(cx + 3.6f, tTop + 0.6f, cx + 3.3f, k.sh + 0.2f, 0.9f, T); }
    if (!sp.ink) {
        bandShade(sp, k, tTop, dress ? k.waistY : tb, baggy, 0.24f, TD);
        if (dress) { float hw = rowW(k, k.waistY) + 0.4f, hem = bust ? k.hipJ : (kid ? k.kneeY - 4 : k.kneeY - 7); sp.fq(cx - hw, k.waistY, cx - hw * 0.3f, k.waistY, cx - (hw + 8.5f) * 0.4f, hem, cx - hw - 8.5f, hem, TD); for (int i = 0; i < 3; i++) { float u = (i + 1) / 4.f - 0.5f; sp.dl(cx + u * hw * 1.4f, k.waistY + 2, cx + u * (hw + 8.5f) * 1.8f, hem - 0.5f, 0.3f, TD); } sp.dl(cx - hw, k.waistY + 0.3f, cx + hw, k.waistY + 0.3f, 1.0f, C(w.bot)); }
        sp.dl(cx + rowW(k, tTop + 3) * 0.55f, tTop + 2.5f, cx + rowW(k, tb - 2) * 0.62f, tb - 2.f, 0.45f, TL);   // highlight
        if (!guy && !kid && !dress && w.ts != TS_HOODIE && w.ts != TS_JERSEY) { sp.dl(cx - 4.2f, k.rowsY[4] + 1.6f, cx - 1.6f, k.rowsY[4] + 2.4f, 0.3f, TD); sp.dl(cx + 1.6f, k.rowsY[4] + 2.4f, cx + 4.2f, k.rowsY[4] + 1.6f, 0.3f, TD); }   // bust fold
        sp.dl(cx - rowW(k, k.waistY) - baggy + 0.4f, k.waistY - 1.f, cx - rowW(k, k.waistY) - baggy + 2.0f, k.waistY + 0.4f, 0.3f, TD);   // waist fold
    }
    if (w.ts == TS_HOODIE) { sp.dl(cx - 1.0f, k.sh + 1, cx - 1.3f, k.sh + 8, 0.4f, C(255, 255, 255)); sp.dl(cx + 1.0f, k.sh + 1, cx + 1.3f, k.sh + 8, 0.4f, C(255, 255, 255)); if (!sp.ink) sp.fq(cx - 4.2f, tBot - 8, cx + 4.2f, tBot - 8, cx + 5.2f, tBot - 2.5f, cx - 5.2f, tBot - 2.5f, TD); sp.dl(cx - rowW(k, tBot) - baggy, tBot - 1.2f, cx + rowW(k, tBot) + baggy, tBot - 1.2f, 0.8f, TD); }
    if (w.ts == TS_JERSEY && !bust && s >= 0.6f) { sp.dl(cx - 2.4f, k.rowsY[3], cx - 2.4f, k.rowsY[3] + 7, 1.3f, C(255, 255, 255)); sp.dl(cx + 0.4f, k.rowsY[3], cx + 3.f, k.rowsY[3], 1.1f, C(255, 255, 255)); sp.dl(cx + 3.f, k.rowsY[3], cx + 0.6f, k.rowsY[3] + 7, 1.1f, C(255, 255, 255)); }
    if (w.ts == TS_BUTTON || w.ts == TS_POLO) {
        if (!sp.ink) { sp.ft(cx - 3.0f, k.sh - 0.6f, cx, k.sh + 2.2f, cx - 1.0f, k.sh - 1.0f, C(255, 255, 255)); sp.ft(cx + 3.0f, k.sh - 0.6f, cx, k.sh + 2.2f, cx + 1.0f, k.sh - 1.0f, C(255, 255, 255)); }
        sp.dl(cx, k.sh + 2.2f, cx, tBot - 0.5f, 0.3f, TD);
        for (int i = 0; i < (w.ts == TS_POLO ? 2 : 4); i++) sp.dot(cx + 0.5f, k.sh + 4 + i * 4.5f, 0.42f, TL);
    }
    if (w.ts == TS_SWEATER) { for (int i = 0; i < 5; i++) sp.dl(cx - 4 + i * 2.f, tBot - 2.2f, cx - 4 + i * 2.f, tBot - 0.4f, 0.3f, TD); sp.dl(cx - 2.6f, k.sh + 0.2f, cx + 2.6f, k.sh + 0.2f, 1.0f, TD); }
    if (w.ts == TS_JACKET || w.ts == TS_BLAZER || w.ts == TS_CARDIGAN) {
        RGB inner = w.ts == TS_CARDIGAN ? RGB{250, 236, 216} : RGB{255, 255, 255};
        if (!sp.ink) sp.fq(cx - 2.4f, k.sh, cx + 2.4f, k.sh, cx + 2.0f, std::fmin(tBot, k.hipY) - 0.6f, cx - 2.0f, std::fmin(tBot, k.hipY) - 0.6f, C(inner));
        sp.dl(cx - 2.4f, k.sh, cx - 0.5f, k.sh + 9, 0.7f, TD); sp.dl(cx + 2.4f, k.sh, cx + 0.5f, k.sh + 9, 0.7f, TD);
        if (w.ts == TS_BLAZER && guy && !sp.ink) sp.fq(cx - 0.7f, k.sh + 1, cx + 0.7f, k.sh + 1, cx + 1.0f, k.sh + 13, cx - 1.0f, k.sh + 13, C(HOT));
        if (w.ts == TS_JACKET) { sp.dl(cx - 6.0f, k.sh + 6, cx - 2.6f, k.sh + 6, 0.3f, C(240, 200, 120, 200)); sp.dl(cx + 2.6f, k.sh + 6, cx + 6.0f, k.sh + 6, 0.3f, C(240, 200, 120, 200)); }
    }
    if (w.ts == TS_RAINCOAT) for (int i = 0; i < 4; i++) sp.dot(cx + 1.2f, k.sh + 4 + i * 6.f, 0.6f, TD);
    if (w.ts == TS_CROP || w.ts == TS_TUBE) sp.dl(cx - rowW(k, tBot) - 0.6f, tBot - 0.6f, cx + rowW(k, tBot) + 0.6f, tBot - 0.6f, 0.4f, TL);
    if (w.pat && !dress && !sp.ink) {
        const float y0 = tTop + 2, y1 = tb - 2, ww = rowW(k, (y0 + y1) / 2);
        switch (w.pat) {
        case 1: for (int i = 0; i < 4; i++) sp.dl(cx - ww, y0 + i * 3.6f, cx + ww - 1, y0 + i * 3.6f + 1.2f, 0.9f, C(BLACK)); break;
        case 2: for (int i = 0; i < 8; i++) { sp.p.Ci(cx - 4 + (i % 4) * 2.7f, y0 + 1 + (i / 4) * 5.f + (i % 2), 0.75f, C(80, 50, 30)); sp.p.Ci(cx - 4 + (i % 4) * 2.7f + 0.2f, y0 + 1 + (i / 4) * 5.f + (i % 2), 0.35f, C(190, 130, 60)); } break;
        case 3: for (int i = 0; i < 5; i++) sp.dl(cx - ww, y0 + i * 3.4f, cx + ww, y0 + i * 3.4f, 1.1f, C(255, 255, 255, 170)); break;
        case 4: for (int i = 0; i < 4; i++) { sp.dl(cx - ww, y0 + i * 4.f, cx + ww, y0 + i * 4.f, 0.45f, C(BLACK, 110)); sp.dl(cx - 4.5f + i * 3.f, y0 - 1, cx - 4.5f + i * 3.f, y1, 0.45f, C(200, 40, 50, 160)); } break;
        case 5: for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if ((r + c) % 2) sp.fr(cx - 5.f + c * 2.5f, y0 + r * 2.5f, 2.5f, 2.5f, C(BLACK, 200)); break;
        case 6: for (int i = 0; i < 4; i++) sp.p.H(cx - 2.2f + (i % 2) * 4.4f, y0 + 2.5f + (i / 2) * 5.5f, 0.9f, C(255, 255, 255, 220)); break;
        case 7: for (int i = 0; i < 8; i++) sp.p.Ci(cx - 4 + (i % 4) * 2.7f, y0 + 1.5f + (i / 4) * 4.5f, 0.38f, C(255, 255, 255, 240)); sp.p.Sp(cx, y0 + 4, 1.6f, C(255, 255, 255, 230)); break;
        case 8: sp.p.H(cx, y0 + 4.5f, 2.2f, C(HOT)); sp.p.H(cx, y0 + 4.2f, 1.0f, C(255, 170, 210)); sp.p.Sp(cx + 2.6f, y0 + 1.5f, 1.2f, C(GOLD)); break;
        }
    }
    if (bt == BT_OVERALLS && !dress && !bust) {
        grp(sp, [&] { sp.rc(cx - 3.8f, k.rowsY[4], 7.6f, k.hipY - k.rowsY[4], C(w.bot)); });
        sp.dl(cx - 3.4f, k.rowsY[4] + 0.5f, cx - 3.6f, k.sh, 1.0f, C(w.bot)); sp.dl(cx + 3.4f, k.rowsY[4] + 0.5f, cx + 3.6f, k.sh, 1.0f, C(w.bot));
        sp.dot(cx - 2.6f, k.rowsY[4] + 1.2f, 0.55f, C(GOLD)); sp.dot(cx + 2.6f, k.rowsY[4] + 1.2f, 0.55f, C(GOLD));
        if (!sp.ink) sp.fr(cx - 2.2f, k.rowsY[4] + 3, 4.4f, 3.5f, C(shade(w.bot, 0.85f)));
    }
    if (w.acc & ACC_FLANNEL) grp(sp, [&] { band(sp, k, k.hipY - 2.5f, k.hipY, 0.9f, C(w.accCol)); sp.tr(cx - rowW(k, k.hipY), k.hipY, cx - rowW(k, k.hipY) + 4, k.hipY, cx - rowW(k, k.hipY) + 1, k.hipY + 11, C(w.accCol)); });
    // jewellery on the chest
    if (w.acc & ACC_PEARLS) for (int i = 0; i < 9; i++) { float u = i / 8.f - 0.5f; sp.dot(cx + u * 6.f, k.sh + 1.2f + (0.25f - u * u) * 5.f, 0.5f, C(255, 252, 240)); }
    if (w.acc & ACC_NECKLACE) {
        for (int L = 0; L < 2; L++) for (int i = 0; i < 8; i++) { float u0 = i / 8.f - 0.5f, u1 = (i + 1) / 8.f - 0.5f, dep = 3.2f + L * 2.6f; sp.dl(cx + u0 * (5 + L * 2), k.sh + 0.5f + (0.25f - u0 * u0) * dep * 4, cx + u1 * (5 + L * 2), k.sh + 0.5f + (0.25f - u1 * u1) * dep * 4, 0.3f, C(GOLD)); }
        sp.p.H(cx, k.sh + 1.6f + 5.6f * 1.0f + 1.2f, 0.8f, C(HOT));
    }
    if ((w.acc & ACC_CHOKER) && !sp.ink) { sp.dl(cx - 1.7f, k.hy + k.ry * 1.25f, cx + 1.7f, k.hy + k.ry * 1.25f, 0.9f, C(BLACK)); sp.dot(cx, k.hy + k.ry * 1.25f + 0.8f, 0.5f, C(HOT)); }

    // ---- arms (skin, then sleeves)
    auto arm = [&](float sx, float ex2, float ey2, float hx, float hy, bool onHip) {
        grp(sp, [&] {
            taper(sp, sx, shY - 0.5f, k.upperW, ex2, ey2, k.upperW * 0.82f, SK);
            sp.ci(ex2, ey2, k.upperW * 0.46f, SK);
            taper(sp, ex2, ey2, k.upperW * 0.82f, hx, hy, k.foreW * 0.85f, SK);
            // hand: an elegant little mitten + thumb
            float dx = hx - ex2, dy = hy - ey2, L = std::sqrt(dx * dx + dy * dy); if (L < 0.01f) L = 0.01f;
            float ux = dx / L, uy = dy / L;
            if (onHip) sp.el(hx - 0.4f, hy + 0.6f, 2.6f, 3.4f, SK);
            else { sp.el(hx + ux * 1.5f, hy + uy * 1.5f, 2.6f + std::fabs(ux) * 0.8f, 2.6f + std::fabs(uy) * 1.2f, SK); sp.ln(hx, hy, hx + ux * 1.4f - uy * 1.2f, hy + uy * 1.4f + ux * 1.2f, 0.9f, SK); }
        });
        taperShade(sp, sx, shY - 0.5f, k.upperW, ex2, ey2, k.upperW * 0.82f, SKD);
        if (!noSleeve) {
            grp(sp, [&] {
                float g = w.ts == TS_HOODIE || w.ts == TS_RAINCOAT || w.ts == TS_SWEATER ? 1.5f : 0.9f;
                if (longSleeve) {
                    taper(sp, sx, shY - 0.8f, k.upperW + g + 0.4f, ex2, ey2, k.upperW * 0.82f + g, T);
                    float u = 0.86f;
                    taper(sp, ex2, ey2, k.foreW + g + 0.3f, ex2 + (hx - ex2) * u, ey2 + (hy - ey2) * u, k.foreW + g + (w.ts == TS_BUTTON ? 0.f : 0.6f), T);
                } else {
                    float u = 0.5f;
                    taper(sp, sx, shY - 0.8f, k.upperW + g + 0.6f, sx + (ex2 - sx) * u, shY + (ey2 - shY) * u, k.upperW + g + 0.8f, T);
                }
            });
            if (longSleeve) { sp.dl(ex2 - 1.0f, ey2 - 0.5f, ex2 + 1.0f, ey2 + 0.8f, 0.3f, TD); taperShade(sp, sx, shY - 0.8f, k.upperW + 1.2f, ex2, ey2, k.upperW, TD); }
        }
        if ((w.acc & ACC_BANGLES) && !sp.ink) for (int i = 0; i < 3; i++) { float u = 0.78f - i * 0.06f; float bx = ex2 + (hx - ex2) * u, by = ey2 + (hy - ey2) * u; sp.p.E(bx, by, k.foreW + 1.4f, 0.9f, C(i == 1 ? HOT : GOLD)); }
    };
    arm(shL, eLx, eLy, hLx, hLy, handOnHip);
    if (pose != PO_PHONE) arm(shR, eRx, eRy, hRx, hRy, false);
    // handbag on the relaxed arm
    if ((w.acc & ACC_BAG) && !bust && pose != PO_DANCE && !sitting) {
        RGB bc = (w.accCol.r > 200 && w.accCol.g > 200 && w.accCol.b > 200) ? RGB{255, 90, 160} : w.accCol;
        float bx = handOnHip ? eLx - 1.f : hRx, by = handOnHip ? eLy + 1.f : hRy - 1.f;
        grp(sp, [&] { sp.ln(bx - 2.2f, by + 3.f, bx, by - 1.5f, 0.6f, C(shade(bc, 0.7f))); sp.ln(bx + 2.2f, by + 3.f, bx, by - 1.5f, 0.6f, C(shade(bc, 0.7f))); sp.qd(bx - 3.6f, by + 2.6f, bx + 3.6f, by + 2.6f, bx + 4.2f, by + 8.4f, bx - 4.2f, by + 8.4f, C(bc)); });
        if (!sp.ink) { sp.dl(bx - 3.8f, by + 4.2f, bx + 3.8f, by + 4.2f, 0.4f, C(shade(bc, 0.7f))); sp.p.Ci(bx, by + 4.6f, 0.6f, C(GOLD)); sp.dl(bx + 1.5f, by + 3.4f, bx + 3.0f, by + 7.4f, 0.4f, C(shade(bc, 1.3f))); }
    }

    // ---- head
    const float hl = hairlineOf(hs);
    grp(sp, [&] {
        sp.el(k.hx, k.hy, k.rx * 2, k.ry * 2, SK);
        if (guy) sp.qd(k.hx - k.rx * 0.95f, k.hy + 1.5f, k.hx + k.rx * 0.95f, k.hy + 1.5f, k.hx + k.rx * 0.55f, k.hy + k.ry * 0.95f, k.hx - k.rx * 0.55f, k.hy + k.ry * 0.95f, SK);
        else sp.tr(k.hx - k.rx * 0.82f, k.hy + k.ry * 0.38f, k.hx + k.rx * 0.82f, k.hy + k.ry * 0.38f, k.hx, k.hy + k.ry * 1.06f, SK);
        sp.el(k.hx - k.rx * 0.98f, k.hy + 1.2f, 1.8f, 3.2f, SK); sp.el(k.hx + k.rx * 0.98f, k.hy + 1.2f, 1.8f, 3.2f, SK);
    });

    // hair cap (& top pieces), then the face over it
    grp(sp, [&] {
        if (hl < 5.f) {
            const float R = 1.07f * H, ccy = hc - 0.03f * H, fx = cx, fy = hc + 0.3f * H;
            const int N = 12;
            for (int i = 0; i < N; i++) {
                float a0 = 3.14159f * (-0.1f + 1.2f * i / N), a1 = 3.14159f * (-0.1f + 1.2f * (i + 1) / N);
                sp.tr(fx, fy, cx - std::cos(a0) * R, ccy - std::sin(a0) * R, cx - std::cos(a1) * R, ccy - std::sin(a1) * R, HR);
            }
        }
        hairFront(sp, hs, cx, hc, H, HR, HRD, t, 0);
    });
    if (hl < 5.f) {   // the face: skin from the hairline down, so the cap reads as hair
        const float top = hc + hl * H, bot = k.hy + k.ry * 0.5f;
        sp.p.E(k.hx, (top + bot) * 0.5f, k.rx * 1.92f, bot - top, SK);
        const float ry = (bot - top) * 0.5f, rx = k.rx * 0.96f, ccy = (top + bot) * 0.5f;
        for (int i = 0; i < 8; i++) { float a0 = 3.14159f * (1.12f + i * 0.0975f), a1 = a0 + 3.14159f * 0.0975f; sp.dl(k.hx + std::cos(a0) * rx, ccy + std::sin(a0) * ry, k.hx + std::cos(a1) * rx, ccy + std::sin(a1) * ry, 0.28f, C(shade(hair, 0.6f))); }
    }
    hairDetail(sp, hs, cx, hc, H, hair, false, t);
    if (hl < 5.f && hs != 16 && hs != 17 && !sp.ink) {   // sections combed from the part, so it reads as hair, not a helmet
        const u32 SEC = C(shade(hair, 0.68f), 170);
        const float pX = cx + 0.25f * H, pY = hc - 1.02f * H;
        for (int i = 0; i < 6; i++) {
            float a = 3.14159f * (0.12f + i * 0.15f);
            float ex = cx - std::cos(a) * 0.97f * H, ey = hc - 0.05f * H - std::sin(a) * 0.62f * H;
            float mx = (pX + ex) * 0.5f - std::cos(a) * 0.12f * H, my = (pY + ey) * 0.5f - 0.1f * H;
            sp.dl(pX, pY, mx, my, 0.06f * H, SEC); sp.dl(mx, my, ex, ey, 0.05f * H, SEC);
        }
    }
    if (hl < 5.f && hs != 5 && hs != 16 && hs != 17 && hs != 19 && hs != 27 && hs != 14 && hs != 24)
        grp(sp, [&] { for (int s2 = -1; s2 <= 1; s2 += 2) sp.tr(cx + s2 * 0.7f * H, hc - 0.62f * H, cx + s2 * 1.04f * H, hc - 0.45f * H, cx + s2 * 0.9f * H, hc + 0.12f * H, HR); });
    // earrings
    const bool hoops = ((d.f.pierce == 2 || d.f.pierce == 4 || (w.acc & ACC_HOOPS)) && !kid);
    if (hoops) for (int s2 = -1; s2 <= 1; s2 += 2) for (int i = 0; i < 10; i++) { float a0 = i * 0.628f, a1 = a0 + 0.628f; float ex = k.hx + s2 * k.rx * 0.98f, ey = k.hy + 4.2f; float r = (w.acc & ACC_HOOPS) ? 1.9f : 1.3f; sp.dl(ex + std::cos(a0) * r, ey + std::sin(a0) * r, ex + std::cos(a1) * r, ey + std::sin(a1) * r, 0.35f, C(GOLD)); }
    else if (d.f.pierce == 1 || d.f.pierce == 3) { sp.dot(k.hx - k.rx * 0.98f, k.hy + 2.8f, 0.5f, C(GOLD)); sp.dot(k.hx + k.rx * 0.98f, k.hy + 2.8f, 0.5f, C(GOLD)); }
    dollFace(sp, d, k, mood, blink, walking ? 1.f : 0.3f, t);
    if (d.f.pierce >= 3) sp.dot(k.hx - 0.9f, k.hy + 4.1f, 0.3f, C(235, 235, 245));
    // front hair (bangs & framing strands), hats, accessories
    grp(sp, [&] { hairFront(sp, hs, cx, hc, H, HR, HRD, t, 1); });
    if ((w.acc & ACC_STREAKS) && !sp.ink) { sp.dl(cx - 0.5f * H, hc - 0.95f * H, cx - 0.8f * H, hc + 0.6f * H, 0.14f * H, C(w.accCol)); sp.dl(cx + 0.45f * H, hc - 0.95f * H, cx + 0.8f * H, hc + 0.4f * H, 0.12f * H, C(80, 200, 255)); }
    int hat = w.hat;
    if (!hat && (w.acc & ACC_BEANIE)) hat = HT_BEANIE;
    if (!hat && (w.acc & ACC_BOW)) hat = HT_BOW;
    if (!hat && (w.acc & ACC_TIARA)) hat = HT_TIARA;
    RGB hatCol = (w.acc & (ACC_BEANIE | ACC_BOW)) ? w.accCol : hat == HT_BOW || hat == HT_HEADBAND ? RGB{255, 90, 160} : shade(w.top, 0.95f);
    if (holiday) grp(sp, [&] { holidayHat(sp, holiday, cx, hc, H); });
    else if (hat) grp(sp, [&] { drawHatS(sp, hat, cx, hc, H * 1.04f, hatCol, t); });
    if (w.acc & ACC_CLIPS) for (int s2 = -1; s2 <= 1; s2 += 2) { float bx = cx + s2 * 0.55f * H, by = hc - 0.75f * H; sp.ft(bx, by, bx - 1.4f, by - 1.0f, bx - 1.2f, by + 0.9f, C(w.accCol)); sp.ft(bx, by, bx + 1.4f, by - 1.0f, bx + 1.2f, by + 0.9f, C(w.accCol)); }
    if (w.acc & ACC_SHADES) {
        const float sc2 = k.rx / 7.f, ey = k.hy + 0.8f * sc2;
        for (int s2 = -1; s2 <= 1; s2 += 2) { sp.fq(k.hx + s2 * 0.6f, ey - 1.5f, k.hx + s2 * 5.6f * sc2, ey - 2.0f, k.hx + s2 * 5.2f * sc2, ey + 1.6f, k.hx + s2 * 1.2f, ey + 1.7f, C(30, 22, 34, 245)); sp.dl(k.hx + s2 * 4.4f * sc2, ey - 1.0f, k.hx + s2 * 3.0f * sc2, ey + 0.6f, 0.35f, C(255, 255, 255, 150)); }
        sp.dl(k.hx - 0.8f, ey - 1.2f, k.hx + 0.8f, ey - 1.2f, 0.4f, C(30, 22, 34));
    }
    if (outfit == 8 && !guy) grp(sp, [&] { sp.tr(cx - 0.85f * H, hc - 0.7f * H, cx - 0.25f * H, hc - 0.98f * H, cx - 0.75f * H, hc - 1.5f * H, C(40, 30, 40)); sp.tr(cx + 0.85f * H, hc - 0.7f * H, cx + 0.25f * H, hc - 0.98f * H, cx + 0.75f * H, hc - 1.5f * H, C(40, 30, 40)); });
    if (pose == PO_PHONE) {
        arm(shR, eRx, eRy, hRx, hRy, false);
        grp(sp, [&] { sp.rc(hRx - 1.5f, hRy - 4.2f, 3.2f, 5.6f, C(255, 110, 180)); });
        if (!sp.ink) sp.p.Sp(hRx, hRy - 2.f, 0.9f, C(255, 255, 255, 220));
    }
    if (!bust && pose == PO_SLEEP && !g_ink) {
        float zt = std::fmod(t, 2.f);
        text("z", pen.X(cx + H) + zt * 4, pen.Y(hc - H) - zt * 6, 0.4f * s + 0.14f, C(255, 255, 255, (uint8_t)(220 - zt * 100)));
    }
}

void drawStickPerson(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip) {
    if (d.age == BABY) { drawStickBaby(d, outfit, x, footY, s, t, holiday, pose, flip); return; }
    if (artIsDoll(g_artStyle)) { drawDoll(d, outfit, x, footY, s * ageScale(d.age), t, holiday, pose, flip, 1e9f); return; }
    drawFigure(d, outfit, x, footY, s * ageScale(d.age), t, holiday, pose, flip, 1e9f);
}

// Head-and-shoulders close-up centred in a circle of radius r at (cx, cy)
void drawStickBust(const PersonDef& d, int outfit, float cx, float cy, float r, float t) {
    if (d.age == BABY) { drawStickBaby(d, outfit, cx, cy + r * 0.9f, r / 22.f, t, 0, PO_IDLE, false); return; }
    if (artIsDoll(g_artStyle)) {
        const DollSkel k = dollSkel(d.age, d.guy);
        float s = r / (k.ry * 2.1f);
        float footY = cy - r * 0.08f + (120 - k.hy) * s;
        drawDoll(d, outfit, cx, footY, s, t, 0, PO_IDLE, false, k.rowsY[std::min(3, k.nRows - 1)] + 1.5f);
        return;
    }
    const Skel k = skelFor(d.age, d.guy, g_artStyle);
    float s = r / (k.H * 1.75f);
    // put the head centre slightly above the circle centre
    float footY = cy - r * 0.12f + (120 - k.hc) * s;
    drawFigure(d, outfit, cx, footY, s, t, 0, PO_IDLE, false, k.sh + k.T * 0.55f);
}
