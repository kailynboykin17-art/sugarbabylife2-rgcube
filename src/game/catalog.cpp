#include "catalog.hpp"
#include "cc.hpp"
#include "draw.hpp"
#include <cmath>
#include <cstdio>

const RGB PALETTE[NUM_COLORS] = {
    {255, 176, 206},  // Bubblegum
    {246, 70, 145},   // Hot Pink
    {198, 162, 228},  // Lilac
    {164, 206, 240},  // Baby Blue
    {248, 234, 212},  // Cream
    {150, 102, 78},   // Mocha
    {82, 50, 40},     // Chocolate
    {224, 184, 100},  // Gold
    {196, 28, 62},    // Cherry
    {34, 28, 32},     // Black Gloss
    {244, 240, 234},  // Zebra (base)
    {214, 166, 98},   // Leopard (base)
};
const char* COLOR_NAMES[NUM_COLORS] = {"Bubblegum", "Hot Pink", "Lilac", "Baby Blue", "Cream", "Mocha",
                                       "Chocolate", "Gold", "Cherry", "Black Gloss", "Zebra", "Leopard"};

ShapeInfo SHAPES[SH_COUNT] = {
    {"Canopy Bed", false, 110, 64}, {"Loveseat", false, 100, 44},  {"Accent Chair", false, 38, 52},
    {"Coffee Table", false, 70, 30}, {"Floor Lamp", false, 24, 74}, {"Palm Plant", false, 34, 56},
    {"Shag Rug", false, 120, 16},    {"Flat TV", false, 72, 58},     {"Bookcase", false, 50, 90},
    {"Dresser", false, 62, 46},      {"Vanity", false, 64, 66},      {"Shoe Wall", false, 60, 84},
    {"Fridge", false, 48, 96},       {"Oven", false, 52, 52},        {"Sink", false, 56, 50},
    {"Counter", false, 70, 44},      {"Crib", false, 70, 54},        {"Toy Chest", false, 44, 32},
    {"Art Print", true, 50, 38},     {"Curtains", true, 80, 72},     {"Wall Clock", true, 28, 28},
    {"Mirror", true, 34, 52},        {"Beanbag", false, 46, 32},     {"Desk", false, 70, 52},
    {"Speaker", false, 26, 50},      {"Lounger", false, 84, 30},     {"BBQ Grill", false, 42, 58},
    {"Parasol", false, 60, 92},      {"Arcade", false, 40, 80},      {"Disco Ball", true, 26, 40},
    {"Perfume Shelf", true, 62, 34}, {"Rose Vase", false, 24, 40},   {"Clawfoot Tub", false, 82, 42},
    {"Toilet", false, 30, 44},       {"Glass Shower", false, 44, 98},  {"Park Bench", false, 80, 36},
    {"Brass Bed", false, 112, 60},   {"Plushie Bed", false, 112, 56}, {"Oval Vanity", false, 70, 80},
    {"Fluffy Rug", false, 130, 22},  {"Heart Rug", false, 64, 30},    {"Flower Cushion", false, 60, 22},
    {"Egg Chair", false, 54, 100},   {"Gamer Chair", false, 40, 72},  {"Clothes Rack", false, 74, 82},
    {"Record Player", false, 40, 40},{"Retro PC Desk", false, 84, 62},{"Tea Table", false, 70, 30},
    {"Wardrobe", false, 62, 104},    {"Floor Mirror", false, 36, 92}, {"Nightstand", false, 36, 40},
    {"Star Lights", true, 130, 28},  {"Hanging Vines", true, 130, 36},{"Poster Wall", true, 80, 56},
    {"Bath Vanity", false, 66, 70},  {"Pink Soak Tub", false, 92, 46},{"Retro Fridge", false, 46, 96},
    {"Plush Sofa", false, 104, 46},  {"Book Cubby", false, 52, 92},   {"Monstera", false, 44, 70},
    {"Drum Lamp", false, 28, 78},
    {"Gaming PC Setup", false, 80, 70},
    {"Canopy Bed", false, 110, 110}, {"Storage Daybed", false, 110, 56}, {"Heart Shelf", true, 44, 44}, {"Plushie Shelf", true, 80, 34},
    {"Mannequin", false, 34, 96}, {"Guitar", false, 30, 72}, {"Skateboard", false, 18, 62}, {"Boombox", false, 44, 28},
    {"Corkboard", true, 60, 44}, {"Vinyl Wall", true, 70, 34}, {"Tufted Pouf", false, 40, 30}, {"Stiletto Chair", false, 70, 80},
    {"Hollywood Vanity", false, 70, 96}, {"Round Rug", false, 80, 18}, {"Paper Lantern", true, 30, 40}, {"Hexagon Shelves", true, 60, 50},
    {"Floor Lamp", false, 28, 100}, {"Plaid Loveseat", false, 90, 50}, {"World Map Mural", true, 90, 56}, {"Lips Pillow", false, 40, 22},
    {"Clothes Pile", false, 50, 20}, {"Book Stack", false, 22, 26}, {"Purse Hooks", true, 60, 40}, {"White Iron Bed", false, 110, 64},
    {"Music Stand", false, 34, 80}, {"Window Seat", false, 90, 40}, {"Plant Shelf", true, 70, 40}, {"Desk Lamp", false, 26, 32},
    {"Polaroid Wall", true, 64, 44}, {"Candles", false, 24, 20}, {"Headphones", false, 26, 12}, {"Magazines", false, 40, 12},
    {"Patchwork Bed", false, 110, 60}, {"Ballet Flats", false, 26, 10},
    {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40}, {"", false, 40, 40},
    {"Cloud Princess Bed", false, 116, 66}, {"Emo Leopard Bed", false, 112, 58}, {"Pastel Gamer Bed", false, 112, 50}, {"Y2K Stripe Bed", false, 112, 60}, {"Daisy Cottage Bed", false, 112, 60}, {"Zebra Glam Bed", false, 112, 62}, {"Ribbon Bow Bed", false, 116, 72}, {"Fairy Light Curtain", true, 96, 60}, {"Neon Heart Sign", true, 44, 38}, {"Figure Shelf", true, 90, 26}, {"RGB Gaming Desk", false, 96, 74}, {"Retro CRT TV", false, 62, 60}, {"Lava Lamp", false, 14, 36}, {"Moon Saucer Chair", false, 52, 44}, {"Checker Rug", false, 130, 16}, {"Sunflower Rug", false, 120, 16}, {"Leopard Rug", false, 110, 16}, {"Heart Wall Mirror", true, 40, 40}, {"Kitchen Island", false, 140, 54}, {"Steel French Fridge", false, 62, 100}, {"Floating Shelves", true, 80, 40}, {"Fireplace TV Wall", false, 110, 120}, {"Cloud Sectional", false, 150, 48}, {"Pink Record Console", false, 60, 56}, {"Bead Door Curtain", true, 46, 96}
};

std::string itemName(int id) {
    return std::string(COLOR_NAMES[itemColor(id)]) + " " + SHAPES[itemShape(id)].name;
}

// A little high-heel silhouette, used on the shoe wall
static void miniHeel(const Pen& p, float x, float y, u32 c) {
    p.Q(x, y, x + 8, y + 2, x + 9, y + 5, x + 1, y + 5, c);
    p.T(x, y - 3, x + 3, y, x, y + 5, c);
    p.L(x + 1, y + 5, x, y + 9, 1.2f, c);
}

