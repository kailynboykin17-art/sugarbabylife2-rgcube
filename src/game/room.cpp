#include "emote.hpp"
#include "tween.hpp"
#include "sprites.hpp"
#include "lockup.hpp"
#include "bgs.hpp"
#include "cc.hpp"
#include "seasons.hpp"
#include "oc.hpp"
#include "game.hpp"
#include "catalog.hpp"
#include "people.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "sim.hpp"
#include "goals.hpp"
#include "extras.hpp"
#include "district.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace col;

const char* ROOM_NAMES[R_COUNT] = {"Living Room", "Kitchen", "Bedroom", "Closet & Bath", "Nursery", "Party Basement",
                                   "Backyard", "Sunset Beach", "Glitter High", "Club Glitz", "Bella Notte",
                                   "Diva Boutique", "Starlight College", "Cafe Latte", "Pearl Spa & Salon",
                                   "Glam Mall", "Rosebud Park", "Starlite Cinema", "Ice Palace",
                                   "Beauty & Food Court", "Fun Zone Arcade", "Sugar Rush Bakery", "Playground & Treats",
                                   "Sunny Side Diner", "Sakura Sushi Bar", "Maple Lane", "Neighbor's House",
                                   "Starlight Hospital", "Paws & Claws Pets", "Page Turner Library", "FitLife Gym", "Grand Starlight Hotel", "Lovebird Chapel", "Sugar Market",
    "Second Bedroom", "Bathroom", "Game Room", "Teen Room", "Quik Stop Corner Store", "Westbrook Tennis Club", "Second Chance Thrift", "Hollow Oak Manor", "Alpha Beta Kappa House", "Coral Cove Beach", "Coral Cove Village", "Frostpeak Slopes", "Frostpeak Lodge", "Belle Ville Boulevard", "Belle Ville Tower", "Westbrook County Jail", "Rosewood Academy",
    "Coral Cove Resort Suite", "Waterfall Lagoon", "Frostpeak Chalet Suite", "Frostpeak Village Market", "Belle Ville Hotel Suite", "Musee de Belle Ville",
    "Hotel Lobby", "Suite Bathroom", "Hotel Pool & Spa", "Glow Bowl", "Karaoke Lounge", "Paint & Sip", "Dance Studio", "Night Market", "Serenity Center", "Downtown", "Chic Boutique", "Le Bistro", "The Velvet Room", "Sparkle Land", "Sparkle Land Rides", "Subway Station", "Pixel Palace Arcade", "Suds Laundromat", "Maple Aquarium", "Museum of Glam", "Botanical Gardens", "Skyline Rooftop",
    "Food Court", "School Hallway", "Classroom", "Cafeteria", "School Gym", "Daycare", "Church", "Community Center", "Community Pool", "Block Party", "College Dorm",
    "Glam Salon", "Roller Rink", "Boardwalk Night Market", "Courthouse",
    "Apple Valley Square", "Sunny Acres Farm", "The Big Red Barn"};
const char* HOUSE_NAMES[2] = {"Cocoa Cottage", "Pink Penthouse"};
const RGB WALL_COLORS[NUM_WALLS] = {{255, 214, 228}, {250, 236, 216}, {150, 102, 80}, {84, 52, 42},
                                    {246, 120, 170}, {206, 176, 232}, {176, 214, 240}, {196, 232, 210},
                                    {240, 214, 170}, {40, 32, 36}, {255, 246, 240}, {196, 40, 90}};
const RGB FLOOR_COLORS[NUM_FLOORS] = {{176, 120, 84}, {120, 78, 56}, {74, 46, 38}, {236, 222, 206},
                                      {255, 200, 220}, {246, 120, 170}, {40, 34, 38}, {228, 228, 232},
                                      {214, 186, 140}, {196, 168, 230}, {166, 206, 236}, {200, 160, 110}};
const char* WALL_PATTERNS[NUM_WALLPATS] = {"Plain", "Stripes", "Damask", "Zebra", "Balcony", "Fairy Vines", "Starry", "Lavender", "Cozy Loft", "Kawaii", "Mint Bath", "Sage Living"};
const char* FLOOR_PATTERNS[NUM_FLOORPATS] = {"Gloss", "Planks", "Checker", "Shag", "Honey Wood", "Fluffy", "Pink Tiles"};
const char* HOLIDAY_NAMES[4] = {"None", "Birthday", "Halloween", "Holiday"};

static const float FLOOR_Y = 158.f;

// ---------------- backgrounds ----------------
static void damask(float x, float y, u32 c) {
    quad(x, y - 6, x + 4, y, x, y + 6, x - 4, y, c);
    swirl(x - 5, y + 2, 4, 1.1f, 1.f, c, true);
    swirl(x + 5, y + 2, 4, 1.1f, 1.f, c);
    circle(x, y - 9, 1.2f, c);
}

// ---- Pocket City style: a 2:1 dimetric dollhouse cutaway - two walls meeting in a back
// corner, honey-wood planks, crisp 1px dark edges, light from the top-left.
static const float ISO_CX = 200, ISO_CY = 96;   // the back corner where the walls meet the floor
static float isoEdgeY(float x) { return ISO_CY + std::fabs(x - ISO_CX) * 0.5f; }   // wall/floor seam
static void isoPara(float x0, float y0, float w, float h, float slope, u32 c) {   // a wall-aligned parallelogram
    quad(x0, y0, x0 + w, y0 + w * slope, x0 + w, y0 + w * slope + h, x0, y0 + h, c);
}
static void flipWall(const RoomState& rs) {
    const u32 INK = C(34, 24, 40);
    // left wall: exposed red brick (or the chosen paint), lit from the top-left
    RGB lb = RGB{196, 82, 58};   // exposed red brick on the left, the chosen paint colour on the right
    RGB rb = rs.wall == 0 ? RGB{255, 214, 226} : shade(WALL_COLORS[rs.wall], 0.94f);
    quad(0, 0, ISO_CX, 0, ISO_CX, ISO_CY, 0, ISO_CY + ISO_CX * 0.5f, C(lb));
    quad(ISO_CX, 0, TOP_W, 0, TOP_W, ISO_CY + ISO_CX * 0.5f, ISO_CX, ISO_CY, C(rb));
    {   // bricks follow the 2:1 slope of the left wall
        const u32 MOR = C(shade(lb, 0.66f)), HI = C(shade(lb, 1.16f)), DK = C(shade(lb, 0.86f));
        for (int r = 0; r < 26; r++) {
            float yb = r * 8.f;
            for (int k = 0; k < 13; k++) {
                float x = k * 16.f + (r % 2) * 8.f;
                float y = yb - (ISO_CX - x) * 0 + (ISO_CX - x) * 0.5f - 100 + 0;
                if (x > ISO_CX - 2 || y > isoEdgeY(x) - 6) continue;
                isoPara(x, y, 14, 6, -0.5f, ((k * 5 + r * 3) % 7 == 0) ? DK : C(lb));
                isoPara(x, y, 14, 1, -0.5f, HI);
            }
            (void)MOR;
        }
    }
    // pastel paint stripe band on the right wall
    for (int k = 0; k < 10; k++) { float x = ISO_CX + 8 + k * 20.f; isoPara(x, 0 + (x - ISO_CX) * 0.5f, 8, ISO_CY - 6, 0.5f, C(255, 255, 255, 50)); }
    // the window with blue blinds, on the left wall
    {
        float wx = 52, wy = 30 + (ISO_CX - wx) * 0.5f - 100 + 70;
        isoPara(wx - 2, wy - 2, 64, 54, -0.5f, INK);
        isoPara(wx, wy, 60, 50, -0.5f, C(150, 210, 250));
        for (int i = 0; i < 8; i++) isoPara(wx, wy + i * 6.f, 60, 4, -0.5f, C(i % 2 ? RGB{70, 120, 210} : RGB{96, 150, 230}));
        isoPara(wx, wy, 60, 2, -0.5f, C(255, 255, 255, 160));
    }
    // corner and top cut edges (dollhouse cutaway)
    line(ISO_CX, 0, ISO_CX, ISO_CY, 1.5f, INK);
    rect(0, 0, TOP_W, 3, INK);
}
static void flipFloor(const RoomState& rs) {
    RGB f = rs.floor == 0 ? RGB{226, 160, 82} : FLOOR_COLORS[rs.floor];
    const u32 INK = C(34, 24, 40);
    // floor fills everything below the V-shaped seam
    quad(0, isoEdgeY(0), ISO_CX, ISO_CY, ISO_CX, TOP_H, 0, TOP_H, C(f));
    quad(ISO_CX, ISO_CY, TOP_W, isoEdgeY(TOP_W), TOP_W, TOP_H, ISO_CX, TOP_H, C(f));
    // planks run along the 2:1 diagonal, 2-tone boards with dark seams
    for (int i = 0; i < 48; i++) {
        float x0 = ISO_CX + i * 14.f;   // board edge where it meets the right wall line
        float y0 = isoEdgeY(x0 < ISO_CX ? ISO_CX : x0);
        float len = 400;
        float x1 = x0 - len, y1 = y0 + len * 0.5f;
        if (i % 2) line(x0, y0, x1, y1, 7, C(shade(f, 1.08f)));
        line(x0, y0, x1, y1, 1, C(shade(f, 0.7f)));
    }
    // re-draw the walls' lower edge over plank overrun
    // baseboards along both seams, lit from the left
    quad(0, isoEdgeY(0) - 5, ISO_CX, ISO_CY - 5, ISO_CX, ISO_CY, 0, isoEdgeY(0), C(255, 236, 214));
    quad(ISO_CX, ISO_CY - 5, TOP_W, isoEdgeY(TOP_W) - 5, TOP_W, isoEdgeY(TOP_W), ISO_CX, ISO_CY, C(214, 190, 170));
    line(0, isoEdgeY(0), ISO_CX, ISO_CY, 1.5f, INK); line(ISO_CX, ISO_CY, TOP_W, isoEdgeY(TOP_W), 1.5f, INK);
    // soft warm lamp glow pools
    ellipse(110, 200, 120, 40, C(255, 220, 140, 40));
}