void drawItem(int id, float x, float y, float s, float t, bool music) {
    const int sh = itemShape(id), ci = itemColor(id);
    const RGB c = PALETTE[ci];
    const RGB d = shade(c, ci == 10 ? 0.55f : 0.72f), l = shade(c, 1.25f);
    const ShapeInfo& si = SHAPES[sh];
    Pen p(x, y, si.w, si.h, s);
    const u32 C0 = C(c), CD = C(d), CL = C(l);
    const u32 GOLD = C(col::GOLD), GOLDD = C(shade(col::GOLD, 0.7f)), WH = C(col::CREAM), DK = C(col::BLACK);
    const u32 GLOSS = C(255, 255, 255, 80);
    const u32 TRIM = (ci == 7) ? C(col::CREAM) : GOLD;   // gold furniture gets cream trim

    if (sh >= SH_CC0 && sh <= SH_CC_LAST) { ccDrawFurniture(sh - SH_CC0, x, y, s, ci, t); return; }
    switch (sh) {
    case SH_BED:   // canopy princess bed
        p.R(2, 0, 4, 46, TRIM); p.R(104, 0, 4, 46, TRIM);
        p.Q(0, 0, 110, 0, 104, 14, 6, 14, C(255, 220, 235, 150));      // sheer canopy
        p.T(6, 0, 20, 0, 8, 40, C(255, 220, 235, 110)); p.T(104, 0, 90, 0, 102, 40, C(255, 220, 235, 110));
        p.R(0, 0, 110, 3, TRIM);
        p.F(6, 24, 14, 26, ci, CD, 1);                                    // headboard
        p.E(13, 24, 14, 10, CD);
        p.R(8, 40, 96, 16, WH);
        p.F(34, 36, 70, 18, ci, C0, 2);
        p.R(34, 52, 70, 3, CD);
        p.E(24, 40, 22, 9, C(col::BLUSH)); p.E(24, 39, 18, 6, WH);
        p.H(70, 44, 3, C(255, 255, 255, 120));
        p.R(6, 56, 98, 4, TRIM); p.R(8, 60, 4, 4, TRIM); p.R(98, 60, 4, 4, TRIM);
        break;
    case SH_SOFA:
        p.F(6, 0, 88, 22, ci, CD, 3); p.E(6, 4, 12, 10, CD); p.E(94, 4, 12, 10, CD);
        p.F(0, 18, 100, 16, ci, C0, 4);
        p.E(7, 18, 14, 22, CD); p.E(93, 18, 14, 22, CD);
        p.E(30, 20, 26, 10, CL); p.E(70, 20, 26, 10, CL);
        p.H(50, 10, 4, C(col::HOT));
        p.R(0, 32, 100, 3, CD);
        p.L(10, 35, 7, 44, 2.5f, TRIM); p.L(90, 35, 93, 44, 2.5f, TRIM);
        break;
    case SH_CHAIR:   // tufted ghost/accent chair
        p.E(19, 14, 34, 28, CD); p.E(19, 14, 28, 22, C0);
        p.Ci(12, 12, 1.2f, CD); p.Ci(26, 12, 1.2f, CD); p.Ci(19, 18, 1.2f, CD);
        p.F(4, 26, 30, 9, ci, C0, 5);
        p.L(7, 35, 4, 52, 2.2f, TRIM); p.L(31, 35, 34, 52, 2.2f, TRIM);
        break;
    case SH_TABLE:
        p.E(35, 5, 70, 10, CD); p.E(35, 4, 68, 8, C0); p.E(30, 3, 30, 3, GLOSS);
        p.R(30, 8, 10, 16, TRIM);
        p.E(35, 27, 36, 6, TRIM);
        p.Ci(52, 1, 3, C(col::HOT)); p.R(51, -6, 2, 6, C(col::GREEN));   // little rose
        break;
    case SH_LAMP: {
        float on = music ? 0.f : 0.f;
        (void)on;
        p.Q(4, 0, 20, 0, 24, 22, 0, 22, C0);
        if (ci == 10 || ci == 11) p.F(6, 3, 12, 16, ci, C0, 6);
        p.R(0, 20, 24, 3, TRIM);
        p.Ci(6, 26, 1.3f, TRIM); p.Ci(12, 27, 1.3f, TRIM); p.Ci(18, 26, 1.3f, TRIM);  // fringe beads
        p.R(11, 23, 2, 44, TRIM);
        p.E(12, 70, 20, 7, CD);
        break;
    }
    case SH_PLANT: {
        u32 lg = C(col::GREEN), dg = C(shade(col::GREEN, 0.7f));
        float sway = std::sin(t * 1.5f) * 1.5f;
        for (int i = 0; i < 6; i++) {
            float a = -1.2f + i * 0.48f;
            float tx = 17 + std::sin(a) * 18 + sway, ty = 18 - std::cos(a) * 14;
            p.L(17, 36, tx, ty, 2.5f, i % 2 ? lg : dg);
            p.E(tx, ty, 10, 5, i % 2 ? lg : dg);
        }
        p.Q(6, 36, 28, 36, 25, 56, 9, 56, C0);
        p.R(5, 34, 24, 4, TRIM);
        break;
    }
    case SH_RUG:
        p.E(60, 8, 120, 16, CD);
        p.E(60, 8, 114, 13, C0);
        for (int i = 0; i < 14; i++) p.Ci(10 + i * 7.5f, 8 + ((i * 5) % 3) - 1, 1.6f, CL);  // shag tufts
        if (ci == 10 || ci == 11) p.F(30, 5, 60, 6, ci, C0, 7);
        break;
    case SH_TV: {
        p.F(10, 38, 52, 20, ci, C0, 8); p.R(10, 46, 52, 1.5f, CD); p.R(34, 38, 1.5f, 20, CD);
        p.R(4, 0, 64, 38, DK);
        u32 sc = C((uint8_t)(170 + 60 * std::sin(t * 1.3f)), 120, (uint8_t)(190 + 50 * std::sin(t * 0.7f)));
        p.R(7, 3, 58, 30, sc);
        p.E(36, 26, 30, 8, C(255, 255, 255, 60));   // on-screen "show"
        p.H(36, 14, 5, C(col::HOT, 200));
        p.T(48, 3, 65, 3, 65, 14, GLOSS);
        break;
    }
    case SH_BOOKSHELF:
        p.F(0, 0, 50, 90, ci, C0, 9); p.R(4, 4, 42, 82, CD);
        for (int r = 0; r < 3; r++) {
            float sy = 4 + r * 28;
            p.R(4, sy + 25, 42, 3, C0);
            for (int b = 0; b < 6; b++) {
                static const RGB bc[] = {col::HOT, col::BLUSH, col::CREAM, col::LILAC, col::GOLD, col::BABYBLUE};
                float bh = 16 + ((b + r) * 5 % 7);
                p.R(6 + b * 6.5f, sy + 25 - bh, 5, bh, C(bc[(b + r) % 6]));
            }
        }
        break;
    case SH_DRESSER:
        p.F(0, 0, 62, 42, ci, C0, 10);
        for (int i = 0; i < 3; i++) {
            p.R(4, 4 + i * 12.5f, 54, 10, CL);
            if (ci == 10 || ci == 11) p.F(5, 5 + i * 12.5f, 52, 8, ci, CL, 11 + i);
            p.Ci(31, 9 + i * 12.5f, 1.8f, TRIM);
        }
        p.L(5, 42, 3, 46, 2.f, TRIM); p.L(57, 42, 59, 46, 2.f, TRIM);
        // jewelry box on top
        p.R(40, -6, 12, 6, C(col::HOT)); p.R(40, -7, 12, 2, GOLD);
        break;
    case SH_VANITY: {   // Hollywood vanity with bulb mirror and perfume
        p.E(32, 14, 40, 30, TRIM); p.E(32, 14, 34, 25, C(200, 225, 240));
        p.T(40, 4, 46, 6, 34, 26, C(255, 255, 255, 90));
        for (int i = 0; i < 7; i++) {
            float a = 3.14159f + i * (3.14159f / 6);
            float tw = 0.6f + 0.4f * std::sin(t * 3 + i);
            p.Ci(32 + std::cos(a) * 21, 15 + std::sin(a) * 16, 2.2f, C(255, 240, 200, (uint8_t)(160 + 95 * tw)));
        }
        p.F(0, 30, 64, 8, ci, C0, 14);
        p.F(4, 38, 16, 24, ci, CD, 15); p.F(44, 38, 16, 24, ci, CD, 16);
        p.Ci(12, 48, 1.4f, TRIM); p.Ci(52, 48, 1.4f, TRIM);
        p.L(22, 38, 22, 66, 1.5f, TRIM); p.L(42, 38, 42, 66, 1.5f, TRIM);
        // perfume bottles + lipstick
        p.R(8, 22, 6, 8, C(255, 200, 120, 200)); p.R(10, 19, 2, 3, GOLD);
        p.Ci(18, 26, 3.5f, C(255, 160, 200, 200)); p.R(17, 20, 2, 3, GOLD);
        p.R(52, 23, 3, 7, GOLD); p.R(52, 20, 3, 3, C(col::RED));
        break;
    }
    case SH_SHOERACK: {   // the diva shoe wall
        p.F(0, 0, 60, 84, ci, C0, 17); p.R(4, 4, 52, 76, CD);
        static const RGB shoes[] = {col::HOT, col::BLACK, col::GOLD, col::RED, col::BLUSH, col::CREAM, col::LILAC, col::BERRY};
        for (int r = 0; r < 4; r++) {
            p.R(4, 20 + r * 19, 52, 2, TRIM);
            for (int k = 0; k < 2; k++) {
                u32 sc = C(shoes[(r * 2 + k + ci) % 8]);
                if ((r + k) % 3 == 0 && ci != 10) {   // a zebra pair now and then
                    sc = C(col::CREAM);
                }
                miniHeel(p, 8 + k * 24, 10 + r * 19, sc);
                miniHeel(p, 18 + k * 24, 10 + r * 19, sc);
            }
        }
        p.Sp(50, 6, 4, C(255, 255, 255, (uint8_t)(140 + 100 * std::sin(t * 4))));
        break;
    }
    case SH_FRIDGE:
        p.F(0, 0, 48, 96, ci, C0, 18); p.R(0, 34, 48, 2, CD);
        p.R(40, 12, 3, 16, TRIM); p.R(40, 42, 3, 26, TRIM);
        p.T(4, 2, 16, 2, 4, 60, GLOSS);
        p.H(14, 48, 3.5f, C(col::HOT)); p.Ci(24, 52, 2.2f, C(col::GOLD));   // magnets
        p.R(4, 94, 40, 2, CD);
        break;
    case SH_OVEN:
        p.F(0, 0, 52, 52, ci, C0, 19); p.R(0, 0, 52, 10, CD);
        for (int i = 0; i < 4; i++) p.Ci(10 + i * 11, 5, 2.4f, TRIM);
        p.R(7, 16, 38, 26, DK); p.R(9, 18, 34, 22, C(80, 40, 30));
        p.R(10, 13, 32, 2, TRIM);
        break;
    case SH_SINK:
        p.F(0, 14, 56, 36, ci, C0, 20); p.R(0, 10, 56, 6, C(col::LIGHTGRAY));
        p.R(26, 0, 4, 11, TRIM); p.R(26, 0, 13, 3, TRIM);
        p.L(28, 16, 28, 48, 1.f, CD); p.Ci(24, 30, 1.4f, TRIM); p.Ci(32, 30, 1.4f, TRIM);
        break;
    case SH_COUNTER:
        p.F(0, 6, 70, 38, ci, C0, 21); p.R(0, 0, 70, 7, C(236, 228, 226)); p.R(0, 5, 70, 2, C(col::GRAY));
        p.L(35, 8, 35, 42, 1.f, CD); p.Ci(31, 22, 1.4f, TRIM); p.Ci(39, 22, 1.4f, TRIM);
        p.Ci(12, -3, 3.5f, C(col::RED)); p.Ci(18, -3, 3.5f, C(col::GREEN)); // fruit bowl
        p.E(15, 0, 16, 5, C(col::CREAM));
        break;
    case SH_CRIB:
        p.R(0, 0, 6, 54, C0); p.R(64, 0, 6, 54, C0);
        p.Ci(3, 0, 3.5f, TRIM); p.Ci(67, 0, 3.5f, TRIM);
        p.R(0, 8, 70, 4, C0); p.R(0, 38, 70, 4, C0);
        p.R(6, 30, 58, 8, WH); p.E(18, 30, 16, 6, C(col::BLUSH));
        for (int i = 0; i < 8; i++) p.R(9 + i * 7, 12, 2.5f, 26, C0);
        // mobile
        p.L(35, -14, 35, -4, 1.f, TRIM);
        for (int i = 0; i < 3; i++) {
            float a = t + i * 2.1f;
            float mx = 35 + std::cos(a) * 10;
            p.L(35, -14, mx, -8, 0.8f, TRIM);
            if (i == 0) p.Sp(mx, -6, 3, GOLD); else if (i == 1) heart(p.X(mx), p.Y(-6), 2.5f * s, C(col::HOT));
            else p.Ci(mx, -6, 2.4f, C(col::BABYBLUE));
        }
        break;
    case SH_TOYBOX:
        p.F(0, 8, 44, 24, ci, C0, 22); p.F(-2, 3, 48, 7, ci, CD, 23);
        p.H(22, 20, 4.5f, C(col::HOT)); p.R(0, 30, 44, 2, CD);
        p.Ci(36, 2, 4, C(col::BABYBLUE));   // ball peeking out
        break;
    case SH_PAINTING:
        p.F(0, 0, 50, 38, ci, TRIM, 24); p.R(3, 3, 44, 32, C(col::CREAM));
        // fashion sketch: a tiny dress form
        p.Ci(25, 9, 3.5f, C(col::MOCHA));
        p.Q(21, 13, 29, 13, 34, 32, 16, 32, C0);
        p.L(25, 32, 25, 34, 1.f, DK);
        swirl(9, 28, 5, 1.4f, 0.8f, C(col::MOCHA, 150));
        p.Sp(41, 9, 3, C(col::GOLD));
        break;
    case SH_CURTAINS:
        p.R(0, 0, 80, 4, TRIM); p.Ci(0, 2, 3, TRIM); p.Ci(80, 2, 3, TRIM);
        p.F(2, 4, 22, 68, ci, C0, 25); p.F(56, 4, 22, 68, ci, C0, 26);
        p.Q(2, 4, 24, 4, 32, 72, 2, 72, C(c, 170));
        p.Q(56, 4, 78, 4, 78, 72, 48, 72, C(c, 170));
        p.L(8, 4, 8, 70, 1.f, CD); p.L(17, 4, 20, 70, 1.f, CD); p.L(70, 4, 70, 70, 1.f, CD); p.L(62, 4, 60, 70, 1.f, CD);
        p.R(4, 40, 22, 3, TRIM); p.R(54, 40, 22, 3, TRIM);   // gold tie-backs
        break;
    case SH_CLOCK: {
        p.Ci(14, 14, 14, TRIM); p.Ci(14, 14, 12, C0); p.Ci(14, 14, 10, WH);
        for (int i = 0; i < 12; i++) {
            float a = i * 0.5236f;
            p.Ci(14 + std::cos(a) * 8, 14 + std::sin(a) * 8, 0.7f, CD);
        }
        float sec = std::fmod(t, 60.f);
        float ah = sec / 60.f * 6.2831f - 1.5708f, am = t * 0.5f - 1.5708f;
        p.L(14, 14, 14 + std::cos(am) * 7, 14 + std::sin(am) * 7, 1.2f, DK);
        p.L(14, 14, 14 + std::cos(ah) * 5, 14 + std::sin(ah) * 5, 1.6f, C(col::HOT));
        break;
    }
    case SH_MIRROR:
        p.E(17, 26, 34, 52, TRIM); p.E(17, 26, 30, 47, C0); p.E(17, 26, 25, 41, C(205, 228, 244));
        p.L(10, 18, 18, 8, 2.f, C(255, 255, 255, 170)); p.L(13, 26, 22, 14, 1.2f, C(255, 255, 255, 140));
        p.H(17, 2, 3.5f, C(col::HOT));
        break;
    case SH_BEANBAG:
        p.E(23, 20, 46, 24, CD); p.E(23, 18, 44, 22, C0); p.E(22, 10, 30, 12, CL);
        if (ci == 10 || ci == 11) p.F(10, 14, 26, 8, ci, C0, 27);
        break;
    case SH_DESK:
        p.F(0, 8, 70, 6, ci, C0, 28);
        p.F(0, 14, 24, 38, ci, CD, 29); p.R(3, 20, 18, 10, CL); p.R(3, 34, 18, 10, CL);
        p.L(66, 14, 68, 52, 2.5f, TRIM);
        // pink flip phone + laptop
        p.Q(36, 8, 56, 8, 60, -6, 40, -6, C(col::LIGHTGRAY)); p.Q(39, 6, 55, 6, 58, -4, 42, -4, C(col::BLUSH));
        p.H(48, 0, 2, C(col::HOT));
        p.R(20, 4, 5, 4, C(col::HOT)); p.R(20, 2, 5, 2, C(col::GOLD));
        break;
    case SH_SPEAKER: {
        float pump = music ? 1.f + 0.15f * std::fabs(std::sin(t * 8)) : 1.f;
        p.F(0, 0, 26, 50, ci, C0, 30);
        p.Ci(13, 34, 9 * pump, DK); p.Ci(13, 34, 4, C(col::GRAY));
        p.Ci(13, 13, 5 * pump, DK); p.Ci(13, 13, 2, C(col::GRAY));
        if (music) { p.Sp(30, 6 + std::sin(t * 4) * 3, 3, C(col::HOT)); }
        break;
    }
    case SH_LOUNGER:
        p.Q(0, 4, 8, 0, 22, 18, 14, 22, CD);
        p.F(14, 14, 70, 7, ci, C0, 31);
        for (int i = 0; i < 6; i++) p.R(20 + i * 11, 14, 4, 7, C(255, 255, 255, 90));
        p.L(18, 21, 16, 30, 2.f, TRIM); p.L(78, 21, 80, 30, 2.f, TRIM);
        p.E(72, 12, 16, 6, C(col::BLUSH));   // towel
        break;
    case SH_GRILL: {
        p.E(21, 16, 42, 22, C0); p.R(0, 10, 42, 8, CD); p.E(21, 10, 40, 10, CD);
        p.L(10, 24, 4, 58, 2.f, DK); p.L(32, 24, 38, 58, 2.f, DK); p.L(21, 26, 21, 58, 2.f, DK);
        p.Ci(4, 56, 2.5f, DK); p.Ci(38, 56, 2.5f, DK);
        for (int i = 0; i < 3; i++) {
            float k = std::fmod(t * 0.6f + i * 0.33f, 1.f);
            p.Ci(14 + i * 7 + std::sin(t * 2 + i) * 3, 6 - k * 22, 2 + k * 3, C(255, 255, 255, (uint8_t)(120 * (1 - k))));
        }
        break;
    }
    case SH_UMBRELLA:
        p.R(29, 26, 2, 66, C(col::LIGHTGRAY));
        p.T(0, 30, 30, 0, 60, 30, C0);
        for (int i = 0; i < 3; i++) p.T(i * 20.f, 30, 30, 0, i * 20.f + 10, 30, ci == 4 ? C(col::HOT) : WH);
        for (int i = 0; i < 6; i++) p.Ci(5 + i * 10, 30, 2, TRIM);   // fringe
        break;
    case SH_ARCADE: {
        p.F(0, 0, 40, 80, ci, C0, 32);
        p.R(4, 0, 32, 8, DK);
        p.R(6, 10, 28, 22, DK);
        for (int i = 0; i < 4; i++)
            p.R(9 + i * 6, 14 + std::fabs(std::sin(t * 3 + i)) * 12, 4, 4, C(i % 2 ? col::HOT : col::GOLD));
        p.Q(2, 40, 38, 40, 36, 48, 4, 48, CD);
        p.Ci(12, 43, 2, C(col::HOT)); p.Ci(24, 43, 2, C(col::GOLD)); p.Ci(30, 44, 2, C(col::BABYBLUE));
        p.H(20, 62, 5, C(col::HOT, 200));
        break;
    }
    case SH_DISCOBALL: {
        p.R(12, 0, 2, 14, C(col::GRAY));
        p.Ci(13, 27, 13, C(200, 200, 210));
        for (int r = 0; r < 4; r++)
            for (int k = 0; k < 4; k++) {
                float tw = std::sin(t * 6 + r * 1.3f + k * 2.1f);
                RGB tc = tw > 0.5f ? c : col::LIGHTGRAY;
                p.R(5 + k * 4, 18 + r * 4, 3.5f, 3.5f, C(tc, 220));
            }
        p.Sp(4 + std::sin(t * 2) * 2, 18, 4, C(255, 255, 255, 220));
        break;
    }
    case SH_PERFUMESHELF:
        p.F(0, 24, 62, 5, ci, C0, 33);
        p.T(6, 29, 14, 29, 6, 34, TRIM); p.T(56, 29, 48, 29, 56, 34, TRIM);
        p.R(6, 12, 9, 12, C(255, 205, 130, 210)); p.R(9, 8, 3, 4, GOLD);
        p.Ci(24, 18, 6, C(255, 150, 200, 210)); p.R(22, 8, 4, 5, GOLD); p.H(24, 18, 2.5f, C(255, 255, 255, 120));
        p.Q(36, 10, 46, 10, 48, 24, 34, 24, C(200, 170, 240, 210)); p.Ci(41, 7, 3, C(col::HOT));
        p.R(52, 16, 5, 8, C(col::CREAM));
        p.Sp(30, 8, 3, C(255, 255, 255, (uint8_t)(150 + 100 * std::sin(t * 3))));
        break;
    case SH_FLOWERS: {
        static const RGB fl[] = {col::HOT, col::BLUSH, col::RED, col::CREAM, col::PINK};
        for (int i = 0; i < 5; i++) {
            float fx = 4 + i * 4, fy = 6 + (i % 2) * 5;
            p.L(12, 24, fx, fy, 1.2f, C(col::GREEN));
            p.Ci(fx, fy, 3.6f, C(fl[(i + ci) % 5])); p.Ci(fx, fy, 1.4f, C(shade(fl[(i + ci) % 5], 0.7f)));
        }
        p.Q(6, 22, 18, 22, 20, 40, 4, 40, C(c, 220)); p.R(5, 22, 14, 2, TRIM);
        break;
    }
    case SH_BATHTUB:
        p.F(2, 12, 78, 24, ci, C0, 34); p.E(41, 13, 82, 7, CL); p.E(41, 13, 74, 4, C(170, 210, 240));
        p.L(8, 34, 4, 42, 3.f, TRIM); p.L(74, 34, 78, 42, 3.f, TRIM);
        p.R(70, 0, 3, 12, TRIM); p.R(64, 0, 9, 3, TRIM);
        for (int i = 0; i < 6; i++) p.Ci(20 + i * 8 + std::sin(t * 2 + i) * 2, 10 - (i % 3) * 3, 3 + (i % 2), C(255, 255, 255, 220));
        break;
    case SH_TOILET:
        p.R(4, 0, 22, 16, C0); p.R(4, 0, 22, 3, CL); p.R(10, 3, 10, 2, TRIM);   // tank & flush
        p.E(15, 24, 28, 12, CD); p.E(15, 23, 26, 10, C(255, 255, 255)); p.E(15, 23, 18, 6, C(200, 225, 240));
        p.Q(8, 26, 22, 26, 20, 44, 10, 44, C0);
        break;
    case SH_SHOWER:
        p.R(0, 0, 44, 4, TRIM); p.R(0, 0, 3, 98, TRIM); p.R(41, 0, 3, 98, TRIM);
        p.R(3, 4, 38, 90, C(c, 70));
        p.F(0, 92, 44, 6, ci, C0, 35);
        p.R(18, 4, 3, 10, C(col::GRAY)); p.E(19.5f, 15, 12, 4, C(col::GRAY));
        break;
    // ---------------- cute room pack
    case SH_BRASSBED: {
        const u32 BR = C(232, 196, 120), BRD = C(190, 150, 80);
        p.R(4, 34, 104, 14, WH); p.R(4, 46, 104, 3, C(230, 222, 220));
        p.F(22, 28, 86, 22, ci, C0, 41);
        for (int i = 0; i < 14; i++) p.Ci(26 + (i * 23 % 80), 32 + (i * 7 % 14), 1.4f, i % 2 ? C(255, 150, 180) : C(255, 255, 255, 200));
        p.E(18, 30, 22, 11, WH); p.E(34, 29, 20, 10, C(250, 220, 232)); p.H(48, 26, 5, C(255, 110, 150));
        p.E(88, 27, 9, 9, WH); p.E(85, 19, 3, 9, WH); p.E(91, 19, 3, 9, WH); p.Ci(86, 27, 0.8f, DK); p.Ci(90, 27, 0.8f, DK);   // a little bunny plush
        // brass headboard (left) and footboard (right): posts, finials and rails
        for (int k = 0; k < 2; k++) { float x = k ? 14 : 3; p.L(x, 4, x, 58, 2.5f, BR); p.Ci(x, 3, 2.6f, BRD); }
        for (int i = 0; i < 3; i++) p.L(3, 8 + i * 9, 14, 8 + i * 9, 1.8f, BR);
        for (int k = 0; k < 2; k++) { float x = k ? 110 : 100; p.L(x, 22, x, 58, 2.5f, BR); p.Ci(x, 21, 2.4f, BRD); }
        for (int i = 0; i < 2; i++) p.L(100, 26 + i * 8, 110, 26 + i * 8, 1.8f, BR);
        break;
    }
    case SH_PLUSHBED: {
        const u32 SPREAD = C(118, 78, 66);
        for (int i = 0; i < 5; i++) p.Ci(10 + i * 18.f, 12, 9, C0);   // scalloped headboard
        p.R(2, 12, 92, 12, C0);
        p.R(0, 24, 112, 14, SPREAD); for (int r = 0; r < 2; r++) for (int i = 0; i < 12; i++) p.Ci(5 + i * 9.f + r * 4, 28 + r * 6, 1.2f, C(255, 255, 255, 200));
        p.R(0, 38, 112, 18, CL); p.R(4, 41, 48, 12, C0); p.R(60, 41, 48, 12, C0); p.R(24, 46, 8, 2, CD); p.R(80, 46, 8, 2, CD);
        // original plushies along the back
        p.E(18, 18, 14, 13, C(210, 170, 120)); p.Ci(12, 12, 3, C(210, 170, 120)); p.Ci(24, 12, 3, C(210, 170, 120)); p.Ci(16, 17, 0.9f, DK); p.Ci(20, 17, 0.9f, DK);   // bear
        p.E(38, 19, 13, 12, WH); p.E(35, 9, 3, 10, WH); p.E(41, 9, 3, 10, WH); p.Ci(36, 18, 0.9f, DK); p.Ci(40, 18, 0.9f, DK); p.Ci(38, 21, 1, C(255, 150, 180));   // bunny
        p.E(57, 19, 14, 12, C(190, 190, 200)); p.T(50, 15, 53, 9, 55, 14, C(190, 190, 200)); p.T(59, 14, 61, 9, 64, 15, C(190, 190, 200)); p.Ci(55, 18, 0.9f, DK); p.Ci(59, 18, 0.9f, DK);   // cat
        p.E(75, 20, 12, 11, C(255, 226, 120)); p.T(80, 20, 84, 21, 80, 22, C(255, 160, 80)); p.Ci(74, 18, 0.9f, DK);   // chick
        p.E(92, 19, 13, 12, C(250, 190, 210)); p.Ci(87, 13, 3, C(250, 190, 210)); p.Ci(97, 13, 3, C(250, 190, 210)); p.Ci(90, 18, 0.9f, DK); p.Ci(94, 18, 0.9f, DK);   // pink bear
        break;
    }
    case SH_OVALVANITY: {
        p.E(35, 22, 42, 42, TRIM); p.E(35, 22, 36, 36, C0); p.E(35, 22, 30, 30, C(205, 228, 244));
        p.L(26, 14, 34, 8, 2.f, C(255, 255, 255, 170)); p.L(28, 22, 38, 12, 1.2f, C(255, 255, 255, 130));
        p.H(35, 1, 3, C(col::HOT)); p.L(35, 43, 35, 46, 3, TRIM);
        p.R(0, 46, 70, 6, CL); p.R(2, 52, 24, 14, C0); p.R(44, 52, 24, 14, C0); p.Ci(14, 59, 1.4f, GOLD); p.Ci(56, 59, 1.4f, GOLD);
        p.L(5, 66, 3, 80, 2.5f, C0); p.L(65, 66, 67, 80, 2.5f, C0); p.L(24, 66, 24, 78, 2, C0); p.L(46, 66, 46, 78, 2, C0);
        p.R(6, 38, 4, 8, C(255, 170, 200)); p.R(12, 40, 3, 6, C(200, 170, 240)); p.R(17, 41, 5, 5, C(255, 236, 180));
        p.R(56, 36, 6, 10, C(220, 240, 250)); for (int i = 0; i < 4; i++) p.Ci(56 + (i % 2) * 6, 31 + (i / 2) * 4, 2.2f, i % 2 ? C(255, 180, 200) : C(255, 255, 255));
        break;
    }
    case SH_FLUFFYRUG: {
        for (int i = 0; i < 48; i++) { float a = i * 0.1309f; p.L(65 + std::cos(a) * 62, 11 + std::sin(a) * 10, 65 + std::cos(a) * 67, 11 + std::sin(a) * 12, 2, CD); }
        p.E(65, 11, 128, 20, C0); p.E(65, 10, 104, 14, CL);
        for (int i = 0; i < 30; i++) { float x = 14 + (i * 37 % 102), y = 5 + (i * 13 % 12); p.L(x, y, x + 2, y - 2, 1, C(255, 255, 255, 90)); }
        break;
    }
    case SH_HEARTRUG:
        for (int i = 0; i < 24; i++) { float a = i * 0.2618f; p.L(32 + std::cos(a) * 14, 14 + std::sin(a) * 10, 32 + std::cos(a) * 17, 14 + std::sin(a) * 13, 1.5f, CD); }
        p.H(32, 12, 15, C0); p.H(30, 10, 8, CL);
        for (int i = 0; i < 10; i++) p.L(20 + i * 2.5f, 8 + (i % 3) * 4, 21 + i * 2.5f, 6 + (i % 3) * 4, 1, C(255, 255, 255, 110));
        break;
    case SH_FLOWERCUSHION:
        for (int i = 0; i < 5; i++) { float a = i * 1.2566f - 1.5708f; p.E(30 + std::cos(a) * 14, 10 + std::sin(a) * 5, 24, 13, i % 2 ? C0 : CL); }
        p.E(30, 10, 18, 9, CD); p.E(30, 9, 14, 6, CL);
        break;
    case SH_EGGCHAIR: {
        const u32 RAT = C(214, 182, 132), RATD = C(170, 138, 96), POLE = C(200, 200, 210);
        p.E(30, 97, 34, 6, POLE); p.L(46, 96, 50, 14, 3, POLE); p.L(50, 14, 34, 4, 3, POLE); p.L(34, 4, 28, 24, 1.5f, POLE);
        p.E(27, 58, 48, 66, RAT);
        for (int i = 0; i < 10; i++) { p.L(6 + i * 4.5f, 30, 4 + i * 4.5f, 88, 1, RATD); }
        for (int r = 0; r < 6; r++) p.L(6, 34 + r * 9, 48, 34 + r * 9, 1, RATD);
        p.E(27, 60, 36, 50, C(150, 118, 80)); p.E(27, 74, 32, 16, C0); p.E(27, 56, 22, 18, CL);
        break;
    }
    case SH_GAMERCHAIR: {
        const u32 G2 = C(80, 80, 90);
        for (int i = 0; i < 5; i++) { float a = i * 1.2566f; p.L(20, 66, 20 + std::cos(a) * 16, 68 + std::sin(a) * 3, 2, G2); p.Ci(20 + std::cos(a) * 16, 70 + std::sin(a) * 2, 1.8f, G2); }
        p.R(18, 52, 4, 14, C(170, 170, 180));
        p.R(4, 44, 32, 10, WH); p.R(8, 6, 24, 40, WH); p.E(20, 6, 24, 8, WH);
        p.R(12, 10, 4, 34, C0); p.R(24, 10, 4, 34, C0); p.E(20, 34, 16, 8, C0); p.E(20, 12, 14, 6, C0);
        p.R(1, 36, 4, 12, G2); p.R(35, 36, 4, 12, G2);
        break;
    }
    case SH_CLOTHESRACK: {
        p.L(4, 6, 4, 80, 2, TRIM); p.L(70, 6, 70, 80, 2, TRIM); p.L(3, 6, 71, 6, 2.5f, TRIM); p.L(0, 80, 10, 80, 2, TRIM); p.L(64, 80, 74, 80, 2, TRIM);
        const u32 CLS[6] = {C0, C(250, 170, 196), C(246, 240, 226), C(110, 140, 190), C(40, 36, 44), C(240, 200, 214)};
        for (int i = 0; i < 6; i++) {
            float x = 10 + i * 10.f, len = i == 5 ? 46 : 26 + (i % 3) * 6;
            p.L(x + 3, 6, x + 3, 10, 1, C(200, 200, 210)); p.T(x - 1, 13, x + 3, 9, x + 7, 13, C(200, 200, 210));
            p.Q(x - 1, 13, x + 7, 13, x + 9, 13 + len, x - 3, 13 + len, CLS[i]);
            if (i == 5) for (int k = 0; k < 6; k++) p.Ci(x + (k % 3) * 2.5f, 20 + k * 6, 1, C(255, 120, 150));
        }
        p.R(16, 70, 5, 12, WH); p.R(24, 70, 5, 12, WH); p.R(16, 79, 8, 3, WH); p.R(24, 79, 8, 3, WH);   // boots
        break;
    }
    case SH_RECORDPLAYER: {
        p.R(2, 18, 36, 22, CD); p.R(5, 22, 13, 15, C(240, 200, 214)); p.R(21, 22, 13, 15, C(170, 200, 240));
        p.R(0, 10, 40, 9, C0); p.E(16, 10, 24, 6, DK);
        float a = music ? t * 6 : 0; p.L(16 + std::cos(a) * 8, 10 + std::sin(a) * 2, 16 + std::cos(a) * 10, 10 + std::sin(a) * 2.5f, 1, C(255, 255, 255, 120));
        p.E(16, 10, 6, 2, C(col::HOT)); p.L(34, 5, 24, 9, 1.2f, C(200, 200, 210)); p.Ci(34, 5, 1.6f, C(200, 200, 210));
        if (music) { p.Sp(36, -2 + std::sin(t * 4) * 2, 2.5f, C(col::HOT)); p.Sp(6, -4 + std::cos(t * 4) * 2, 2, C(col::HOT)); }
        break;
    }
    case SH_RETROPC: {
        p.R(0, 26, 84, 6, C0); p.R(60, 32, 22, 30, CD); p.R(63, 36, 16, 10, CL); p.R(63, 49, 16, 10, CL); p.L(4, 32, 4, 62, 2.5f, CD); p.L(56, 32, 56, 62, 2.5f, CD);
        p.R(8, 2, 32, 24, C(255, 140, 190)); p.R(12, 5, 24, 16, C(46, 34, 56)); p.H(24, 12, 3, C(255, 120, 170)); p.R(14, 7, 6, 1, C(120, 230, 200));
        p.R(18, 22, 12, 4, C(255, 170, 210));
        p.Q(10, 26, 38, 26, 40, 22, 12, 22, C(255, 186, 220));
        p.E(56, 22, 14, 8, C(150, 200, 240)); p.Ci(72, 21, 4, C(210, 214, 224)); p.Ci(72, 21, 3, C(230, 240, 250)); p.R(44, 22, 6, 4, C(250, 236, 120));
        break;
    }
    case SH_TEATABLE: {
        p.L(14, 12, 8, 30, 1.5f, DK); p.L(56, 12, 62, 30, 1.5f, DK); p.L(35, 14, 35, 30, 1.5f, DK);
        p.E(35, 11, 70, 14, CD); p.E(35, 9, 68, 12, C0); p.E(35, 8, 34, 7, WH);
        p.E(29, 2, 12, 10, C(240, 90, 110)); p.E(29, -3, 5, 3, C(240, 90, 110)); p.L(35, 2, 39, -1, 1.5f, C(240, 90, 110));
        p.E(43, 6, 5, 3, C(250, 250, 250)); p.E(48, 7, 5, 3, C(250, 250, 250));
        for (int i = 0; i < 3; i++) p.Ci(20 + i * 3.f, 2 - (i % 2) * 2, 2.2f, C(170, 30, 50));
        break;
    }
    case SH_WARDROBE: {
        p.R(2, 14, 58, 90, C0); p.R(0, 10, 62, 6, CL); p.L(31, 18, 31, 98, 1, CD);
        p.R(6, 20, 21, 74, CL); p.R(35, 20, 21, 74, CL); p.R(27, 52, 2, 8, GOLD); p.R(33, 52, 2, 8, GOLD);
        p.R(4, 98, 4, 6, CD); p.R(54, 98, 4, 6, CD);
        p.E(16, 5, 10, 10, C(250, 190, 210)); p.E(13, -2, 3, 7, C(250, 190, 210)); p.E(19, -2, 3, 7, C(250, 190, 210));   // bunny plush on top
        p.E(44, 5, 11, 10, C(240, 220, 120)); p.Ci(40, 1, 2.5f, C(240, 220, 120)); p.Ci(48, 1, 2.5f, C(240, 220, 120));     // bear plush on top
        break;
    }
    case SH_FLOORMIRROR:
        p.L(18, 76, 6, 92, 2, C0); p.L(18, 76, 30, 92, 2, C0);
        p.E(18, 40, 34, 76, C0); p.E(18, 40, 28, 70, C(205, 228, 244));
        p.L(10, 22, 20, 8, 2, C(255, 255, 255, 170)); p.L(12, 34, 24, 18, 1.2f, C(255, 255, 255, 130));
        p.H(18, 2, 3, C(col::HOT));
        break;
    case SH_NIGHTSTAND:
        p.R(2, 14, 32, 26, C0); p.R(0, 12, 36, 3, CL); p.R(5, 18, 26, 9, CL); p.R(5, 29, 26, 9, CL); p.Ci(18, 22, 1.2f, GOLD); p.Ci(18, 33, 1.2f, GOLD);
        p.R(8, 4, 4, 8, GOLD); p.Q(3, -4, 17, -4, 19, 4, 1, 4, C(255, 236, 210)); p.E(10, 6, 18, 6, C(255, 236, 170, 60));
        p.R(24, 6, 4, 6, C(250, 246, 236)); p.Ci(26, 4.5f, 1, C(255, 200, 80));
        break;
    case SH_STARLIGHTS:
        for (int i = 0; i <= 12; i++) {
            float u = i / 12.f, x = u * 130, y = 4 + std::sin(u * 3.14159f) * 16;
            if (i) { float pu = (i - 1) / 12.f; p.L(pu * 130, 4 + std::sin(pu * 3.14159f) * 16, x, y, 1, C(90, 80, 90)); }
            p.Sp(x, y + 4, 3.6f, ci == 0 || ci == 10 || ci == 11 ? C(255, 226, 120, (uint8_t)(170 + 80 * std::sin(t * 3 + i))) : C(c, (uint8_t)(170 + 80 * std::sin(t * 3 + i))));
        }
        break;
    case SH_VINES:
        for (int i = 0; i < 26; i++) {
            float u = i / 25.f, x = u * 130, y = 6 + std::sin(u * 3.14159f) * 12 + std::sin(u * 20) * 2;
            p.E(x, y, 8, 5, i % 2 ? C(110, 170, 90) : C(140, 196, 110));
            if (i % 3 == 0) { p.L(x, y, x + 1, y + 12 + (i % 4) * 3, 1, C(110, 160, 90)); p.E(x + 1, y + 13 + (i % 4) * 3, 5, 3, C(130, 186, 100)); }
            if (i % 4 == 1) p.Ci(x + 2, y + 3, 1.6f, C0);
        }
        break;
    case SH_POSTERWALL: {
        p.R(0, 4, 30, 40, C(250, 190, 210)); p.H(15, 20, 9, C(255, 90, 140)); p.R(4, 36, 22, 3, C(255, 255, 255, 180));
        p.R(34, 0, 24, 30, C(196, 172, 240)); p.Ci(46, 12, 6, C(255, 250, 220)); p.Ci(49, 10, 5, C(196, 172, 240)); p.Sp(40, 22, 2.5f, C(255, 255, 255)); p.Sp(52, 24, 2, C(255, 255, 255));
        p.R(62, 6, 18, 24, C(170, 210, 245)); p.Ci(68, 18, 3, C(220, 30, 60)); p.Ci(74, 19, 3, C(220, 30, 60)); p.L(70, 12, 68, 16, 1, C(80, 140, 70)); p.L(70, 12, 74, 16, 1, C(80, 140, 70));
        for (int k = 0; k < 2; k++) { float x = 36 + k * 22.f, y = 34; p.R(x, y, 18, 20, C(252, 252, 250)); p.R(x + 2, y + 2, 14, 12, k ? C(255, 200, 150) : C(150, 220, 200)); p.R(x + 5, y - 2, 8, 3, C0); }
        p.R(10, 1, 10, 3, C0); p.R(66, 3, 10, 3, CL);
        break;
    }
    case SH_BATHVANITY: {
        p.R(6, 0, 54, 26, TRIM); p.R(9, 3, 48, 20, C(205, 228, 244)); p.L(14, 6, 22, 18, 1.5f, C(255, 255, 255, 150));
        for (int k = 0; k < 3; k++) { p.Ci(14 + k * 19.f, -2, 3, C(255, 250, 230)); p.E(14 + k * 19.f, 2, 14, 6, C(255, 240, 190, 60)); }
        p.R(0, 30, 66, 6, C0); p.R(2, 36, 62, 34, WH); p.L(33, 36, 33, 70, 1, C(210, 210, 214)); p.R(28, 48, 2, 8, GOLD); p.R(36, 48, 2, 8, GOLD);
        p.E(24, 32, 22, 5, WH); p.E(24, 32, 16, 3, C(210, 226, 236)); p.L(24, 26, 24, 31, 2, C(200, 200, 210)); p.L(24, 26, 28, 26, 2, C(200, 200, 210));
        p.R(46, 23, 6, 8, C(250, 236, 170)); p.L(47, 23, 46, 17, 1, C(120, 200, 220)); p.L(50, 23, 51, 17, 1, C(255, 140, 170));
        p.Ci(56, 28, 2.5f, C(250, 160, 120)); p.Ci(10, 28, 2, C(255, 190, 210));
        break;
    }
    case SH_SOAKTUB: {
        const u32 CH = C(200, 204, 212);
        p.L(86, -42, 86, 12, 2, CH); p.L(86, -42, 74, -42, 2, CH); p.E(72, -40, 12, 4, CH);
        for (int i = 0; i < 6; i++) p.L(68 + i * 2.f, -38, 66 + i * 2.5f, 10, 1, C(210, 236, 255, 120));
        p.R(0, 10, 92, 36, C0); for (int r = 0; r < 5; r++) p.R(0, 14 + r * 7.f, 92, 1, C(255, 255, 255, 140)); for (int k = 0; k < 13; k++) p.R(k * 7.f, 14, 1, 32, C(255, 255, 255, 140));
        p.R(0, 8, 92, 5, CL); p.R(4, 9, 84, 4, C(255, 180, 206, 230));
        for (int i = 0; i < 4; i++) p.L(10 + i * 20.f, 11, 18 + i * 20.f, 11, 1, C(255, 255, 255, 160));
        break;
    }
    case SH_RETROFRIDGE: {
        p.R(2, 8, 42, 86, C0); p.E(23, 8, 42, 14, C0); p.R(2, 40, 42, 1.5f, CD);
        p.R(35, 18, 3, 16, C(220, 224, 232)); p.R(35, 46, 3, 22, C(220, 224, 232));
        p.R(10, 82, 26, 3, C(255, 255, 255, 120)); p.L(8, 12, 8, 36, 2, C(255, 255, 255, 90)); p.L(8, 46, 8, 86, 2, C(255, 255, 255, 90));
        p.R(5, 94, 4, 2, DK); p.R(37, 94, 4, 2, DK);
        p.H(20, 24, 3, C(col::HOT)); p.Ci(28, 56, 2.5f, C(250, 236, 120)); p.R(14, 60, 6, 6, C(255, 255, 255, 200));   // magnets & a note
        break;
    }
    case SH_PLUSHSOFA:
        p.R(4, 40, 4, 6, CD); p.R(96, 40, 4, 6, CD);
        p.E(28, 18, 50, 26, CL); p.E(76, 18, 50, 26, CL);
        p.R(0, 24, 104, 18, C0); p.E(5, 28, 16, 28, C0); p.E(99, 28, 16, 28, C0);
        p.R(10, 24, 42, 9, CL); p.R(52, 24, 42, 9, CL);
        p.Q(16, 8, 30, 10, 28, 24, 14, 22, C(240, 196, 176)); p.Q(76, 10, 90, 8, 92, 22, 78, 24, C(250, 196, 214)); p.Ci(84, 16, 1.5f, C(255, 255, 255));
        break;
    case SH_BOOKCUBBY: {
        p.R(0, 0, 52, 92, WH); p.R(3, 3, 46, 86, C(240, 236, 230));
        for (int r = 0; r < 4; r++) {
            float y = 3 + r * 22.f;
            p.R(3, y + 20, 46, 2, WH);
            for (int b = 0; b < 7; b++) { const RGB& bc = PALETTE[(b * 3 + r * 5) % 10]; float bh = 14 + (b * 7 + r) % 5; p.R(5 + b * 6.f, y + 20 - bh, 5, bh, C(bc)); }
        }
        p.Ci(42, 7, 3, C(250, 190, 210));   // a little plush on top shelf
        break;
    }
    case SH_MONSTERA: {
        const u32 G1 = C(70, 150, 90), G2 = C(96, 176, 110), GV = C(50, 110, 70);
        for (int i = 0; i < 5; i++) { float a = -2.4f + i * 0.6f, lx = 22 + std::cos(a) * 14, ly = 30 + std::sin(a) * 18; p.L(22, 52, lx, ly, 1.5f, GV); p.E(lx, ly, 20, 15, i % 2 ? G1 : G2); p.L(lx - 6, ly, lx + 6, ly, 1, GV); p.L(lx, ly - 4, lx + 3, ly + 4, 1, C(250, 240, 230)); }
        p.Q(10, 52, 34, 52, 31, 70, 13, 70, C0); p.R(9, 50, 26, 4, CL);
        break;
    }
    case SH_DRUMLAMP:
        p.E(14, 76, 18, 4, DK); p.L(14, 76, 14, 20, 1.5f, TRIM);
        p.R(2, 6, 24, 16, C0); p.R(2, 6, 24, 2, CL); p.R(2, 20, 24, 2, CD); p.E(14, 26, 34, 12, C(255, 236, 170, 60));
        break;
    case SH_GAMINGPC: {   // desk + RGB tower + two monitors
        p.R(0, 40, 80, 6, C(40, 40, 48)); p.R(4, 46, 4, 24, C(30, 30, 36)); p.R(72, 46, 4, 24, C(30, 30, 36));
        p.R(6, 12, 30, 22, C(20, 20, 26)); p.R(8, 14, 26, 18, C(60, 120, 220)); p.R(40, 10, 30, 24, C(20, 20, 26)); p.R(42, 12, 26, 20, C(150, 70, 220));
        p.R(20, 34, 4, 6, C(30, 30, 36)); p.R(54, 34, 4, 6, C(30, 30, 36)); p.R(14, 36, 52, 3, C(50, 50, 60));
        p.R(62, 46, 16, 22, C(24, 24, 30)); for (int k = 0; k < 3; k++) p.Ci(70, 51 + k * 6, 2, C(k == 0 ? RGB{255, 60, 160} : k == 1 ? RGB{60, 220, 255} : RGB{160, 255, 120}));
        p.R(0, 38, 80, 1, C0);
        break;
    }
    case SH_CANOPYBED:   // four posts, scalloped valance, sheer drapes
        p.R(4, 4, 4, 106, TRIM); p.R(102, 4, 4, 106, TRIM);
        p.R(0, 2, 110, 12, C0); for (int k = 0; k < 11; k++) p.E(5 + k * 10.f, 14, 10, 6, C0);
        p.R(8, 60, 94, 26, WH); p.F(8, 70, 94, 34, ci, C0, 50); p.R(8, 100, 94, 4, CD);
        p.E(28, 66, 30, 12, C(255, 255, 255)); p.E(82, 66, 30, 12, C(255, 255, 255)); p.H(55, 64, 6, CL);
        p.Q(0, 14, 14, 14, 18, 108, 0, 108, C(c, 120)); p.Q(96, 14, 110, 14, 110, 108, 92, 108, C(c, 120));
        p.R(0, 56, 18, 3, TRIM); p.R(92, 56, 18, 3, TRIM);
        break;
    case SH_DAYBED:   // white frame, drawers underneath
        p.R(0, 0, 8, 56, WH); p.R(102, 0, 8, 56, WH); p.R(0, 10, 110, 4, WH);
        for (int k = 0; k < 9; k++) p.R(10 + k * 10.f, 14, 3, 14, WH);
        p.F(8, 26, 94, 12, ci, C0, 51); p.E(24, 26, 22, 10, C(255, 255, 255));
        p.R(6, 38, 98, 18, C(245, 240, 236)); for (int k = 0; k < 3; k++) { p.R(10 + k * 32.f, 41, 28, 12, C(235, 228, 224)); p.R(21 + k * 32.f, 46, 6, 2, TRIM); }
        break;
    case SH_HEARTSHELF:
        p.H(22, 22, 22, C0); p.H(22, 22, 17, C(250, 248, 246)); p.R(8, 22, 28, 2, C0); p.R(21, 8, 2, 30, C0);
        p.R(10, 14, 4, 8, C(255, 150, 190)); p.Ci(29, 18, 3, C(255, 220, 120)); p.R(12, 26, 6, 6, C(160, 200, 255)); p.Ci(30, 29, 3, C(200, 150, 255));
        break;
    case SH_PLUSHSHELF:
        p.R(0, 28, 80, 4, C0); p.R(4, 32, 3, 4, CD); p.R(73, 32, 3, 4, CD);
        { const RGB pl[4] = {{200, 140, 90}, {255, 190, 210}, {250, 250, 250}, {180, 220, 255}};
          for (int k = 0; k < 4; k++) { float x = 12 + k * 18.f; p.Ci(x, 20, 7, C(pl[k])); p.Ci(x, 11, 6, C(pl[k])); p.Ci(x - 4, 6, 2.5f, C(pl[k])); p.Ci(x + 4, 6, 2.5f, C(pl[k])); p.Ci(x - 2, 11, 0.9f, DK); p.Ci(x + 2, 11, 0.9f, DK); } }
        break;
    case SH_MANNEQUIN:
        p.L(17, 60, 17, 92, 2, TRIM); p.L(8, 96, 17, 90, 2, TRIM); p.L(26, 96, 17, 90, 2, TRIM);
        p.Q(6, 8, 28, 8, 24, 60, 10, 60, C(230, 210, 190)); p.R(15, 2, 4, 6, TRIM); p.Ci(17, 2, 3, TRIM);
        p.Q(5, 10, 29, 10, 26, 40, 8, 40, C0); p.Q(8, 38, 26, 38, 32, 60, 2, 60, C0); p.L(5, 10, 29, 10, 1, CD);
        break;
    case SH_GUITAR:
        p.L(15, 72, 6, 50, 1.5f, DK); p.L(15, 72, 24, 50, 1.5f, DK);
        p.R(13, 2, 4, 30, C(110, 70, 40)); p.R(11, 0, 8, 6, C(60, 40, 30));
        p.E(15, 50, 26, 26, C0); p.E(15, 36, 20, 18, C0); p.Ci(15, 44, 4, DK); p.R(10, 54, 10, 2, C(60, 40, 30));
        break;
    case SH_SKATEBOARD:
        p.Q(4, 2, 14, 2, 16, 56, 2, 56, C0); p.E(9, 3, 10, 6, C0); p.E(9, 56, 14, 6, C0);
        p.L(9, 10, 9, 50, 2, CL); p.Ci(4, 14, 2.5f, DK); p.Ci(14, 14, 2.5f, DK); p.Ci(4, 46, 2.5f, DK); p.Ci(14, 46, 2.5f, DK);
        break;
    case SH_BOOMBOX:
        p.R(0, 6, 44, 22, C0); p.R(10, 0, 24, 3, DK); p.L(10, 2, 8, 6, 1.5f, DK); p.L(34, 2, 36, 6, 1.5f, DK);
        p.Ci(10, 18, 7, DK); p.Ci(34, 18, 7, DK); p.Ci(10, 18, 3, CD); p.Ci(34, 18, 3, CD); p.R(18, 10, 8, 6, C(200, 230, 255));
        if (music) { p.Sp(4, 2, 2.5f, C(255, 255, 255)); p.Sp(40, 0, 2.5f, C(255, 255, 255)); }
        break;
    case SH_CORKBOARD:
        p.R(0, 0, 60, 44, C(110, 70, 40)); p.R(3, 3, 54, 38, C(214, 170, 110));
        p.R(8, 8, 14, 16, WH); p.R(26, 6, 12, 12, C(255, 240, 140)); p.R(40, 10, 14, 18, C(255, 190, 210)); p.R(10, 26, 18, 10, C(180, 220, 255));
        p.Ci(15, 8, 1.5f, C(230, 40, 60)); p.Ci(32, 6, 1.5f, C(40, 120, 230)); p.Ci(47, 10, 1.5f, C(230, 40, 60));
        break;
    case SH_VINYLWALL:
        for (int k = 0; k < 3; k++) { float x = 12 + k * 23.f; p.Ci(x, 17, 11, DK); p.Ci(x, 17, 8, C(40, 40, 46)); p.Ci(x, 17, 4, k == 1 ? C0 : CL); p.Ci(x, 17, 1, DK); }
        break;
    case SH_POUF:
        p.E(20, 16, 40, 26, C0); p.E(20, 9, 36, 12, CL); for (int k = 0; k < 5; k++) p.Ci(6 + k * 7.f, 16 + (k % 2) * 4, 1.2f, CD);
        p.R(6, 24, 3, 6, TRIM); p.R(31, 24, 3, 6, TRIM);
        break;
    case SH_STILETTOCHAIR: {   // the giant glam heel
        p.R(2, 66, 40, 8, CD); p.E(16, 62, 30, 16, C0);
        p.Q(10, 56, 46, 50, 52, 66, 14, 70, C0); p.F(14, 54, 30, 10, ci, C0, 52);
        p.Q(44, 50, 58, 14, 70, 8, 60, 58, C0); p.Q(48, 50, 60, 18, 66, 14, 58, 54, CL);
        p.L(62, 56, 64, 78, 3.f, CD); p.R(60, 76, 8, 4, DK);
        break; }
    case SH_HOLLYVANITY:
        p.R(4, 4, 62, 48, DK); p.R(8, 8, 54, 40, C(200, 220, 235)); p.R(10, 10, 10, 30, C(255, 255, 255, 70));
        for (int k = 0; k < 6; k++) { p.Ci(6 + k * 11.6f, 4, 2.5f, C(255, 246, 200)); p.Ci(6 + k * 11.6f, 52, 2.5f, C(255, 246, 200)); }
        for (int k = 1; k < 4; k++) { p.Ci(4, 4 + k * 12.f, 2.5f, C(255, 246, 200)); p.Ci(66, 4 + k * 12.f, 2.5f, C(255, 246, 200)); }
        p.R(0, 56, 70, 8, C0); p.R(0, 64, 70, 18, CD); p.R(4, 82, 4, 14, CD); p.R(62, 82, 4, 14, CD); p.R(30, 70, 10, 2, TRIM);
        p.R(10, 50, 4, 6, C(255, 120, 170)); p.Ci(54, 53, 3, C(255, 200, 220));
        break;
    case SH_ROUNDRUG:
        p.E(40, 9, 80, 18, CD); p.E(40, 9, 74, 15, C0); p.E(40, 9, 50, 9, CL); p.H(40, 8, 4, CD);
        break;
    case SH_LANTERN:
        p.L(15, 0, 15, 6, 1, DK); p.E(15, 22, 30, 32, C0); for (int k = 0; k < 4; k++) p.L(3, 12 + k * 6.f, 27, 12 + k * 6.f, 0.8f, CD);
        p.R(9, 5, 12, 3, CD); p.R(9, 36, 12, 3, CD); p.E(15, 22, 34, 36, C(255, 236, 170, 50));
        break;
    case SH_HEXSHELF:
        for (int k = 0; k < 4; k++) { float cx = 12 + (k % 2) * 22.f + (k / 2) * 14, cy = 12 + (k / 2) * 22.f;
            for (int e = 0; e < 6; e++) { float a0 = e * 1.0472f, a1 = a0 + 1.0472f; p.L(cx + std::cos(a0) * 11, cy + std::sin(a0) * 11, cx + std::cos(a1) * 11, cy + std::sin(a1) * 11, 1.6f, C0); } }
        p.Ci(12, 14, 3, C(120, 200, 140)); p.R(32, 8, 4, 8, C(255, 180, 200)); p.Ci(48, 36, 3, C(255, 220, 120));
        break;
    case SH_FLOORLAMP:
        p.E(14, 98, 20, 5, DK); p.L(14, 98, 14, 24, 1.5f, TRIM);
        p.Q(4, 22, 24, 22, 20, 4, 8, 4, C0); p.R(4, 21, 20, 2, CD); p.E(14, 28, 30, 14, C(255, 236, 170, 60));
        break;
    case SH_PLAIDSOFA:
        p.R(4, 6, 82, 22, C(170, 70, 50)); for (int k = 0; k < 6; k++) p.R(6 + k * 14.f, 6, 3, 22, C(120, 40, 30, 160)); for (int k = 0; k < 2; k++) p.R(4, 12 + k * 9.f, 82, 2, C(240, 200, 120, 160));
        p.R(0, 22, 90, 18, C0); p.R(0, 14, 10, 26, CD); p.R(80, 14, 10, 26, CD); p.R(12, 24, 32, 10, CL); p.R(46, 24, 32, 10, CL);
        p.Ci(64, 18, 6, C(230, 120, 50)); p.R(4, 40, 4, 10, DK); p.R(82, 40, 4, 10, DK);
        break;
    case SH_WORLDMAP:
        p.R(0, 0, 90, 56, C(50, 70, 130)); p.R(0, 0, 90, 56, C(c, 50));
        p.E(20, 18, 22, 14, C(220, 220, 170)); p.E(26, 38, 12, 18, C(220, 220, 170)); p.E(46, 16, 14, 10, C(220, 220, 170)); p.E(50, 32, 14, 18, C(220, 220, 170)); p.E(68, 18, 26, 14, C(220, 220, 170)); p.E(78, 42, 12, 8, C(220, 220, 170));
        p.L(22, 22, 50, 14, 1, C(255, 255, 255, 200)); p.L(50, 14, 76, 40, 1, C(255, 255, 255, 200)); p.Sp(76, 40, 3, C(255, 240, 160));
        break;
    case SH_LIPSPILLOW:
        p.E(20, 9, 38, 12, C0); p.E(20, 15, 36, 12, CD); p.E(12, 7, 16, 9, C0); p.E(28, 7, 16, 9, C0); p.L(4, 12, 36, 12, 1.2f, CD); p.E(14, 6, 6, 2, C(255, 255, 255, 120));
        break;
    case SH_CLOTHESPILE: {   // a messy heap of tees, jeans and a sock
        p.E(25, 15, 50, 12, C(c, 255)); p.Q(4, 14, 22, 6, 30, 12, 8, 18, C(90, 120, 190));
        p.Q(20, 8, 40, 4, 46, 14, 26, 14, CL); p.E(34, 12, 18, 7, C(255, 255, 255)); p.R(10, 12, 8, 4, C(255, 160, 190));
        p.L(30, 16, 44, 18, 2.5f, C(200, 200, 210)); p.E(14, 16, 10, 4, CD);
        break; }
    case SH_BOOKSTACK:
        { const RGB bk[5] = {{200, 60, 80}, {60, 140, 200}, {240, 200, 70}, {120, 190, 130}, {c.r, c.g, c.b}};
          for (int k = 0; k < 5; k++) { p.R(1 + (k % 2) * 2.f, 21 - k * 5.f, 19, 5, C(bk[k])); p.R(3 + (k % 2) * 2.f, 22 - k * 5.f, 15, 1, C(255, 255, 255, 140)); } }
        break;
    case SH_PURSEHOOKS:
        p.R(2, 2, 56, 4, TRIM); for (int k = 0; k < 3; k++) { float x = 12 + k * 18.f; p.Ci(x, 6, 2, TRIM);
            u32 bc = k == 0 ? C0 : k == 1 ? C(120, 70, 50) : DK; p.L(x - 5, 16, x, 7, 1, bc); p.L(x + 5, 16, x, 7, 1, bc); p.Q(x - 8, 16, x + 8, 16, x + 7, 32, x - 7, 32, bc); p.R(x - 2, 20, 4, 2, TRIM); }
        break;
    case SH_IRONBED:   // white scrolled iron frame
        p.R(2, 4, 4, 60, WH); p.R(104, 22, 4, 42, WH); p.L(4, 6, 30, 6, 2, WH); for (int k = 0; k < 4; k++) p.R(10 + k * 6.f, 8, 2, 30, WH);
        p.Ci(18, 16, 6, C(255, 255, 255, 0)); p.Ci(4, 4, 3, WH); p.Ci(106, 22, 3, WH);
        p.R(6, 34, 98, 10, WH); p.F(8, 38, 96, 18, ci, C0, 53); p.R(8, 54, 96, 6, CD); p.E(22, 36, 24, 10, C(255, 255, 255));
        break;
    case SH_MUSICSTAND:
        p.L(17, 30, 17, 76, 1.5f, DK); p.L(17, 76, 6, 80, 1.5f, DK); p.L(17, 76, 28, 80, 1.5f, DK);
        p.Q(2, 6, 32, 2, 32, 26, 2, 30, DK); p.Q(4, 8, 16, 6, 16, 26, 4, 28, WH); p.Q(18, 6, 30, 4, 30, 24, 18, 26, WH);
        for (int k = 0; k < 4; k++) { p.L(6, 12 + k * 4.f, 14, 12 + k * 4.f, 0.6f, DK); p.L(20, 10 + k * 4.f, 28, 10 + k * 4.f, 0.6f, DK); }
        break;
    case SH_WINDOWSEAT:
        p.R(0, 14, 90, 26, C(150, 100, 60)); for (int k = 0; k < 3; k++) { p.R(4 + k * 29.f, 20, 25, 16, C(170, 120, 75)); p.R(14 + k * 29.f, 27, 6, 2, TRIM); }
        p.R(0, 6, 90, 10, C0); p.R(0, 6, 90, 2, CL); p.E(16, 4, 18, 10, CL); p.E(74, 4, 18, 10, C(255, 255, 255));
        break;
    case SH_PLANTSHELF:
        p.R(0, 22, 70, 4, C(150, 100, 60)); p.R(4, 26, 3, 6, C(120, 80, 50)); p.R(63, 26, 3, 6, C(120, 80, 50));
        p.R(6, 14, 10, 8, C0); p.E(11, 10, 14, 10, C(80, 160, 90)); p.R(28, 12, 8, 10, C(250, 250, 250)); p.R(30, 6, 4, 6, C(200, 60, 80)); p.R(32, 4, 2, 2, C(60, 140, 200));
        p.R(48, 14, 10, 8, CD); p.L(53, 14, 46, 36, 1.5f, C(70, 150, 90)); p.L(53, 14, 60, 34, 1.5f, C(70, 150, 90)); p.E(47, 26, 6, 4, C(96, 176, 110)); p.E(59, 26, 6, 4, C(96, 176, 110));
        break;
    case SH_DESKLAMP:
        p.E(13, 30, 18, 4, C0); p.L(13, 30, 6, 16, 1.5f, C0); p.L(6, 16, 16, 6, 1.5f, C0);
        p.Q(12, 2, 24, 6, 22, 14, 10, 10, C0); p.E(20, 14, 16, 10, C(255, 236, 170, 70));
        break;
    case SH_POLAROIDS:
        p.L(0, 4, 64, 4, 0.8f, C(120, 90, 70));
        for (int k = 0; k < 6; k++) { float x = 2 + (k % 3) * 21.f, y = 6 + (k / 3) * 19.f; p.R(x, y, 16, 17, WH); p.R(x + 2, y + 2, 12, 10, C(k % 2 ? RGB{255, 190, 200} : RGB{170, 210, 240})); p.Ci(x + 8, y, 1.4f, C0); }
        break;
    case SH_CANDLES:
        for (int k = 0; k < 3; k++) { float x = 5 + k * 7.f, h = 10 + (k % 2) * 5.f; p.R(x - 2, 20 - h, 5, h, WH); p.E(x + 0.5f, 18 - h - 2, 3, 5, C(255, 200, 80, (uint8_t)(200 + 50 * std::sin(t * 6 + k)))); }
        p.E(12, 20, 24, 4, C0);
        break;
    case SH_FLOORHEADPHONES:
        p.L(5, 9, 13, 2, 2, C0); p.L(13, 2, 21, 9, 2, C0); p.E(5, 9, 8, 7, CD); p.E(21, 9, 8, 7, CD); p.L(21, 11, 26, 12, 0.8f, DK);
        break;
    case SH_MAGAZINES:
        p.Q(0, 8, 18, 4, 22, 10, 4, 12, C0); p.Q(16, 6, 36, 6, 38, 12, 18, 12, C(120, 190, 130)); p.R(22, 7, 8, 3, C(255, 255, 255, 200)); p.R(4, 8, 6, 2, C(255, 255, 255, 200));
        break;
    case SH_PATCHWORKBED:
        p.R(0, 6, 110, 20, C(170, 120, 80)); p.R(0, 6, 110, 3, C(190, 140, 95));
        p.R(4, 24, 102, 28, WH); for (int r = 0; r < 3; r++) for (int k = 0; k < 8; k++) p.R(30 + k * 9.5f, 26 + r * 8.f, 9, 7, ((r + k) % 3 == 0) ? C0 : ((r + k) % 3 == 1) ? CL : C(255, 230, 220));
        p.E(16, 24, 22, 10, C(255, 255, 255)); p.R(4, 50, 102, 10, C(c, 160));
        break;
    case SH_FLATSHOES:
        p.E(7, 6, 13, 7, C0); p.E(19, 7, 13, 7, C0); p.E(7, 5, 8, 4, CD); p.E(19, 6, 8, 4, CD); p.Ci(3, 5, 1.2f, C(255, 255, 255)); p.Ci(15, 6, 1.2f, C(255, 255, 255));
        break;
    // ---------------- aesthetic room pack ----------------
    case SH_CLOUDBED: {   // scalloped cloud headboard, pink floral comforter, heart pillows, bunny plush
        for (int i = 0; i < 6; i++) p.Ci(12 + i * 18.4f, 16 + (i % 2) * 2, 12, C(255, 248, 250)); p.R(2, 16, 112, 14, C(255, 248, 250));
        for (int i = 0; i < 6; i++) p.Ci(12 + i * 18.4f, 16 + (i % 2) * 2, 9, C(255, 235, 242));
        p.R(0, 30, 116, 22, C0); for (int k = 0; k < 14; k++) { const float fx = 6 + (k * 37 % 104), fy = 34 + (k * 13 % 14); p.Ci(fx, fy, 2.2f, C(255, 160, 190)); p.Ci(fx, fy, 0.9f, C(255, 240, 160)); }
        p.R(0, 50, 116, 10, C(255, 245, 248)); for (int k = 0; k < 12; k++) p.Ci(4 + k * 10.f, 60, 4, C(255, 245, 248));
        p.H(24, 28, 7, C(255, 120, 160)); p.H(40, 29, 6, C(255, 200, 215));
        p.E(96, 26, 12, 12, C(255, 255, 255)); p.E(92, 16, 3, 9, C(255, 255, 255)); p.E(100, 16, 3, 9, C(255, 255, 255)); p.Ci(94, 25, 0.9f, DK); p.Ci(98, 25, 0.9f, DK); p.E(96, 28, 3, 2, C(255, 170, 190));
        break; }
    case SH_EMOBED: {   // black frame, red leopard comforter, skull pillow, plushies
        p.R(0, 4, 112, 26, C(25, 20, 25)); p.R(4, 8, 104, 4, C(55, 45, 55));
        p.R(0, 28, 112, 22, C(190, 20, 40)); for (int k = 0; k < 22; k++) { const float lx = 4 + (k * 29 % 104), ly = 31 + (k * 7 % 16); p.E(lx, ly, 6, 4, C(30, 10, 15)); p.Ci(lx, ly, 1.1f, C(220, 60, 80)); }
        p.R(0, 50, 112, 8, C(25, 20, 25));
        p.E(22, 26, 18, 12, C(245, 245, 245)); p.Ci(18, 25, 2.2f, DK); p.Ci(26, 25, 2.2f, DK); p.R(20, 30, 4, 2, DK);
        p.E(80, 22, 12, 14, C(170, 120, 220)); p.E(94, 22, 12, 14, C(240, 220, 90)); p.Ci(78, 19, 1, DK); p.Ci(82, 19, 1, DK); p.Ci(92, 19, 1, DK); p.Ci(96, 19, 1, DK);
        break; }
    case SH_GAMERBED: {   // low white platform, pink heart blanket, shark plush
        p.R(0, 10, 112, 18, C(250, 250, 252)); p.R(4, 14, 30, 12, C(240, 238, 245)); p.R(38, 14, 30, 12, C(240, 238, 245));
        p.R(0, 28, 112, 14, C(255, 255, 255)); p.R(46, 26, 66, 16, C0); p.H(80, 33, 9, C(255, 255, 255, 200)); p.H(62, 35, 5, CL);
        p.R(0, 42, 112, 8, C(200, 170, 140));
        p.E(30, 24, 24, 10, C(150, 160, 175)); p.T(24, 18, 30, 12, 34, 19, C(150, 160, 175)); p.E(30, 27, 18, 4, C(255, 255, 255)); p.Ci(22, 23, 1.2f, DK);
        break; }
    case SH_BRATBED: {   // pink & white stripes, leopard throw, laptop
        p.R(0, 4, 112, 26, C(255, 250, 252)); for (int k = 0; k < 8; k++) p.R(4 + k * 14.f, 4, 6, 26, C(255, 190, 210));
        p.R(0, 30, 112, 20, C0); p.R(52, 28, 60, 20, C(215, 160, 90)); for (int k = 0; k < 16; k++) p.E(56 + (k * 23 % 52), 31 + (k * 7 % 14), 4, 3, C(70, 40, 20));
        p.R(0, 50, 112, 10, C(255, 250, 252)); for (int k = 0; k < 8; k++) p.R(4 + k * 14.f, 50, 6, 10, C(255, 190, 210));
        p.Q(18, 30, 40, 30, 44, 24, 22, 24, C(200, 205, 215)); p.R(22, 24, 18, 2, C(150, 200, 255));
        break; }
    case SH_DAISYBED: {   // wood frame, pink quilt with a giant daisy
        p.R(0, 6, 112, 24, C(150, 100, 70)); p.R(4, 10, 104, 3, C(175, 125, 90));
        p.E(20, 26, 22, 10, C(250, 245, 240)); p.E(44, 26, 22, 10, C(250, 245, 240));
        p.R(0, 28, 112, 24, C0); for (int k = 0; k < 10; k++) p.R(k * 11.2f, 28, 1, 24, C(255, 255, 255, 50));
        for (int k = 0; k < 8; k++) { const float a = k * 0.785f; p.E(70 + std::cos(a) * 10, 39 + std::sin(a) * 6, 9, 5, C(255, 255, 255)); } p.Ci(70, 39, 5, C(255, 210, 70));
        p.R(0, 52, 112, 8, C(150, 100, 70));
        break; }
    case SH_ZEBRABED: {   // hot pink frame, zebra comforter, bunny-ear pillows
        p.R(0, 2, 112, 30, C(255, 60, 150)); p.R(6, 6, 100, 22, C(255, 120, 190));
        p.R(0, 30, 112, 22, C(250, 250, 250)); for (int k = 0; k < 14; k++) p.Q(4 + k * 8.f, 30, 8 + k * 8.f, 30, 4 + k * 8.f + (k % 2) * 3, 52, k * 8.f + (k % 2) * 3, 52, C(25, 25, 25));
        p.R(0, 52, 112, 10, C(255, 60, 150));
        p.E(30, 26, 22, 12, C(255, 170, 210)); p.E(24, 16, 4, 10, C(255, 170, 210)); p.E(36, 16, 4, 10, C(255, 170, 210));
        p.E(70, 26, 22, 12, C(30, 30, 30)); p.R(64, 24, 12, 2, C(255, 255, 255));
        break; }
    case SH_BOWBED: {   // cream tufted headboard with a giant ribbon bow, sheer canopy
        p.R(6, 0, 104, 6, C(255, 240, 245)); p.R(4, 4, 6, 60, C(255, 225, 235, 140)); p.R(106, 4, 6, 60, C(255, 225, 235, 140));
        p.R(12, 12, 92, 28, C(250, 240, 230)); for (int r = 0; r < 2; r++) for (int k = 0; k < 6; k++) p.Ci(22 + k * 14.f, 20 + r * 10.f, 1.4f, C(220, 205, 195));
        p.T(58, 10, 42, 2, 42, 18, C0); p.T(58, 10, 74, 2, 74, 18, C0); p.Ci(58, 10, 3, CD); p.L(57, 12, 50, 26, 2, C0); p.L(59, 12, 66, 26, 2, C0);
        p.R(8, 40, 100, 22, C(255, 225, 235)); for (int k = 0; k < 10; k++) p.Ci(12 + k * 10.f, 46 + (k % 2) * 6, 1.8f, C(255, 170, 200));
        p.R(8, 62, 100, 10, C(250, 240, 230));
        break; }
    case SH_FAIRYCURTAIN:
        p.R(0, 0, 96, 3, C(90, 60, 50));
        for (int k = 0; k < 9; k++) { const float lx = 5 + k * 11.f, len = 30 + (k * 17 % 26); p.L(lx, 2, lx, len, 0.6f, C(120, 100, 90));
            for (int j = 0; j < 5; j++) { const float ly = 8 + j * (len - 8) / 5; const float tw = 0.6f + 0.4f * std::sin(t * 3 + k + j); p.Ci(lx, ly, 2, C(255, 210 + (int)(30 * tw), 140, (uint8_t)(160 + 90 * tw))); } }
        break;
    case SH_NEONHEART: { const float g = 0.75f + 0.25f * std::sin(t * 4); p.H(22, 18, 22, C(255, 60, 160, (uint8_t)(80 * g))); p.H(22, 18, 16, C(255, 90, 190, (uint8_t)(255 * g))); p.H(22, 17, 10, C(255, 200, 235, (uint8_t)(255 * g))); break; }
    case SH_FIGSHELF:
        p.R(0, 20, 90, 5, CD); p.R(0, 18, 90, 3, C0);
        for (int k = 0; k < 6; k++) { const RGB fc[6] = {{120, 220, 220}, {255, 150, 200}, {170, 220, 120}, {255, 220, 120}, {200, 160, 255}, {255, 140, 140}}; const float fx = 8 + k * 15.f;
            p.R(fx - 3, 8, 6, 10, C(fc[k])); p.Ci(fx, 6, 4, C(255, 225, 205)); p.R(fx - 4, 2, 8, 3, C(fc[(k + 2) % 6])); }
        break;
    case SH_GAMERDESK:
        p.R(0, 40, 96, 6, C(250, 250, 252)); p.R(2, 46, 4, 28, C(230, 230, 235)); p.R(78, 46, 16, 28, C(240, 240, 245)); for (int k = 0; k < 3; k++) p.R(80, 50 + k * 8.f, 12, 6, C(225, 225, 232));
        p.R(20, 6, 46, 30, C(30, 30, 40)); p.R(22, 8, 42, 26, C(120 + (int)(60 * std::sin(t)), 90, 220)); p.R(40, 36, 6, 4, C(200, 200, 210));
        p.R(70, 14, 14, 26, C(30, 30, 40)); for (int k = 0; k < 4; k++) p.R(72, 16 + k * 6.f, 10, 2, C((int)(127 + 127 * std::sin(t * 2 + k)), (int)(127 + 127 * std::sin(t * 2 + k + 2)), 255));
        p.R(24, 37, 30, 3, C(220, 220, 228)); p.L(0, 40, 96, 40, 1, C(255, 120, 220, 160));
        break;
    case SH_CRTTV:
        p.R(4, 4, 54, 44, C0); p.R(8, 8, 46, 34, C(30, 25, 40)); p.R(10, 10, 42, 30, C(220, 140, 230)); p.R(14, 22, 34, 6, C(255, 220, 250)); p.Ci(54, 44, 1.5f, CD);
        p.R(0, 48, 62, 12, C(250, 250, 252)); p.R(4, 51, 24, 6, C(230, 230, 235)); p.R(34, 51, 24, 6, C(230, 230, 235));
        break;
    case SH_LAVALAMP:
        p.T(7, 0, 2, 8, 12, 8, C(60, 60, 70)); p.R(2, 8, 10, 18, C(255, 120, 200, 200));
        for (int k = 0; k < 3; k++) p.Ci(7, 11 + std::fmod(t * 4 + k * 6.f, 14.f), 2.2f - k * 0.4f, C(255, 230, 120));
        p.T(2, 26, 12, 26, 7, 30, C(60, 60, 70)); p.R(1, 30, 12, 6, C(60, 60, 70));
        break;
    case SH_MOONCHAIR:
        p.E(26, 20, 50, 30, C0); p.E(26, 16, 42, 20, CL); for (int k = 0; k < 6; k++) p.Ci(10 + k * 6.f, 14 + (k % 2) * 4, 1.5f, CD);
        p.L(12, 30, 6, 44, 2, C(80, 80, 90)); p.L(40, 30, 46, 44, 2, C(80, 80, 90)); p.L(26, 32, 26, 44, 2, C(80, 80, 90));
        break;
    case SH_CHECKRUG: for (int r = 0; r < 2; r++) for (int k = 0; k < 13; k++) p.R(k * 10.f, r * 8.f, 10, 8, (r + k) % 2 ? C(250, 245, 250) : C0); break;
    case SH_SUNRUG: { for (int k = 0; k < 14; k++) { const float a = k * 0.449f; p.E(60 + std::cos(a) * 42, 8 + std::sin(a) * 6, 22, 7, C(250, 190, 40)); } p.E(60, 8, 60, 10, C(120, 70, 30)); for (int k = 0; k < 9; k++) p.R(36 + k * 5.5f, 5, 3, 2, C(90, 50, 20)); break; }
    case SH_LEORUG: p.E(55, 8, 108, 15, C(220, 170, 100)); for (int k = 0; k < 18; k++) p.E(8 + (k * 31 % 96), 3 + (k * 7 % 10), 7, 3, C(70, 40, 20)); break;
    case SH_HEARTMIRROR: p.H(20, 18, 20, GOLD); p.H(20, 17, 16, C(210, 235, 250)); p.L(10, 12, 18, 22, 1.2f, C(255, 255, 255, 180)); break;
    case SH_ISLAND:
        p.R(0, 0, 140, 8, C(70, 65, 75)); for (int k = 0; k < 10; k++) p.Ci(6 + k * 14.f, 3 + (k % 2) * 2, 1.5f, C(150, 150, 160));
        p.R(4, 8, 132, 46, C(250, 248, 242)); for (int k = 0; k < 4; k++) p.R(10 + k * 32.f, 14, 26, 34, C(240, 236, 228)); for (int k = 0; k < 4; k++) p.R(20 + k * 32.f, 28, 6, 2, C(160, 150, 140));
        for (int k = 0; k < 3; k++) { p.R(18 + k * 44.f, 30, 18, 6, C(110, 75, 55)); p.R(20 + k * 44.f, 36, 2, 18, C(60, 45, 35)); p.R(32 + k * 44.f, 36, 2, 18, C(60, 45, 35)); }
        break;
    case SH_STEELFRIDGE:
        p.R(0, 0, 62, 100, C(190, 195, 205)); p.R(1, 1, 30, 62, C(205, 210, 220)); p.R(31, 1, 30, 62, C(205, 210, 220)); p.R(1, 64, 60, 35, C(200, 205, 215));
        p.R(27, 14, 2, 30, C(120, 125, 135)); p.R(33, 14, 2, 30, C(120, 125, 135)); p.R(20, 70, 22, 2, C(120, 125, 135)); p.R(6, 14, 14, 18, C(60, 70, 85));
        p.R(4, 2, 2, 60, C(255, 255, 255, 100));
        break;
    case SH_FLOATSHELF:
        for (int r = 0; r < 2; r++) { p.R(0, 16 + r * 20.f, 80, 4, C0); p.R(6, 4 + r * 20.f, 4, 12, C(90, 160, 90)); p.Ci(8, 4 + r * 20.f, 5, C(110, 180, 100));
            for (int k = 0; k < 5; k++) p.R(24 + k * 6.f, 6 + r * 20.f, 5, 10, C(k % 2 ? RGB{255, 190, 210} : RGB{200, 190, 255})); p.Ci(66, 12 + r * 20.f, 4, C(250, 245, 240)); }
        break;
    case SH_FIREPLACE:
        p.R(0, 0, 110, 120, C(120, 115, 120)); for (int r = 0; r < 20; r++) for (int k = 0; k < 7; k++) { const float bx = (r % 2) * 8 + k * 16.f; if (bx >= 110) continue; p.R(bx, r * 6.f, std::min(15.f, 110 - bx), 5, C(r % 3 ? RGB{140, 135, 140} : RGB{105, 100, 108})); }
        p.R(20, 30, 70, 40, C(25, 25, 30)); p.R(22, 32, 66, 36, C(60, 130 + (int)(30 * std::sin(t)), 200));
        p.R(10, 74, 90, 5, C(90, 60, 45)); p.R(16, 84, 78, 20, C(20, 20, 24));
        for (int k = 0; k < 8; k++) p.T(20 + k * 9.f, 104, 24 + k * 9.f, 92 - (int)(4 * std::sin(t * 7 + k)), 28 + k * 9.f, 104, C(255, 140 + k * 10, 40));
        break;
    case SH_SECTIONAL:
        p.R(0, 6, 150, 20, C(250, 245, 238)); for (int k = 0; k < 4; k++) p.R(4 + k * 36.f, 8, 32, 16, C(255, 252, 246)); for (int k = 0; k < 12; k++) p.R(6 + k * 12, 8, 1, 16, C(230, 225, 215));
        p.R(0, 24, 150, 18, C(245, 240, 232)); p.E(120, 22, 22, 10, C(70, 50, 35)); for (int k = 0; k < 5; k++) p.E(112 + k * 4.f, 21, 3, 2, C(200, 160, 100));
        p.R(0, 42, 150, 6, C(225, 220, 210)); p.H(30, 18, 6, C0);
        break;
    case SH_RECORDCONSOLE:
        p.R(0, 18, 60, 30, C0); p.R(4, 22, 24, 22, CL); p.R(32, 22, 24, 22, CL); p.Ci(16, 33, 1.5f, CD); p.Ci(44, 33, 1.5f, CD); p.R(4, 48, 4, 8, CD); p.R(52, 48, 4, 8, CD);
        p.E(30, 14, 36, 8, C(40, 35, 45)); p.E(30, 13, 26, 5, C(25, 20, 30)); p.Ci(30, 13, 3, C(255, 120, 180)); p.L(48, 8, 38, 13, 1.2f, C(200, 200, 210));
        if (music) for (int k = 0; k < 2; k++) p.Ci(50 + k * 4.f, 4 - std::fmod(t * 8 + k * 3, 6.f), 1.2f, C(255, 120, 200));
        break;
    case SH_BEADCURTAIN:
        p.R(0, 0, 46, 4, GOLD);
        for (int k = 0; k < 9; k++) { const float lx = 3 + k * 5.f + std::sin(t * 2 + k) * 0.6f; for (int j = 0; j < 14; j++) p.Ci(lx, 7 + j * 6.4f, 1.3f, C(255, 120 + (j * 9 % 100), 200, 220)); }
        break;
    case SH_BENCH:
        p.F(0, 4, 80, 8, ci, C0, 36); p.F(0, 16, 80, 6, ci, CD, 37);
        p.L(4, 22, 2, 36, 3.f, DK); p.L(76, 22, 78, 36, 3.f, DK); p.L(6, 12, 6, 22, 2.f, DK); p.L(74, 12, 74, 22, 2.f, DK);
        break;
    }
    (void)DK; (void)GOLDD;
}