// ---------------- cute room shells: soft one-point perspective, a big balcony door,
// curtains, a ceiling light, crown moulding (and vines / star lights / moon decals)
static bool shellPat(int p) { return p >= 4; }
static const float SW_L = 48, SW_R = 352, SH_TOP = 22, SH_DROP = 34;   // back wall box + side-wall slant
static void shellCurtain(float x0, float x1, float top, float bot, RGB c, bool left, float t) {
    int folds = 4; float w = (x1 - x0) / folds;
    for (int i = 0; i < folds; i++) {
        float a = x0 + i * w, sway = std::sin(t * 0.8f + i) * 0.8f;
        // gathered toward the tie-back
        float tie = top + (bot - top) * 0.62f, pinch = left ? (x1 - x0) * 0.55f : -(x1 - x0) * 0.55f;
        quad(a, top, a + w, top, a + w + (left ? -pinch * (i + 1) / folds * 0.6f : -pinch * (folds - i) / folds * 0.6f) + sway, tie, a + (left ? -pinch * i / folds * 0.6f : -pinch * (folds - i + 1) / folds * 0.6f) + sway, tie, C(shade(c, i % 2 ? 0.9f : 1.04f)));
        quad(a + (left ? -pinch * i / folds * 0.6f : -pinch * (folds - i + 1) / folds * 0.6f) + sway, tie, a + w + (left ? -pinch * (i + 1) / folds * 0.6f : -pinch * (folds - i) / folds * 0.6f) + sway, tie, a + w + sway * 2, bot, a + sway * 2, bot, C(shade(c, i % 2 ? 0.88f : 1.02f)));
    }
    float tx = left ? x1 - (x1 - x0) * 0.42f : x0 + (x1 - x0) * 0.42f;
    rrect(tx - 8, top + (bot - top) * 0.6f, 16, 5, 2, C(shade(c, 0.75f)));
}
static void shellView(int pat, int house, float x, float y, float w, float h, float t, bool night) {
    bool dark = night || pat == 7;
    grad(x, y, w, h, dark ? C(36, 30, 80) : C(140, 200, 245), dark ? C(110, 70, 140) : C(236, 244, 255));
    if (dark) { for (int i = 0; i < 18; i++) sparkle(x + std::fmod(i * 37.f, w), y + std::fmod(i * 23.f, h * 0.6f), 1.2f + (i % 3) * 0.5f, C(255, 250, 230, 200)); circle(x + w * 0.72f, y + 16, 7, C(255, 246, 220)); circle(x + w * 0.72f + 3, y + 14, 6, C(36, 30, 80)); }
    else { for (int i = 0; i < 3; i++) { float cx = x + std::fmod(20 + i * 40 + t * 2, w + 30) - 15, cy = y + 14 + i * 9; ellipse(cx, cy, 22, 7, C(255, 255, 255, 220)); ellipse(cx + 8, cy - 3, 14, 6, C(255, 255, 255, 220)); } }
    if (pat == 6) {   // sea view
        rect(x, y + h * 0.58f, w, h * 0.42f, C(dark ? RGB{40, 60, 110} : RGB{120, 190, 230}));
        for (int i = 0; i < 6; i++) rect(x + 6 + i * 16, y + h * 0.66f + (i % 2) * 6, 9, 1, C(255, 255, 255, 140));
    } else {          // the city
        float base = y + h * 0.78f;
        for (int i = 0; i < 9; i++) { float bh = 14 + (i * 29 % 30), bx = x + i * (w / 9); rect(bx, base - bh, w / 9 - 2, bh + h * 0.22f, C(dark ? RGB{60, 50, 90} : RGB{190, 196, 214})); for (int k = 0; k < 3; k++) if ((i + k) % 2) rect(bx + 3, base - bh + 4 + k * 6, 2, 2, C(255, 230, 150, dark ? 230 : 120)); }
        if (house == 0 || pat == 5) for (int i = 0; i < 5; i++) ellipse(x + 8 + i * (w / 4), base + 6, 18, 9, C(dark ? RGB{40, 70, 60} : RGB{150, 200, 140}));
    }
    // balcony railing
    rect(x, y + h - 20, w, 2, C(240, 240, 246)); for (int i = 0; i <= 12; i++) rect(x + i * (w / 12), y + h - 20, 1.5f, 20, C(240, 240, 246));
    drawWeatherInWindow(x, y, w, h - 20, t);
}

// the shells with their own windows: cozy loft, mint bath, sage living
static void drawShellExtra(const RoomState& rs, int house, float t, bool night) {
    (void)house;
    const int pat = rs.wallPat;
    RGB w = WALL_COLORS[rs.wall];
    if (pat == 8) {   // cozy loft: a tall lace-curtained window, a poster collage and bag hooks
        const float wx = 268, wy = 30, ww = 50, wh = 110;
        rect(wx - 3, wy - 3, ww + 6, wh + 6, C(250, 246, 240));
        grad(wx, wy, ww, wh, night ? C(40, 34, 80) : C(200, 222, 245), night ? C(90, 60, 110) : C(255, 236, 220));
        rect(wx + ww / 2 - 1, wy, 2, wh, C(250, 246, 240)); rect(wx, wy + 36, ww, 2, C(250, 246, 240)); rect(wx, wy + 72, ww, 2, C(250, 246, 240));
        drawWeatherInWindow(wx, wy, ww, wh, t);
        for (int sd = 0; sd < 2; sd++) {   // floral lace curtains
            float x0 = sd ? wx + ww - 6 : wx - 16, x1 = sd ? wx + ww + 16 : wx + 6;
            rect(x0, wy - 6, x1 - x0, wh + 14, C(246, 226, 220, 230));
            for (int i = 0; i < 16; i++) circle(x0 + 3 + (i * 7 % (int)(x1 - x0 - 4)), wy + (i * 13 % (int)wh), 1.4f, C(236, 160, 170));
        }
        line(wx - 20, wy - 6, wx + ww + 20, wy - 6, 2, C(200, 180, 150));
        // poster collage (original art: a film strip, a moon, a heart, polaroids)
        rect(64, 44, 34, 44, C(70, 60, 80)); for (int i = 0; i < 4; i++) { rect(68, 48 + i * 10, 26, 7, C(230, 190, 150)); }
        rect(104, 40, 30, 40, C(250, 240, 230)); circle(119, 56, 8, C(240, 170, 190)); rect(108, 70, 22, 3, C(80, 70, 90));
        rect(140, 48, 24, 30, C(150, 120, 160)); circle(152, 60, 6, C(255, 246, 220)); circle(155, 58, 5, C(150, 120, 160));
        for (int k = 0; k < 3; k++) { float px = 70 + k * 30.f, py = 96; rect(px, py, 20, 22, C(252, 252, 250)); rect(px + 2, py + 2, 16, 14, C(k == 0 ? RGB{250, 200, 180} : k == 1 ? RGB{180, 210, 240} : RGB{240, 190, 214})); }
        line(180, 40, 230, 40, 3, C(214, 196, 170)); for (int k = 0; k < 3; k++) { float hx = 188 + k * 17.f; line(hx, 40, hx, 46, 1.5f, C(190, 170, 140)); rrect(hx - 7, 46, 14, 12, 3, C(k == 0 ? RGB{200, 60, 70} : k == 1 ? RGB{120, 80, 60} : RGB{40, 36, 40})); line(hx - 4, 46, hx, 42, 1, C(80, 60, 50)); line(hx + 4, 46, hx, 42, 1, C(80, 60, 50)); }
        ellipse(110, 150, 120, 30, C(255, 220, 160, 40));   // warm lamp glow
    } else if (pat == 10) {   // mint bath: tiled lower wall, a wooden window, a shelf niche with bottles
        const float ty = 88;
        rect(SW_L, ty, SW_R - SW_L, FLOOR_Y - ty, C(176, 226, 206));
        for (int r = 0; r < 10; r++) rect(SW_L, ty + r * 7.f, SW_R - SW_L, 1, C(255, 255, 255, 200));
        for (int k = 0; k < 44; k++) rect(SW_L + k * 7.f, ty, 1, FLOOR_Y - ty, C(255, 255, 255, 200));
        quad(0, ty - 20, SW_L, ty, SW_L, FLOOR_Y, 0, FLOOR_Y + SH_DROP, C(160, 210, 190));
        quad(SW_R, ty, TOP_W, ty - 20, TOP_W, FLOOR_Y + SH_DROP, SW_R, FLOOR_Y, C(160, 210, 190));
        rect(70, 34, 50, 44, C(180, 130, 86)); grad(74, 38, 42, 36, night ? C(40, 40, 80) : C(210, 236, 250), night ? C(80, 60, 110) : C(255, 255, 255));
        rect(94, 38, 2, 36, C(180, 130, 86)); rect(74, 55, 42, 2, C(180, 130, 86));
        for (int i = 0; i < 6; i++) ellipse(140 + i * 7.f, 40 + (i % 2) * 6, 7, 4, C(120, 180, 130));   // eucalyptus
        rect(230, 70, 100, 18, C(190, 140, 96)); rect(232, 72, 96, 14, C(150, 106, 70));
        const RGB BT[5] = {{120, 70, 50}, {240, 236, 226}, {130, 80, 60}, {250, 200, 210}, {110, 60, 40}};
        for (int i = 0; i < 5; i++) { float bx = 240 + i * 17.f; rect(bx, 74, 8, 12, C(BT[i])); rect(bx + 2, 71, 4, 3, C(40, 40, 44)); }
        rect(150, 46, 30, 34, C(250, 246, 236)); rect(153, 49, 24, 28, C(240, 232, 246)); circle(165, 62, 7, C(250, 200, 214));   // a little art print
    } else {   // 11 sage living: a gallery wall, a red curtain and a drum pendant lamp
        line(200, 0, 200, 16, 1, C(140, 130, 110)); quad(180, 16, 220, 16, 224, 30, 176, 30, C(230, 200, 120)); ellipse(200, 31, 40, 6, C(255, 240, 190, 120));
        const float fx[5] = {120, 160, 196, 236, 280}, fy[5] = {60, 44, 64, 48, 62}, fw[5] = {30, 26, 34, 28, 24}, fh[5] = {24, 34, 28, 36, 22};
        const RGB FC[5] = {{140, 190, 120}, {240, 150, 150}, {200, 170, 110}, {150, 180, 220}, {240, 200, 120}};
        for (int i = 0; i < 5; i++) { rect(fx[i], fy[i], fw[i], fh[i], C(190, 150, 90)); rect(fx[i] + 3, fy[i] + 3, fw[i] - 6, fh[i] - 6, C(250, 246, 236)); circle(fx[i] + fw[i] / 2, fy[i] + fh[i] / 2, std::min(fw[i], fh[i]) / 4.f, C(FC[i])); }
        rect(SW_R - 28, SH_TOP + 6, 26, FLOOR_Y - SH_TOP - 8, C(250, 250, 250)); grad(SW_R - 24, SH_TOP + 10, 18, FLOOR_Y - SH_TOP - 20, night ? C(40, 40, 80) : C(220, 236, 250), C(250, 250, 255));
        for (int i = 0; i < 4; i++) rect(SW_R - 8 + i * 5.f, SH_TOP + 4, 5, FLOOR_Y - SH_TOP - 4, C(i % 2 ? RGB{210, 70, 80} : RGB{190, 56, 66}));
    }
    (void)w;
}

static void drawShell(const RoomState& rs, int house, float t, bool night) {
    const int pat = rs.wallPat;
    RGB w = WALL_COLORS[rs.wall];
    RGB side = shade(w, 0.9f), ceil = shade(w, 1.1f), back = w;
    // back wall + side walls + ceiling
    grad(SW_L, SH_TOP, SW_R - SW_L, FLOOR_Y - SH_TOP, C(shade(back, 1.04f)), C(shade(back, 0.96f)));
    quad(0, 0, SW_L, SH_TOP, SW_L, FLOOR_Y, 0, FLOOR_Y + SH_DROP, C(side));
    quad(SW_R, SH_TOP, TOP_W, 0, TOP_W, FLOOR_Y + SH_DROP, SW_R, FLOOR_Y, C(shade(side, 0.96f)));
    quad(0, 0, TOP_W, 0, SW_R, SH_TOP, SW_L, SH_TOP, C(ceil));
    ellipse(200, 6, 60, 8, C(255, 255, 255, 230)); ellipse(200, 6, 90, 14, C(255, 255, 240, 50));   // ceiling light
    // crown moulding
    line(SW_L, SH_TOP, SW_R, SH_TOP, 3, C(255, 255, 255)); line(0, 0, SW_L, SH_TOP, 3, C(255, 255, 255)); line(SW_R, SH_TOP, TOP_W, 0, 3, C(255, 255, 255));
    line(SW_L, SH_TOP + 2, SW_R, SH_TOP + 2, 1, C(shade(w, 0.75f)));
    line(SW_L, SH_TOP, SW_L, FLOOR_Y, 1.5f, C(shade(w, 0.78f))); line(SW_R, SH_TOP, SW_R, FLOOR_Y, 1.5f, C(shade(w, 0.78f)));
    if (pat >= 8 && pat != 9) { drawShellExtra(rs, house, t, night); return; }
    // the balcony door
    const float wx = 150, wy = 40, ww = 100, wh = FLOOR_Y - 2 - wy;
    rect(wx - 4, wy - 4, ww + 8, wh + 6, C(250, 248, 244));
    shellView(pat, house, wx, wy, ww, wh, t, night);
    rect(wx + ww / 2 - 1.5f, wy, 3, wh, C(250, 248, 244)); rect(wx, wy + wh * 0.28f, ww, 2, C(250, 248, 244));
    rect(wx + ww / 2 - 6, wy + wh * 0.55f, 2, 8, C(200, 200, 210)); rect(wx + ww / 2 + 4, wy + wh * 0.55f, 2, 8, C(200, 200, 210));
    quad(wx + 6, wy + 4, wx + 16, wy + 4, wx + 4, wy + wh * 0.5f, wx - 4 + 6, wy + wh * 0.5f, C(255, 255, 255, 60));
    // a little wall AC unit above the door (every anime bedroom has one)
    rrect(176, 25, 48, 11, 4, C(236, 242, 248)); rect(180, 32, 40, 1, C(190, 200, 214)); rect(180, 34, 40, 1, C(190, 200, 214));
    // curtains
    RGB cur = pat == 4 || pat == 9 ? RGB{240, 150, 172} : pat == 5 ? RGB{244, 166, 178} : pat == 6 ? RGB{190, 214, 240} : RGB{250, 248, 255};
    line(112, 38, 288, 38, 2.5f, C(220, 200, 170));
    if (pat == 4 || pat == 9) { rect(140, 40, 6, wh, C(252, 236, 170, 200)); rect(254, 40, 6, wh, C(252, 236, 170, 200)); }   // butter sheers
    shellCurtain(114, 152, 38, FLOOR_Y - 2, cur, true, t);
    shellCurtain(248, 286, 38, FLOOR_Y - 2, cur, false, t);
    if (pat == 9) {   // kawaii: butter-yellow trim band + original cute posters + a little bookshelf nook
        rect(SW_L, SH_TOP + 3, SW_R - SW_L, 8, C(252, 236, 170)); rect(SW_L, SH_TOP + 11, SW_R - SW_L, 1, C(220, 196, 130));
        const RGB PC[3] = {{250, 190, 214}, {200, 224, 250}, {255, 236, 170}};
        for (int k = 0; k < 3; k++) { float px = 296 + (k % 2) * 26, py = 52 + k * 30; rect(px, py, 22, 28, C(PC[k])); rect(px + 2, py + 2, 18, 24, C(255, 255, 255, 120)); }
        circle(307, 64, 5, C(255, 255, 255)); circle(304, 58, 2, C(255, 255, 255)); circle(310, 58, 2, C(255, 255, 255)); circle(306, 64, 0.8f, C(60, 40, 50)); circle(309, 64, 0.8f, C(60, 40, 50));   // bunny poster
        sparkle(333, 96, 6, C(255, 226, 120));   // star poster
        rect(301, 122, 12, 8, C(255, 255, 255)); rect(301, 120, 12, 3, C(250, 160, 190)); circle(307, 118, 1.5f, C(220, 40, 60));   // cake poster
        rect(60, 50, 40, 86, C(252, 244, 220)); for (int r = 0; r < 4; r++) { rect(62, 52 + r * 21, 36, 1, C(220, 196, 130)); for (int b2 = 0; b2 < 6; b2++) rect(64 + b2 * 5.5f, 56 + r * 21, 4, 16 - (b2 % 3), C(PC[(b2 + r) % 3])); }
    }
    if (pat == 4 || pat == 9) {   // swag valance
        for (int i = 0; i < 3; i++) { float a = 118 + i * 56.f; quad(a, 38, a + 56, 38, a + 46, 50, a + 10, 50, C(shade(cur, 0.95f))); ellipse(a + 28, 47, 26, 6, C(shade(cur, 1.05f))); }
    }
    if (pat == 7) { for (int i = 0; i < 5; i++) { float a = 116 + i * 34.f; tri(a, 38, a + 34, 38, a + 17, 52, C(i % 2 ? RGB{190, 160, 236} : RGB{230, 214, 250})); } }
    // decor per style
    if (pat == 5) {   // fairy vines along the moulding + framed botanical prints
        for (int i = 0; i < 26; i++) {
            float u = i / 25.f, x = SW_L + u * (SW_R - SW_L), y = SH_TOP + 4 + std::sin(u * 18.f) * 3;
            ellipse(x, y, 7, 4, C(i % 2 ? RGB{110, 170, 90} : RGB{140, 196, 110})); if (i % 3 == 0) { line(x, y, x + 2, y + 14 + (i % 4) * 4, 1, C(110, 160, 90)); ellipse(x + 2, y + 16 + (i % 4) * 4, 5, 3, C(130, 186, 100)); }
            if (i % 2) circle(x + 3, y + 3, 1.3f, C(255, 236, 160, (uint8_t)(150 + 100 * std::fabs(std::sin(t * 2 + i)))));
        }
        for (int s2 = 0; s2 < 2; s2++) for (int i = 0; i < 9; i++) { float x = s2 ? SW_R + (TOP_W - SW_R) * i / 9.f : SW_L * i / 9.f, y = s2 ? SH_TOP - (SH_TOP * i / 9.f) + 4 : (SH_TOP * i / 9.f) + 4; ellipse(x, y, 6, 4, C(130, 186, 100)); }
        for (int k = 0; k < 2; k++) { float fx = k ? 300 : 66, fy = 58; rect(fx, fy, 30, 38, C(176, 130, 86)); rect(fx + 3, fy + 3, 24, 32, C(252, 246, 236)); line(fx + 15, fy + 30, fx + 15, fy + 12, 1, C(110, 160, 90)); circle(fx + 15, fy + 12, 4, C(240, 150, 176)); circle(fx + 11, fy + 18, 3, C(246, 180, 196)); }
    }
    if (pat == 6) {   // star string lights in a swag across the top
        for (int sw = 0; sw < 2; sw++) for (int i = 0; i <= 10; i++) {
            float u = i / 10.f, x = (sw ? 200 : SW_L - 20) + u * (sw ? SW_R + 20 - 200 : 200 - SW_L + 20), y = SH_TOP + 6 + std::sin(u * 3.14159f) * 14;
            if (i) line(x - (sw ? SW_R + 20 - 200 : 200 - SW_L + 20) / 10.f, SH_TOP + 6 + std::sin((i - 1) / 10.f * 3.14159f) * 14, x, y, 1, C(90, 80, 90));
            sparkle(x, y + 4, 3.5f, C(255, 226, 120, (uint8_t)(180 + 70 * std::sin(t * 3 + i + sw * 5))));
        }
        rect(64, 70, 26, 30, C(255, 255, 255)); rect(66, 72, 22, 26, C(250, 214, 226)); circle(77, 84, 5, C(240, 150, 170));
    }
    if (pat == 7) {   // moon and star decals + a purple garland
        circle(318, 66, 12, C(255, 250, 220)); circle(323, 62, 11, C(shade(side, 0.96f)));
        for (int i = 0; i < 7; i++) sparkle(300 + (i * 23 % 44), 90 + (i * 31 % 50), 3 + i % 2, C(255, 250, 230));
        for (int i = 0; i < 12; i++) { float x = 60 + i * 24.f, y = SH_TOP + 8 + std::sin(i * 0.9f) * 3; circle(x, y, 3, C(170, 120, 220)); }
    }
}
static void drawShellFloorEdges(const RoomState& rs) {
    RGB w = WALL_COLORS[rs.wall], side = shade(w, 0.9f);
    tri(0, FLOOR_Y - 4, SW_L, FLOOR_Y, 0, FLOOR_Y + SH_DROP, C(side));
    tri(SW_R, FLOOR_Y, TOP_W, FLOOR_Y - 4, TOP_W, FLOOR_Y + SH_DROP, C(shade(side, 0.96f)));
    // baseboards following the room's corners
    line(0, FLOOR_Y + SH_DROP, SW_L, FLOOR_Y, 3, C(255, 255, 255)); line(SW_R, FLOOR_Y, TOP_W, FLOOR_Y + SH_DROP, 3, C(255, 255, 255));
    line(SW_L, FLOOR_Y, SW_R, FLOOR_Y, 3, C(255, 255, 255));
}

static void drawWall(const RoomState& rs, float t) {
    if (artIsFlip(g_artStyle)) { flipWall(rs); return; }
    RGB w = WALL_COLORS[rs.wall];
    grad(0, 0, TOP_W, FLOOR_Y, C(shade(w, 1.08f)), C(shade(w, 0.9f)));
    switch (rs.wallPat) {
    case 1:
        for (int i = 0; i < 20; i++) rect(i * 20.f + 4, 0, 9, FLOOR_Y, C(255, 255, 255, 40));
        break;
    case 2: {
        u32 dc = C(shade(w, rs.wall == 3 || rs.wall == 9 ? 1.5f : 0.82f), 120);
        for (int r = 0; r < 5; r++)
            for (int k = 0; k < 9; k++) damask(k * 48.f + (r % 2) * 24 + 12, r * 34.f + 16, dc);
        break;
    }
    case 3:
        zebraRect(0, 0, TOP_W, FLOOR_Y, C(shade(w, 1.05f)), C(BLACK, 110), 3);
        break;
    default: break;
    }
    // chair rail + crown moulding in gold
    rect(0, 0, TOP_W, 4, C(GOLD)); rect(0, 4, TOP_W, 1, C(shade(GOLD, 0.7f)));
    (void)t;
}