void drawItemFront(int id, float x, float y, float s) {
    const int sh = itemShape(id), ci = itemColor(id);
    const RGB c = PALETTE[ci];
    const ShapeInfo& si = SHAPES[sh];
    Pen p(x, y, si.w, si.h, s);
    switch (sh) {
    case SH_SOAKTUB: p.R(0, 18, 92, 28, C(c)); for (int r = 0; r < 4; r++) p.R(0, 21 + r * 7.f, 92, 1, C(255, 255, 255, 140)); for (int k = 0; k < 13; k++) p.R(k * 7.f, 18, 1, 28, C(255, 255, 255, 140)); break;
    case SH_BRASSBED: p.F(30, 30, 76, 18, ci, C(c), 41); p.R(30, 46, 76, 3, C(shade(c, 0.72f))); break;
    case SH_PLUSHBED: p.R(28, 22, 82, 16, C(118, 78, 66)); for (int i = 0; i < 9; i++) p.Ci(32 + i * 9.f, 28 + (i % 2) * 5, 1.2f, C(255, 255, 255, 200)); break;
    case SH_BED:   // the comforter tucked over a sleeper
        p.F(32, 36, 72, 18, ci, C(c), 2);
        p.R(32, 52, 72, 3, C(shade(c, 0.72f)));
        p.H(70, 44, 3, C(255, 255, 255, 120));
        break;
    case SH_BATHTUB:
        p.F(2, 14, 78, 22, ci, C(c), 34);
        p.E(41, 15, 82, 6, C(shade(c, 1.25f)));
        for (int i = 0; i < 8; i++) p.Ci(12 + i * 8, 14 - (i % 3) * 2, 3.5f + (i % 2), C(255, 255, 255, 230));
        break;
    case SH_SHOWER:
        p.R(3, 30, 38, 62, C(220, 240, 255, 150));
        p.L(8, 34, 16, 70, 1.f, C(255, 255, 255, 160));
        p.F(0, 92, 44, 6, ci, C(c), 35);
        break;
    case SH_CLOUDBED: case SH_EMOBED: case SH_GAMERBED: case SH_BRATBED: case SH_DAISYBED: case SH_ZEBRABED: case SH_BOWBED: {   // comforter over a sleeper, in the bed's own colours
        const u32 cov = sh == SH_EMOBED ? C(190, 20, 40) : sh == SH_ZEBRABED ? C(250, 250, 250) : sh == SH_GAMERBED || sh == SH_BRATBED || sh == SH_DAISYBED ? C(c) : C(255, 225, 235);
        const float top = sh == SH_GAMERBED ? 28 : sh == SH_CLOUDBED ? 32 : sh == SH_BOWBED ? 42 : 30;
        p.R(32, top, si.w - 36, 20, cov); p.R(32, top + 18, si.w - 36, 3, C(shade(c, 0.72f)));
        if (sh == SH_ZEBRABED) for (int k = 0; k < 9; k++) p.Q(34 + k * 8.f, top, 38 + k * 8.f, top, 34 + k * 8.f + (k % 2) * 3, top + 20, 30 + k * 8.f + (k % 2) * 3, top + 20, C(25, 25, 25));
        break; }
    default: break;
    }
}