static void drawFloor(const RoomState& rs) {
    if (artIsFlip(g_artStyle)) { flipFloor(rs); return; }
    RGB f = FLOOR_COLORS[rs.floor];
    grad(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(shade(f, 0.85f)), C(shade(f, 1.05f)));
    switch (rs.floorPat) {
    case 0:   // glossy, with soft reflections
        for (int i = 0; i < 5; i++) quad(40 + i * 80.f, FLOOR_Y, 60 + i * 80.f, FLOOR_Y, 40 + i * 80.f, 240, 20 + i * 80.f, 240, C(255, 255, 255, 22));
        break;
    case 1:
        for (int r = 0; r < 6; r++) {
            float y = FLOOR_Y + r * 14;
            rect(0, y, TOP_W, 1, C(shade(f, 0.7f), 160));
            for (int k = 0; k < 6; k++) rect(k * 72.f + (r % 2) * 36, y, 1, 14, C(shade(f, 0.7f), 140));
        }
        break;
    case 2:
        for (int r = 0; r < 5; r++)
            for (int k = 0; k < 20; k++)
                if ((r + k) % 2) rect(k * 20.f, FLOOR_Y + r * 17, 20, 17, C(shade(f, 0.75f)));
        break;
    case 4: {   // honey wood planks in perspective (they converge on the balcony door)
        for (int i = 0; i <= 16; i++) { float u = i / 16.f; float xb = SW_L + u * (SW_R - SW_L), xf = -120 + u * 640; line(xb, FLOOR_Y, xf, TOP_H, 1, C(shade(f, 0.78f), 170)); }
        for (int r = 0; r < 6; r++) { float y = FLOOR_Y + 6 + r * r * 2.6f; for (int k = 0; k < 8; k++) if ((k + r) % 3 == 0) { float u = (k + 0.5f) / 8.f, xl = SW_L + u * (SW_R - SW_L); rect(xl + (xl - 200) * (y - FLOOR_Y) / 82.f, y, 1, 3 + r, C(shade(f, 0.8f), 150)); } }
        for (int i = 0; i < 4; i++) quad(150 + i * 26.f, FLOOR_Y, 160 + i * 26.f, FLOOR_Y, 150 + i * 60.f, TOP_H, 120 + i * 60.f, TOP_H, C(255, 255, 255, 18));   // light from the balcony
        break;
    }
    case 6:   // little square tiles with white grout
        for (int r = 0; r < 12; r++) { float y = FLOOR_Y + r * 7.f; rect(0, y, TOP_W, 1, C(255, 255, 255, 170)); }
        for (int k = 0; k < 48; k++) { float xb = k * 9.f; line(200 + (xb - 200) * 0.82f, FLOOR_Y, 200 + (xb - 200) * 1.25f, TOP_H, 1, C(255, 255, 255, 170)); }
        break;
    case 5:   // fluffy wall-to-wall carpet
        for (int i = 0; i < 260; i++) { float x = std::fmod(i * 47.f, 400.f), y = FLOOR_Y + std::fmod(i * 29.f, 82.f); line(x, y, x + 1.5f, y - 2.5f, 1, C(shade(f, i % 2 ? 1.12f : 0.9f), 170)); }
        break;
    case 3:
        for (int r = 0; r < 14; r++)
            for (int k = 0; k < 40; k++) circle(k * 10.f + (r % 2) * 5, FLOOR_Y + 3 + r * 6, 1.4f, C(shade(f, (k + r) % 3 ? 1.15f : 0.85f), 160));
        break;
    }
    if (shellPat(rs.wallPat)) { drawShellFloorEdges(rs); return; }
    rect(0, FLOOR_Y - 4, TOP_W, 5, C(CREAM)); rect(0, FLOOR_Y, TOP_W, 1, C(shade(CREAM, 0.7f)));
}

static void drawWindow(int house, float x, float y, float t, bool night) {
    rrect(x - 4, y - 4, 78, 70, 4, C(CREAM));
    if (house == 0) {   // garden view
        grad(x, y, 70, 62, night ? C(30, 30, 70) : C(150, 205, 245), night ? C(70, 50, 110) : C(255, 220, 230));
        ellipse(x + 18, y + 60, 36, 14, C(night ? RGB{40, 70, 50} : RGB{120, 190, 110}));
        ellipse(x + 50, y + 61, 40, 12, C(night ? RGB{30, 60, 44} : RGB{100, 170, 96}));
        rect(x, y + 58, 70, 4, C(night ? RGB{40, 70, 50} : RGB{120, 190, 110}));
        for (int i = 0; i < 4; i++) circle(x + 10 + i * 16, y + 50 - (i % 2) * 3, 2.4f, C(HOT));
    } else {            // penthouse skyline
        grad(x, y, 70, 62, night ? C(24, 18, 50) : C(250, 190, 210), night ? C(80, 40, 90) : C(255, 230, 200));
        static const float bh[] = {30, 44, 22, 50, 34, 26, 40};
        for (int i = 0; i < 7; i++) {
            float bx = x + i * 10, h = bh[i];
            rect(bx, y + 62 - h, 9, h, C(night ? RGB{40, 30, 60} : RGB{150, 110, 140}));
            for (int k = 0; k < (int)(h / 7); k++) if ((i + k) % 2) rect(bx + 3, y + 62 - h + 3 + k * 7, 3, 3, C(255, 220, 140, night ? 230 : 90));
        }
    }
    if (night) { circle(x + 54, y + 14, 6, C(255, 246, 220)); circle(x + 57, y + 12, 5, C(30, 30, 70)); }
    else circle(x + 54, y + 14, 6, C(255, 236, 160));
    for (int i = 0; i < 3; i++) {
        float a = std::sin(t * 2 + i * 2) * 0.5f + 0.5f;
        sparkle(x + 10 + i * 22, y + 10 + i * 6, 1.5f + a * 2, C(255, 255, 255, (uint8_t)(night ? 200 * a : 60 * a)));
    }
    drawWeatherInWindow(x, y, 70, 62, t);
    rect(x + 34, y, 2, 62, C(CREAM)); rect(x, y + 30, 70, 2, C(CREAM));
    rect(x - 6, y + 62, 82, 4, C(CREAM));
}

static void drawOuting(int house, int room, float t, bool night) {
    if (room == R_YARD) {
        grad(0, 0, TOP_W, FLOOR_Y, night ? C(30, 24, 70) : C(150, 205, 245), night ? C(110, 60, 120) : C(255, 214, 230));
        if (house == 1) {   // rooftop terrace
            for (int i = 0; i < 20; i++) {
                float h = 30 + (i * 37 % 60);
                rect(i * 20.f, FLOOR_Y - h, 18, h, C(night ? RGB{50, 36, 70} : RGB{186, 150, 176}));
                for (int k = 0; k < (int)(h / 10); k++) if ((i * 3 + k) % 3 == 0) rect(i * 20.f + 6, FLOOR_Y - h + 4 + k * 10, 4, 4, C(255, 220, 140, night ? 230 : 80));
            }
            rect(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(170, 120, 92));
            for (int k = 0; k < 20; k++) rect(k * 20.f, FLOOR_Y, 1, 82, C(130, 90, 70));
            ellipse(300, 205, 140, 40, C(GOLD)); ellipse(300, 205, 130, 34, C(120, 200, 240));   // hot tub
            for (int i = 0; i < 6; i++) circle(260 + i * 15 + std::sin(t * 3 + i) * 3, 200 + (i % 2) * 6, 2.5f, C(255, 255, 255, 200));
            for (int i = 0; i < 20; i++) rect(i * 20.f, FLOOR_Y - 22, 2, 22, C(GOLD));
            rect(0, FLOOR_Y - 24, TOP_W, 3, C(GOLD));
        } else {            // cottage garden with pool
            if (night || (G.weather != WX_RAIN && G.weather != WX_STORM && G.weather != WX_CLOUDY && G.weather != WX_SNOW)) circle(340, 40, 18, C(night ? RGB{255, 246, 220} : RGB{255, 230, 140}));
            ellipse(80, 40, 50, 16, C(255, 255, 255, 200)); ellipse(100, 34, 40, 16, C(255, 255, 255, 200));
            for (int i = 0; i < 26; i++) { rect(i * 16.f, FLOOR_Y - 40, 10, 40, C(CREAM)); tri(i * 16.f, FLOOR_Y - 40, i * 16.f + 10, FLOOR_Y - 40, i * 16.f + 5, FLOOR_Y - 46, C(CREAM)); }
            rect(0, FLOOR_Y - 30, TOP_W, 4, C(CREAM));
            grad(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(110, 180, 100), C(140, 200, 110));
            ellipse(310, 200, 150, 44, C(CREAM)); ellipse(310, 200, 138, 36, C(110, 200, 240));
            ellipse(300, 196, 60, 8, C(255, 255, 255, 90));
            ellipse(320 + std::sin(t) * 10, 202, 26, 12, C(HOT)); ellipse(320 + std::sin(t) * 10, 202, 14, 5, C(110, 200, 240));   // pool float
        }
    } else if (room == R_BEACH) {
        const bool grey = G.weather == WX_RAIN || G.weather == WX_STORM || G.weather == WX_CLOUDY || G.weather == WX_SNOW;
        if (grey) grad(0, 0, TOP_W, 110, C(176, 186, 206), C(214, 214, 226));
        else grad(0, 0, TOP_W, 110, C(255, 190, 200), C(255, 230, 190));
        if (!grey) circle(300, 70, 26, C(G.weather == WX_HEAT ? RGB{255, 170, 90} : RGB{255, 200, 120}));
        grad(0, 100, TOP_W, 60, C(80, 170, 220), C(140, 210, 230));
        for (int i = 0; i < 10; i++) ellipse(i * 44.f + std::fmod(t * 10, 44.f), 104 + (i % 2) * 20, 30, 4, C(255, 255, 255, 120));
        grad(0, FLOOR_Y - 10, TOP_W, TOP_H - FLOOR_Y + 10, C(250, 226, 180), C(236, 204, 150));
        // palm tree
        quad(40, 230, 50, 230, 64, 80, 58, 80, C(150, 100, 60));
        for (int i = 0; i < 5; i++) { float a = -2.6f + i * 0.55f; line(61, 80, 61 + std::cos(a) * 46, 80 + std::sin(a) * 22 + 20, 7, C(GREEN)); }
        for (int i = 0; i < 4; i++) sparkle(120 + i * 70, 130 + (i % 2) * 14, 2 + std::sin(t * 4 + i) * 1.5f, C(255, 255, 255, 220));
    } else if (room == R_SCHOOL) {
        grad(0, 0, TOP_W, FLOOR_Y, C(240, 226, 230), C(222, 204, 212));
        for (int i = 0; i < 9; i++) {
            float x = 160 + i * 26;
            rect(x, 30, 24, 128, C(i % 3 == 0 ? HOT : PINK)); rect(x + 2, 34, 20, 58, C(255, 255, 255, 40));
            for (int k = 0; k < 3; k++) rect(x + 6, 40 + k * 5, 12, 2, C(shade(PINK, 0.7f)));
            rect(x + 18, 96, 2, 8, C(LIGHTGRAY));
            if (i == 2) heart(x + 12, 72, 4, C(WHITE));
        }
        rect(12, 30, 130, 80, C(MOCHA)); rect(16, 34, 122, 72, C(70, 110, 80));
        text("Spirit Week!", 77, 44, 0.55f, C(255, 255, 255, 220), 1);
        swirl(40, 86, 10, 1.4f, 1.2f, C(255, 255, 255, 160)); heart(110, 86, 6, C(BLUSH));
        rect(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(232, 226, 228));
        for (int r = 0; r < 5; r++) for (int k = 0; k < 20; k++) if ((r + k) % 2) rect(k * 20.f, FLOOR_Y + r * 17, 20, 17, C(250, 190, 214));
    } else if (room == R_MALL) {
        grad(0, 0, TOP_W, FLOOR_Y, C(255, 240, 246), C(250, 222, 232));
        rect(0, 0, TOP_W, 26, C(255, 255, 255)); for (int i = 0; i < 8; i++) rect(i * 52.f + 10, 4, 40, 18, C(200, 230, 250));   // skylights
        static const RGB shopC[4] = {{255, 58, 140}, {224, 184, 104}, {120, 190, 240}, {200, 166, 230}};
        static const char* shopN[4] = {"GLITZ", "SHOEZ", "BLEND", "BEAUTY"};
        for (int i = 0; i < 4; i++) {
            float sx = 8 + i * 98;
            rect(sx, 40, 90, 118, C(250, 250, 252)); rect(sx, 40, 90, 18, C(shopC[i])); text(shopN[i], sx + 45, 42, 0.45f, C(WHITE), 1);
            rect(sx + 8, 64, 74, 70, C(210, 235, 250)); rect(sx + 8, 64, 74, 2, C(255, 255, 255));
            for (int k = 0; k < 3; k++) rect(sx + 14 + k * 22, 100, 14, 30, C(shopC[(i + k + 1) % 4]));
            sparkle(sx + 70, 72, 3 + std::fabs(std::sin(t * 2 + i)) * 3, C(255, 255, 255, 200));
        }
        rect(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(246, 240, 236));
        for (int r = 0; r < 5; r++) for (int k = 0; k < 10; k++) rect(k * 40.f, FLOOR_Y + r * 17, 1, 17, C(220, 210, 205));
        for (int r = 0; r < 5; r++) rect(0, FLOOR_Y + r * 17, TOP_W, 1, C(220, 210, 205));
        ellipse(200, 205, 70, 18, C(170, 220, 245)); ellipse(200, 203, 40, 8, C(255, 255, 255, 120));   // fountain
        for (int i = 0; i < 4; i++) circle(200 + std::sin(t * 3 + i) * 12, 190 - std::fabs(std::sin(t * 4 + i)) * 14, 2, C(200, 235, 255));
    } else if (room == R_PARK) {
        { const bool grey = !night && (G.weather == WX_RAIN || G.weather == WX_STORM || G.weather == WX_CLOUDY || G.weather == WX_SNOW);
          grad(0, 0, TOP_W, FLOOR_Y, night ? C(30, 24, 70) : grey ? C(170, 182, 204) : C(150, 210, 245), night ? C(110, 60, 120) : grey ? C(214, 218, 230) : C(220, 240, 250)); }
        ellipse(90, 36, 60, 18, C(255, 255, 255, 210)); ellipse(300, 28, 70, 20, C(255, 255, 255, 210));
        for (int i = 0; i < 6; i++) {   // trees
            float tx = 20 + i * 72;
            rect(tx + 14, 90, 8, 68, C(130, 90, 60));
            circle(tx + 18, 80, 26, C(100, 170, 90)); circle(tx + 6, 92, 18, C(90, 160, 80)); circle(tx + 30, 92, 18, C(110, 180, 96));
            if (i % 2) for (int k = 0; k < 4; k++) circle(tx + 6 + k * 8, 76 + (k % 2) * 10, 2.5f, C(255, 170, 200));   // blossoms
        }
        grad(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(120, 190, 100), C(140, 205, 110));
        quad(170, FLOOR_Y, 230, FLOOR_Y, 300, 240, 100, 240, C(230, 214, 180));   // path
        ellipse(330, 205, 90, 26, C(110, 190, 230)); ellipse(330, 203, 60, 12, C(255, 255, 255, 80));   // pond
        for (int i = 0; i < 2; i++) { float dx = 310 + i * 26 + std::sin(t + i) * 6; ellipse(dx, 202, 10, 6, C(255, 255, 255)); circle(dx + 5, 198, 3, C(255, 255, 255)); tri(dx + 8, 198, dx + 11, 199, dx + 8, 200, C(ORANGE)); }
        for (int i = 0; i < 10; i++) circle(20 + i * 17, 225 - (i % 3) * 6, 2.2f, C(i % 2 ? HOT : RGB{255, 240, 120}));
    } else if (room == R_CINEMA) {
        grad(0, 0, TOP_W, FLOOR_Y, C(60, 14, 30), C(30, 8, 18));
        rrect(80, 14, 240, 104, 6, C(GOLD)); rect(86, 20, 228, 92, C(night ? RGB{40, 40, 60} : RGB{50, 50, 70}));
        float flick = 0.5f + 0.5f * std::sin(t * 3);
        rect(86, 20, 228, 92, C(200, 160, 220, (uint8_t)(60 + flick * 50)));
        heart(200, 60, 18, C(HOT, 200)); text("NOW SHOWING", 200, 86, 0.5f, C(WHITE), 1);
        for (int i = 0; i < 2; i++) { float cx = i ? 330 : 10; for (int k = 0; k < 6; k++) quad(cx + k * 10, 0, cx + k * 10 + 8, 0, cx + k * 10 + 6, FLOOR_Y, cx + k * 10 - 2, FLOOR_Y, C(170, 20, 50)); }
        for (int i = 0; i < 12; i++) circle(84 + i * 21, 124, 2.5f, C(255, 230, 140, (uint8_t)(150 + 100 * std::sin(t * 5 + i))));
        rect(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(120, 20, 40));
        for (int r = 0; r < 3; r++) for (int k = 0; k < 10; k++) rrect(10 + k * 39.f + (r % 2) * 18, FLOOR_Y + 14 + r * 24, 32, 16, 5, C(200, 40, 70));   // seats
        rect(0, FLOOR_Y, TOP_W, 4, C(GOLD));
    } else if (room == R_RINK) {
        grad(0, 0, TOP_W, FLOOR_Y, C(190, 220, 250), C(235, 245, 255));
        for (int i = 0; i < 18; i++) { float sx = std::fmod(i * 53.f + t * 12, 400.f), sy = std::fmod(i * 37.f + t * 20, 150.f); circle(sx, sy, 1.6f, C(255, 255, 255, 220)); }
        for (int i = 0; i < 5; i++) { float mx = 30 + i * 85; tri(mx, 150, mx + 40, 60 + (i % 2) * 20, mx + 80, 150, C(200, 215, 240)); tri(mx + 30, 80 + (i % 2) * 20, mx + 40, 60 + (i % 2) * 20, mx + 50, 80 + (i % 2) * 20, C(255, 255, 255)); }
        rrect(130, 14, 140, 28, 8, C(255, 255, 255, 230)); text("Ice Palace", 200, 18, 0.6f, C(80, 120, 200), 1);
        grad(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(220, 240, 255), C(200, 228, 250));
        for (int i = 0; i < 6; i++) { float a = t * 0.3f + i; ellipse(200 + std::cos(a) * 120, 200 + std::sin(a) * 20, 60, 6, C(255, 255, 255, 120)); }
        rect(0, FLOOR_Y, TOP_W, 6, C(255, 255, 255)); for (int i = 0; i < 20; i++) rect(i * 20.f, FLOOR_Y - 14, 3, 14, C(255, 255, 255));
    } else if (room == R_RESTAURANT) {
        grad(0, 0, TOP_W, FLOOR_Y, C(110, 30, 50), C(70, 20, 34));
        for (int r = 0; r < 4; r++) for (int k = 0; k < 9; k++) damask(k * 48.f + (r % 2) * 24 + 12, r * 38.f + 20, C(GOLD, 50));
        for (int i = 0; i < 3; i++) {   // chandeliers
            float cx = 80 + i * 120;
            line(cx, 0, cx, 18, 1, C(GOLD));
            quad(cx - 16, 18, cx + 16, 18, cx + 10, 26, cx - 10, 26, C(GOLD));
            for (int k = 0; k < 5; k++) { circle(cx - 12 + k * 6, 30, 2, C(255, 240, 200)); circle(cx - 12 + k * 6, 30, 5, C(255, 220, 150, 40)); }
        }
        rrect(150, 50, 100, 26, 6, C(CHOC)); text("Bella Notte", 200, 55, 0.6f, C(GOLD), 1);
        rect(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(40, 24, 22));
        for (int r = 0; r < 5; r++) for (int k = 0; k < 20; k++) if ((r + k) % 2) rect(k * 20.f, FLOOR_Y + r * 17, 20, 17, C(70, 40, 34));
        for (int i = 0; i < 2; i++) {   // set tables
            float tx = 110 + i * 190;
            ellipse(tx, 206, 80, 14, C(CREAM)); rect(tx - 40, 206, 80, 22, C(CREAM)); ellipse(tx, 228, 80, 8, C(240, 230, 220));
            rect(tx - 1, 186, 2, 14, C(CREAM)); ellipse(tx, 186, 4, 7, C(255, 200, 80, (uint8_t)(180 + 70 * std::sin(t * 9 + i))));
            heart(tx + 20, 200, 3, C(HOT));
        }
    } else if (room == R_BOUTIQUE) {
        grad(0, 0, TOP_W, FLOOR_Y, C(255, 220, 232), C(255, 196, 216));
        rrect(130, 12, 140, 28, 8, C(BLACK)); text("Diva Boutique", 200, 17, 0.6f, C(HOT), 1);
        for (int i = 0; i < 2; i++) {   // clothing racks
            float rx = 30 + i * 260;
            rect(rx, 60, 100, 3, C(GOLD)); rect(rx, 60, 3, 98, C(GOLD)); rect(rx + 97, 60, 3, 98, C(GOLD));
            static const RGB cl[] = {HOT, BLACK, CREAM, GOLD, LILAC, BERRY, BLUSH};
            for (int k = 0; k < 7; k++) {
                float hx = rx + 8 + k * 13;
                line(hx, 63, hx, 68, 1, C(GRAY));
                if (k % 3 == 1) zebraRect(hx - 5, 68, 10, 34, C(CREAM), C(BLACK), k);
                else quad(hx - 4, 68, hx + 4, 68, hx + 6, 104, hx - 6, 104, C(cl[(k + i) % 7]));
            }
        }
        for (int i = 0; i < 3; i++) {   // shoe display
            float sx = 165 + i * 25;
            rect(sx - 10, 120, 20, 3, C(GOLD));
            Pen p(sx, 120, 10, 10, 1.6f);
            p.Q(0, 0, 8, 2, 9, 5, 1, 5, C(i == 1 ? BLACK : HOT)); p.L(1, 5, 0, 9, 1.2f, C(i == 1 ? BLACK : HOT));
        }
        sparkle(200, 90, 5 + std::sin(t * 3) * 2, C(GOLD));
        rect(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(250, 244, 240));
        for (int i = 0; i < 5; i++) quad(40 + i * 80.f, FLOOR_Y, 60 + i * 80.f, FLOOR_Y, 40 + i * 80.f, 240, 20 + i * 80.f, 240, C(255, 200, 220, 60));
    } else if (room == R_COLLEGE) {
        grad(0, 0, TOP_W, FLOOR_Y, C(236, 222, 200), C(214, 196, 170));
        for (int i = 0; i < 5; i++) { rect(20 + i * 80.f, 10, 16, 148, C(250, 244, 232)); rect(16 + i * 80.f, 10, 24, 6, C(250, 244, 232)); }
        rect(120, 30, 160, 70, C(70, 50, 44)); rect(124, 34, 152, 62, C(60, 90, 70));
        text("Fashion 101", 200, 40, 0.5f, C(255, 255, 255, 220), 1);
        Pen p(200, 90, 20, 30, 1.2f);
        p.Ci(10, 4, 3, C(255, 255, 255, 200)); p.Q(6, 8, 14, 8, 18, 30, 2, 30, C(255, 255, 255, 140));
        rrect(150, 110, 100, 20, 5, C(BERRY)); text("Starlight College", 200, 113, 0.45f, C(GOLD), 1);
        rect(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(150, 100, 70));
        for (int r = 0; r < 6; r++) rect(0, FLOOR_Y + r * 14, TOP_W, 1, C(110, 70, 50));
    } else if (room == R_CAFE) {
        grad(0, 0, TOP_W, FLOOR_Y, C(246, 226, 200), C(226, 196, 160));
        rect(0, 0, TOP_W, 60, C(120, 80, 60));
        for (int i = 0; i < 10; i++) rect(i * 40.f, 0, 20, 60, C(140, 96, 72));
        rrect(140, 14, 120, 30, 8, C(CREAM)); text("Cafe Latte", 200, 20, 0.6f, C(CHOC), 1);
        rect(220, 100, 170, 58, C(CHOC)); rect(220, 96, 170, 6, C(CREAM));   // counter
        for (int i = 0; i < 4; i++) { rect(240 + i * 36.f, 80, 14, 16, C(CREAM)); rect(240 + i * 36.f, 84, 14, 4, C(HOT)); }
        rrect(30, 70, 90, 70, 6, C(BLACK)); text("MENU", 75, 76, 0.45f, C(CREAM), 1);
        text("Latte  $8", 75, 94, 0.4f, C(CREAM), 1); text("Frappe $9", 75, 108, 0.4f, C(CREAM), 1); text("Muffin $5", 75, 122, 0.4f, C(CREAM), 1);
        for (int i = 0; i < 3; i++) { float k = std::fmod(t * 0.5f + i * 0.33f, 1.f); circle(250 + i * 36 + std::sin(t + i) * 2, 76 - k * 20, 2 + k * 2, C(255, 255, 255, (uint8_t)(140 * (1 - k)))); }
        rect(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(160, 110, 80));
        for (int r = 0; r < 5; r++) for (int k = 0; k < 20; k++) if ((r + k) % 2) rect(k * 20.f, FLOOR_Y + r * 17, 20, 17, C(130, 86, 62));
    } else if (room == R_SPA) {
        grad(0, 0, TOP_W, FLOOR_Y, C(232, 244, 240), C(214, 232, 226));
        for (int i = 0; i < 10; i++) rect(i * 40.f + 10, 0, 2, FLOOR_Y, C(255, 255, 255, 90));
        rrect(120, 14, 160, 28, 8, C(255, 255, 255, 220)); text("Pearl Spa & Salon", 200, 20, 0.5f, C(BERRY), 1);
        for (int i = 0; i < 3; i++) {   // salon mirrors
            float mx = 60 + i * 140;
            ellipse(mx, 92, 50, 70, C(GOLD)); ellipse(mx, 92, 42, 62, C(205, 228, 244));
            for (int k = 0; k < 6; k++) circle(mx + std::cos(k * 1.05f) * 27, 92 + std::sin(k * 1.05f) * 37, 2, C(255, 245, 210));
        }
        for (int i = 0; i < 4; i++) circle(30 + i * 110, 145, 5, C(255, 255, 255, 200));
        rect(0, FLOOR_Y, TOP_W, TOP_H - FLOOR_Y, C(244, 240, 236));
        for (int r = 0; r < 5; r++) for (int k = 0; k < 20; k++) if ((r + k) % 2) rect(k * 20.f, FLOOR_Y + r * 17, 20, 17, C(222, 236, 232));
    } else {   // club
        grad(0, 0, TOP_W, FLOOR_Y, C(40, 18, 50), C(90, 30, 80));
        for (int i = 0; i < 4; i++) {
            float sx = 60 + i * 95 + std::sin(t * 1.5f + i) * 40;
            u32 lc = C(i % 2 ? HOT : GOLD, 60);
            tri(sx, 0, sx - 50, 240, sx + 50, 240, lc);
        }
        rrect(110, 18, 180, 26, 6, C(GOLD)); rrect(112, 20, 176, 22, 5, C(BERRY));
        text("Club Glitz", 200, 23, 0.55f, C(CREAM), 1);
        for (int i = 0; i < 18; i++) {
            float x = i * 24.f + 6, y = 60 + std::sin(i * 1.3f) * 6;
            line(x, 50, x, y, 0.8f, C(GOLD, 160));
            sparkle(x, y, 2 + std::fabs(std::sin(t * 3 + i)) * 2, C(255, 240, 200, 220));
        }
        for (int r = 0; r < 5; r++)
            for (int k = 0; k < 20; k++) {
                float a = std::sin(t * 4 + r * 1.1f + k * 0.7f);
                RGB tc = a > 0.6f ? HOT : a > 0.1f ? LILAC : a > -0.4f ? GOLD : RGB{60, 30, 70};
                rect(k * 20.f + 1, FLOOR_Y + r * 17 + 1, 18, 15, C(tc, 220));
            }
    }
    (void)night;
}