UseSpot useSpot(int shape) {
    switch (shape) {
    case SH_BED: return {2, 40, {0, 0}, 2};
    case SH_SOFA: return {2, 24, {-22, 22}, 1};
    case SH_CHAIR: return {1, 26, {0, 0}, 1};
    case SH_BEANBAG: return {1, 14, {0, 0}, 1};
    case SH_LOUNGER: return {1, 20, {6, 0}, 2};
    case SH_BENCH: return {2, 22, {-18, 18}, 1};
    case SH_BRASSBED: return {2, 38, {0, 0}, 2};
    case SH_PLUSHBED: return {2, 32, {0, 0}, 2};
    case SH_SOAKTUB: return {1, 8, {0, 0}, 3};
    case SH_PLUSHSOFA: return {2, 24, {-22, 22}, 1};
    case SH_FLOWERCUSHION: return {1, 8, {0, 0}, 1};
    case SH_EGGCHAIR: return {1, 26, {0, 0}, 1};
    case SH_GAMERCHAIR: return {1, 28, {0, 0}, 1};
    case SH_TOILET: return {1, 22, {0, 0}, 1};
    case SH_BATHTUB: return {1, 8, {0, 0}, 3};
    case SH_SHOWER: return {1, 6, {0, 0}, 3};
    case SH_CLOUDBED: return {2, 38, {0, 0}, 2};
    case SH_EMOBED: return {2, 34, {0, 0}, 2};
    case SH_GAMERBED: return {2, 30, {0, 0}, 2};
    case SH_BRATBED: return {2, 34, {0, 0}, 2};
    case SH_DAISYBED: return {2, 34, {0, 0}, 2};
    case SH_ZEBRABED: return {2, 36, {0, 0}, 2};
    case SH_BOWBED: return {2, 40, {0, 0}, 2};
    case SH_MOONCHAIR: return {1, 16, {0, 0}, 1};
    case SH_SECTIONAL: return {2, 24, {-30, 30}, 1};
    default: return {0, 0, {0, 0}, 0};
    }
}