// ---------------- holiday decor ----------------
static void stringLights(float t, int holiday) {
    for (int i = 0; i < 26; i++) {
        float x = i * 16.f + 4, y = 10 + std::sin(i * 0.8f) * 4;
        if (i) line(x - 16, 10 + std::sin((i - 1) * 0.8f) * 4, x, y, 1.f, C(BLACK, 140));
        bool on = ((int)(t * 3) + i) % 3 != 0;
        RGB lc = holiday == HOL_HALLOWEEN ? (i % 2 ? ORANGE : PURPLE)
                : (i % 4 == 0 ? RED : i % 4 == 1 ? GOLD : i % 4 == 2 ? GREEN : HOT);
        circle(x, y + 4, 2.6f, C(lc, on ? 255 : 120));
        if (on) circle(x, y + 4, 5, C(lc, 50));
    }
}

static void wallHolidayDecor(int holiday, float t) {
    if (holiday == HOL_BIRTHDAY) {
        for (int i = 0; i < 20; i++) {
            RGB c = i % 3 == 0 ? HOT : i % 3 == 1 ? GOLD : BLUSH;
            float x = i * 20.f;
            tri(x, 8, x + 20, 8, x + 10, 24 + (i % 2) * 2, C(c));
        }
        line(0, 8, TOP_W, 8, 1.f, C(CHOC));
        text("Happy Birthday!", 200, 28, 0.6f, C(HOT), 1);
        for (int i = 0; i < 3; i++) {
            float bx = 340 + i * 18, by = 60 + std::sin(t * 1.5f + i) * 4;
            line(bx, by + 12, bx + 4 - i * 4, 150, 0.8f, C(LIGHTGRAY));
            ellipse(bx, by, 16, 20, C(i == 1 ? GOLD : HOT));
            ellipse(bx - 3, by - 4, 4, 6, C(255, 255, 255, 120));
        }
    } else if (holiday == HOL_HALLOWEEN) {
        stringLights(t, holiday);
        for (int i = 0; i < 3; i++) {
            float bx = 120 + i * 90 + std::sin(t * 1.3f + i) * 20, by = 40 + std::cos(t * 2 + i) * 8;
            float flap = std::sin(t * 12 + i) * 5;
            tri(bx, by, bx - 12, by - 4 + flap, bx - 4, by + 4, C(BLACK));
            tri(bx, by, bx + 12, by - 4 + flap, bx + 4, by + 4, C(BLACK));
            circle(bx, by, 3, C(BLACK));
        }
        // cobweb in corner
        for (int i = 0; i < 5; i++) line(0, 6, 40 * std::cos(i * 0.39f), 6 + 40 * std::sin(i * 0.39f), 0.6f, C(255, 255, 255, 140));
        for (int k = 1; k < 4; k++)
            for (int i = 0; i < 4; i++) {
                float r = k * 12;
                line(r * std::cos(i * 0.39f), 6 + r * std::sin(i * 0.39f), r * std::cos((i + 1) * 0.39f), 6 + r * std::sin((i + 1) * 0.39f), 0.6f, C(255, 255, 255, 140));
            }
    } else if (holiday == HOL_XMAS) {
        stringLights(t, holiday);
        // wreath
        for (int i = 0; i < 12; i++) circle(200 + std::cos(i * 0.52f) * 14, 50 + std::sin(i * 0.52f) * 14, 5, C(GREEN));
        tri(200, 64, 194, 72, 206, 72, C(RED)); circle(200, 64, 3, C(RED));
        for (int i = 0; i < 4; i++) circle(200 + std::cos(i * 1.6f) * 14, 50 + std::sin(i * 1.6f) * 14, 1.8f, C(GOLD));
    }
}

static void floorHolidayDecor(int holiday, float t) {
    if (holiday == HOL_HALLOWEEN) {
        for (int i = 0; i < 2; i++) {
            float x = 24 + i * 352, y = 220;
            ellipse(x, y, 32, 24, C(ORANGE)); ellipse(x, y, 14, 24, C(shade(ORANGE, 0.85f)));
            rect(x - 1.5f, y - 15, 3, 5, C(GREEN));
            u32 glow = C(255, 220, 90, (uint8_t)(180 + 60 * std::sin(t * 5 + i)));
            tri(x - 8, y - 4, x - 3, y - 4, x - 5.5f, y - 9, glow); tri(x + 3, y - 4, x + 8, y - 4, x + 5.5f, y - 9, glow);
            quad(x - 8, y + 2, x + 8, y + 2, x + 5, y + 7, x - 5, y + 7, glow);
        }
    } else if (holiday == HOL_XMAS) {
        float x = 360, y = 214;
        rect(x - 4, y, 8, 12, C(WOOD));
        for (int i = 0; i < 3; i++) tri(x - 32 + i * 7, y - i * 26, x + 32 - i * 7, y - i * 26, x, y - 40 - i * 26, C(shade(GREEN, 0.8f + i * 0.1f)));
        for (int i = 0; i < 9; i++) {
            float ox = x + std::sin(i * 2.4f) * (22 - (i / 3) * 6), oy = y - 10 - (i / 3) * 26 - (i % 3) * 6;
            circle(ox, oy, 2.6f, C(i % 3 == 0 ? HOT : i % 3 == 1 ? GOLD : CREAM));
        }
        sparkle(x, y - 92, 7 + std::sin(t * 4) * 2, C(GOLD));
        for (int i = 0; i < 3; i++) {
            float gx = 316 + i * 26, gy = 222;
            RGB gc = i == 0 ? HOT : i == 1 ? CREAM : BERRY;
            rect(gx - 9, gy - 14, 18, 14, C(gc)); rect(gx - 1.5f, gy - 14, 3, 14, C(GOLD)); rect(gx - 9, gy - 8, 18, 3, C(GOLD));
            heart(gx, gy - 16, 3, C(GOLD));
        }
    } else if (holiday == HOL_BIRTHDAY) {
        float x = 30, y = 224;
        rect(x - 18, y - 8, 36, 8, C(CREAM)); rect(x - 14, y - 18, 28, 10, C(BLUSH)); rect(x - 10, y - 26, 20, 8, C(HOT));
        for (int i = 0; i < 3; i++) {
            rect(x - 6 + i * 6, y - 34, 2, 8, C(GOLD));
            ellipse(x - 5 + i * 6, y - 37 + std::sin(t * 10 + i), 3, 5, C(255, 200, 80));
        }
        for (int i = 0; i < 3; i++) { float gx = 350 + i * 20; rect(gx - 8, 214, 16, 14, C(i % 2 ? GOLD : HOT)); rect(gx - 1.5f, 214, 3, 14, C(CREAM)); }
    }
}


// ---------------------------------------------------------------- furniture use
// Where a sim's head is (for gems, hearts & speech bubbles), accounting for sitting / lying.
static float actorHeadY(const Game& g, const RoomState& rs, const Actor& a) {
    if (a.kind != 0) return a.y - 40;
    float h = simHeight(g.sims[a.id]);
    if (a.seat >= 0 && a.seat < (int)rs.items.size()) {
        const Placed& pl = rs.items[a.seat];
        UseSpot u = useSpot(itemShape(pl.item));
        if (a.useKind == 1) return pl.y - u.seatY - h * 0.52f;
        if (a.useKind == 2) return pl.y - u.seatY - 16;
        if (a.useKind == 3) return pl.y - u.seatY - h;
    }
    return a.y - h;
}

static void drawActor(const Game& g, const RoomState& rs, const Actor& a, float t, bool party) {
    if (a.kind == 0 && brawlHides(a.id)) return;   // inside the fight's dust cloud
    const Sim& sm = g.sims[a.id];
    const float tt = t + a.id * 0.37f;
    if (a.seat >= 0 && a.seat < (int)rs.items.size() && a.useKind) {
        const Placed& pl = rs.items[a.seat];
        UseSpot u = useSpot(itemShape(pl.item));
        float sx = pl.x + u.slotDX[a.seatSlot % 2];
        if (a.useKind == 1) {
            g_rigOn = true; for (float& r : g_rig) r = 0;
            drawSim(sm, sx, pl.y - u.seatY, 1.f, tt, g.holiday, a.sleeping ? PO_SITSLEEP : PO_SIT, a.facingLeft);
            g_rigOn = false;
        } else if (a.useKind == 2) {   // lying down: rotate the whole figure
            float L = simHeight(sm) * 0.82f;
            float cx = baseShape(itemShape(pl.item)) == SH_BED ? pl.x + 8 : pl.x + 2;
            float cy = pl.y - u.seatY - a.seatSlot * 7.f;
            C3D_Mtx m;
            C2D_ViewSave(&m);
            C2D_ViewTranslate(cx, cy);
            C2D_ViewRotate(-1.5707963f);
            drawSim(sm, 0, L / 2, 0.82f, tt, g.holiday, a.sleeping ? PO_SLEEP : PO_IDLE, false);
            C2D_ViewRestore(&m);
            if (a.sleeping) {
                float zt = std::fmod(t + a.id, 2.f);
                text("z", cx - L / 2 + 6 + zt * 5, cy - 16 - zt * 8, 0.5f, C(255, 255, 255, (uint8_t)(230 - zt * 100)));
            }
        } else {   // bathing / showering: stand in it, the front covers the body
            drawSim(sm, sx, pl.y - u.seatY, 1.f, tt, g.holiday, PO_IDLE, a.facingLeft);
        }
        drawItemFront(pl.item, pl.x, pl.y, 1.f);
        return;
    }
    ellipse(a.x, a.y, 26, 6, C(0, 0, 0, 50));   // shadow
    Pose pose = a.sleeping ? PO_SLEEP : party ? PO_DANCE : a.walking ? PO_WALK : a.poseT > 0 ? (Pose)a.pose : PO_IDLE;
    g_walkPhase = a.walkPhase;
    if (pose == PO_SLEEP) {   // asleep away from a bed: curl up lying on the floor
        C3D_Mtx m; C2D_ViewSave(&m);
        C2D_ViewTranslate(a.x, a.y - 4); C2D_ViewRotate(a.facingLeft ? 1.5707963f : -1.5707963f);
        drawSim(sm, 0, simHeight(sm) * 0.45f, 0.9f, tt, g.holiday, PO_IDLE, false);
        C2D_ViewRestore(&m);
        const float zt = std::fmod(t + a.id, 2.f); text("z", a.x + 6 + zt * 5, a.y - 26 - zt * 8, 0.5f, C(255, 255, 255, (uint8_t)(230 - zt * 100)));
        return;
    }
    const TweenX tx = tweenGet(a.id, t);
    g_rigOn = true; g_rig[0] = tx.armL; g_rig[1] = tx.armR; g_rig[2] = tx.legL; g_rig[3] = tx.legR; g_rig[4] = tx.head;
    struct RigOff { ~RigOff() { g_rigOn = false; } } rigOff_;
    if (!tx.any && std::fabs(tx.sy - 1) < 0.0005f && tx.sx == 1) { drawSim(sm, a.x, a.y, 1.f, tt, g.holiday, pose, a.facingLeft); return; }
    C3D_Mtx m; C2D_ViewSave(&m);   // tween about the feet
    C2D_ViewTranslate(a.x + tx.dx, a.y + tx.dy); C2D_ViewRotate(tx.rot); C2D_ViewScale(tx.sx, tx.sy);
    drawSim(sm, 0, 0, 1.f, tt, g.holiday, pose, a.facingLeft);
    C2D_ViewRestore(&m);
}

// ---------------- full room ----------------
void drawRoom(const Game& g, float t, int selItem, int selActor, bool party) {
    const RoomState& rs = g.rooms[g.house][g.room];
    const bool night = !g.lightsOn;
    const bool music = audioIsPlaying();

    if (roomIsOuting(g.room)) {
        const bool painted = bgDrawRoom(g.room, night);   // Maple Valley: painted background art
        if (!painted) {
            if (g.room >= R_MALL_BEAUTY) districtDrawRoom(g.room, t, night);
            else drawOuting(g.house, g.room, t, night);
        }
        if (g.room == R_YARD) gardenDraw(t);
        {
            float mcX = -999;
            for (const Actor& a : rs.actors) if (a.kind == 0 && a.id == g.mc) mcX = a.x;
            g_spotMarkers = painted;
            districtDrawSpots(g.room, t, mcX);
            g_spotMarkers = false;
        }
    } else {
        if (shellPat(rs.wallPat) && !artIsFlip(g_artStyle)) drawShell(rs, g.house, t, night);
        else {
            drawWall(rs, t);
            if (g.room != R_BASEMENT && g.room != R_CLOSET && !artIsFlip(g_artStyle)) drawWindow(g.house, 30, 34, t, night);
        }
        if (g.room == R_KITCHEN) {   // backsplash tiles
            for (int r = 0; r < 4; r++) for (int k = 0; k < 15; k++) rrect(150 + k * 16.f, 108 + r * 12.f, 14, 10, 2, C(255, 255, 255, 60));
        }
        if (g.room == R_BASEMENT) {   // neon sign
            rrect(110, 24, 180, 36, 10, C(BLACK, 200));
            u32 neon = C(255, 150, 200, (uint8_t)(200 + 55 * std::sin(t * 6)));
            text("Girls Only Club", 200, 32, 0.6f, neon, 1);
            heart(124, 42, 5, C(HOT)); heart(276, 42, 5, C(HOT));
        }
        if (g.room == R_CLOSET) {   // chandelier
            line(200, 4, 200, 20, 1.2f, C(GOLD));
            quad(180, 20, 220, 20, 214, 30, 186, 30, C(GOLD));
            for (int i = 0; i < 7; i++) {
                float cx = 184 + i * 5.3f;
                circle(cx, 34 + (i % 2) * 4, 2, C(255, 255, 255, 220));
                sparkle(cx, 40 + (i % 3) * 3, 1.5f + std::fabs(std::sin(t * 3 + i)) * 2, C(255, 255, 255, 200));
            }
        }
        drawFloor(rs);
    }

    wallHolidayDecor(g.holiday, t);

    // wall items
    for (size_t i = 0; i < rs.items.size(); i++) {
        const Placed& pl = rs.items[i];
        if (SHAPES[itemShape(pl.item)].wall) drawItem(pl.item, pl.x, pl.y, 1.f, t, music);
    }
    // rugs
    for (size_t i = 0; i < rs.items.size(); i++) {
        const Placed& pl = rs.items[i];
        if (isRugShape(itemShape(pl.item))) drawItem(pl.item, pl.x, pl.y, 1.f, t, music);
    }
    floorHolidayDecor(g.holiday, t);

    // depth-sorted floor items and actors
    struct D { float y; int kind; int idx; };
    std::vector<D> list;
    for (size_t i = 0; i < rs.items.size(); i++) {
        int sh = itemShape(rs.items[i].item);
        if (!SHAPES[sh].wall && !isRugShape(sh)) list.push_back({(float)rs.items[i].y, 0, (int)i});
    }
    for (size_t i = 0; i < rs.actors.size(); i++) list.push_back({rs.actors[i].y + 0.5f, 1, (int)i});
    std::sort(list.begin(), list.end(), [](const D& a, const D& b) { return a.y < b.y; });
    for (const D& d : list) {
        if (d.kind == 0) {
            const Placed& pl = rs.items[d.idx];
            drawItem(pl.item, pl.x, pl.y, 1.f, t, music);
        } else {
            const Actor& a = rs.actors[d.idx];
            if (a.kind == 0) drawActor(g, rs, a, t, party);
            else { ellipse(a.x, a.y, 30, 6, C(0, 0, 0, 50)); drawPet(a.id, a.x, a.y, 0.95f, t + a.id, a.sleeping, a.facingLeft); }
        }
    }

    // night mode
    if (roomOutdoors(g.room)) drawWeather(t, true);
    seasonsDrawOverlay(t);
    if (night) {
        rect(0, 0, TOP_W, TOP_H, C(20, 10, 40, 140));
        for (const Placed& pl : rs.items) {
            int sh = itemShape(pl.item);
            if (sh == SH_LAMP) { circle(pl.x, pl.y - 64, 26, C(255, 220, 150, 50)); circle(pl.x, pl.y - 64, 12, C(255, 230, 170, 90)); }
            if (sh == SH_VANITY) circle(pl.x, pl.y - 52, 24, C(255, 230, 190, 50));
            if (sh == SH_TV || sh == SH_ARCADE) circle(pl.x, pl.y - 40, 26, C(200, 140, 255, 45));
        }
        if (g.holiday == HOL_XMAS || g.holiday == HOL_HALLOWEEN) stringLights(t, g.holiday);
    }
    for (const Actor& a : rs.actors) if (a.kind == 0) emoteDraw(a.id, a.x, actorHeadY(g, rs, a), t);   // emotion bubbles on top
    if (party) {
        for (int i = 0; i < 3; i++) {
            float sx = 80 + i * 120 + std::sin(t * 2 + i * 2) * 60;
            tri(sx, 0, sx - 40, 240, sx + 40, 240, C(i == 1 ? GOLD : HOT, 40));
        }
    }

    // selection highlights
    if (selItem >= 0 && selItem < (int)rs.items.size()) {
        const Placed& pl = rs.items[selItem];
        const ShapeInfo& si = SHAPES[itemShape(pl.item)];
        float a = 0.5f + 0.5f * std::sin(t * 8);
        u32 hc = C(255, 255, 255, (uint8_t)(120 + 120 * a));
        outline(pl.x - si.w / 2 - 3, pl.y - si.h - 3, si.w + 6, si.h + 6, 1.5f, hc);
        sparkle(pl.x + si.w / 2 + 3, pl.y - si.h - 3, 4 + a * 2, C(GOLD));
    }
    // plumbob-style gem above the sim you control (color = mood)
    for (const Actor& a : rs.actors) {
        if (a.kind != 0 || a.id != g.mc) continue;
        const Sim& s = g.sims[a.id];
        int m = moodOf(s);
        RGB gc = m > 30 ? RGB{90, 230, 120} : m > -20 ? RGB{240, 220, 80} : RGB{240, 80, 80};
        float gy = actorHeadY(g, rs, a) - 16 + std::sin(t * 2.5f) * 2;
        (void)s;
        quad(a.x, gy - 9, a.x + 5, gy, a.x, gy + 9, a.x - 5, gy, C(gc, 230));
        tri(a.x, gy - 9, a.x + 5, gy, a.x, gy, C(255, 255, 255, 90));
    }
    if (selActor >= 0 && selActor < (int)rs.actors.size()) {
        const Actor& a = rs.actors[selActor];
        float by = actorHeadY(g, rs, a) - (a.kind == 0 && a.id == g.mc ? 30 : 12) + std::sin(t * 6) * 3;
        heart(a.x, by, 5, C(HOT));
        heart(a.x, by, 3, C(255, 255, 255, 120));
    }
}

// Speech bubbles are drawn after the pixel layer so their text stays sharp.
void drawRoomOverlay(const Game& g, float t) {
    const RoomState& rs = g.rooms[g.house][g.room];
    (void)t;
    for (const Actor& a : rs.actors) {
        if (a.sayT <= 0 || a.say.empty()) continue;
        float w = textW(a.say.c_str(), 0.42f) + 12;
        float bx = clampv(a.x - w / 2, 2.f, TOP_W - w - 2), by = std::fmax(2.f, actorHeadY(g, rs, a) - 34);
        rrect(bx, by, w, 18, 7, C(CREAM, 240));
        tri(a.x - 4, by + 17, a.x + 4, by + 17, a.x, by + 24, C(CREAM, 240));
        text(a.say.c_str(), bx + w / 2, by + 3, 0.42f, C(CHOC), 1);
    }
}

// Map a sim's life stage onto the drawing model
static void drawSimBase(const Sim& s, float x, float footY, float scale, float t, int holiday, Pose pose, bool flip);
void drawSim(const Sim& s, float x, float footY, float scale, float t, int holiday, Pose pose, bool flip) {
    g_ccTime = t;
    const bool art = spriteKey(s) != 0;   // sprite art already has its own hair and clothes: no doll add-ons on top
    if (!art) ccDrawWear(s, x, footY, scale, flip, false, (int)pose);
    drawSimBase(s, x, footY, scale, t, holiday, pose, flip);
    if (!art) ccDrawWear(s, x, footY, scale, flip, true, (int)pose);
}
static void drawSimBase(const Sim& s, float x, float footY, float scale, float t, int holiday, Pose pose, bool flip) {
    if (ocDrawSim(s, x, footY, scale, pose, flip, t, g_walkPhase)) return;   // everyone (kids and up) is a doll now
    PersonDef d = s.look;
    float k = 1.f;
    switch (s.stage) {
    case ST_INFANT: d.age = BABY; break;
    case ST_TODDLER: d.age = BABY; k = 1.35f; break;
    case ST_CHILD: d.age = KID; break;
    case ST_PRETEEN: d.age = KID; k = 1.14f; break;
    case ST_TEEN: d.age = TEEN; k = 0.97f; break;
    default: d.age = ADULT; break;
    }
    if (s.pregnant) { /* a subtle glow so you know */ circle(x, footY - 70 * scale, 10 * scale, C(255, 200, 230, 60)); }
    {   // the stick figures wear their mood on their face
        static const uint8_t MOOD[EM_COUNT] = {0, 1, 4, 8, 0, 1, 1, 2, 3, 9, 6, 0, 2, 9};
        g_stickMood = MOOD[emotionOf(s) % EM_COUNT];
    }
    // Pixel-art style: draw the figure on the half-res pixel layer (unless the whole scene already is)
    bool own = artLayerBegin(1, g_artStyle);
    drawPerson(d, s.outfit, x, footY, scale * k, t, holiday, pose, flip);
    g_stickMood = 0;
    if (own) artLayerEnd(x - 70 * scale, footY - 170 * scale, x + 70 * scale, footY + 14 * scale);
}

void drawSimPortrait(const Sim& s, float cx, float cy, float r, float t) {
    (void)t; if (ocDrawSimBust(s, cx, cy, r)) return;
    if (artIsFlip(g_artStyle)) {
        PersonDef d = s.look;
        d.age = s.stage <= ST_TODDLER ? BABY : s.stage <= ST_PRETEEN ? KID : s.stage == ST_TEEN ? TEEN : ADULT;
        static const uint8_t MOOD[EM_COUNT] = {0, 1, 4, 8, 0, 1, 1, 2, 3, 9, 6, 0, 2, 9};
        g_stickMood = MOOD[emotionOf(s) % EM_COUNT];
        drawFlipBust(d, s.outfit, cx, cy, r, t);
        g_stickMood = 0;
        return;
    }
    if (artIsStick(g_artStyle)) {
        PersonDef d = s.look;
        d.age = s.stage <= ST_TODDLER ? BABY : s.stage <= ST_PRETEEN ? KID : s.stage == ST_TEEN ? TEEN : ADULT;
        static const uint8_t MOOD[EM_COUNT] = {0, 1, 4, 8, 0, 1, 1, 2, 3, 9, 6, 0, 2, 9};
        g_stickMood = MOOD[emotionOf(s) % EM_COUNT];
        bool own = artLayerBegin(1, g_artStyle);
        drawStickBust(d, s.outfit, cx, cy, r, t);
        if (own) artLayerEnd(cx - r * 1.6f, cy - r * 1.7f, cx + r * 1.6f, cy + r * 1.3f);
        g_stickMood = 0;
        return;
    }
    if (!artIsGlam(g_artStyle) || s.stage <= ST_TODDLER) {
        drawSim(s, cx, cy + r - 6, s.stage <= ST_TODDLER ? r / 32.f : r / 69.f, t, 0, PO_IDLE, false);
        return;
    }
    int of = s.outfit;
    PersonDef d = legacyLook(s.look, of);
    d.age = s.stage <= ST_PRETEEN ? KID : s.stage == ST_TEEN ? TEEN : ADULT;
    float k = r / 38.f * (d.age == KID ? 1.0f : 1.15f);
    bool own = artLayerBegin(1, g_artStyle);
    drawGlamBust(d, of, cx, cy + r * 0.3f, k, t);
    if (own) artLayerEnd(cx - r - 4, cy - r - 4, cx + r + 4, cy + r + 4);
}

float simHeight(const Sim& s) {
    if (s.stage >= ST_CHILD) return ocDollHeight(s);
    if (artIsFlip(g_artStyle)) {
        switch (s.stage) {
        case ST_INFANT: return 34;
        case ST_TODDLER: return 44;
        case ST_CHILD: return flipHeight(KID);
        case ST_PRETEEN: return flipHeight(KID) * 1.14f;
        case ST_TEEN: return flipHeight(TEEN);
        default: return flipHeight(ADULT);
        }
    }
    if (artIsStick(g_artStyle)) {
        switch (s.stage) {
        case ST_INFANT: return 26;
        case ST_TODDLER: return 34;
        case ST_CHILD: return stickHeight(KID, g_artStyle);
        case ST_PRETEEN: return stickHeight(KID, g_artStyle) * 1.14f;
        case ST_TEEN: return stickHeight(TEEN, g_artStyle) * 0.97f;
        default: return stickHeight(ADULT, g_artStyle);
        }
    }
    const float big = artIsGlam(g_artStyle) ? 4.f : 0.f;   // Glam Doll heads are bigger
    switch (s.stage) {
    case ST_INFANT: return 26;
    case ST_TODDLER: return 34;
    case ST_CHILD: return 84 + big;
    case ST_PRETEEN: return 96 + big;
    case ST_TEEN: return 109 + big;
    default: return 120 + big;
    }
}

// Per-style colour grading laid over a scene (inside the pixel layer when there is one)
void styleGrade(float t) {
    switch (g_artStyle) {
    case 5:   // Pastel Pixel: a dreamy pink-lilac wash with drifting sparkles
        grad(0, 0, TOP_W, TOP_H, C(255, 190, 230, 46), C(190, 200, 255, 40));
        for (int i = 0; i < 7; i++) {
            float x = std::fmod(i * 61.f + t * 9.f, 400.f), y = std::fmod(i * 37.f + 220 - t * (6 + i), 240.f);
            sparkle(x, y, 3 + (i % 3), C(255, 255, 255, (uint8_t)(140 + 100 * std::sin(t * 2 + i))));
        }
        break;
    case 6:   // Cozy Pixel: warm golden-hour light and a soft vignette
        rect(0, 0, TOP_W, TOP_H, C(255, 176, 96, 34));
        grad(0, 0, TOP_W, 34, C(40, 20, 10, 70), C(40, 20, 10, 0));
        grad(0, TOP_H - 34, TOP_W, 34, C(40, 20, 10, 0), C(40, 20, 10, 80));
        rect(0, 0, 18, TOP_H, C(40, 20, 10, 40)); rect(TOP_W - 18, 0, 18, TOP_H, C(40, 20, 10, 40));
        for (int i = 0; i < 5; i++) circle(std::fmod(i * 83.f + t * 4, 400.f), 40 + std::fmod(i * 47.f, 140.f) + std::sin(t + i) * 6, 1.5f, C(255, 230, 160, (uint8_t)(120 + 80 * std::sin(t * 3 + i))));
        break;
    default: break;
    }
}
