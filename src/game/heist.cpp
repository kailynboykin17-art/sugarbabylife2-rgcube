// Midnight Heist: a side-view stealth minigame. Sneak through a two-floor house,
// grab the loot, stay out of the homeowner's flashlight, don't wake the dog, and
// slip out the window before sunrise. (Teen 13+: cartoon cat-burglar stuff.)
#include "heist.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "crime.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace col;

static const float FLOOR_Y[2] = {222.f, 122.f};   // ground, upstairs
static const float STAIR_X0 = 26, STAIR_X1 = 78, EXIT_X = 366;
struct Loot { const char* name; float x; int floor, value; float grab; bool taken; };
static Loot loot[6];
struct Hide { const char* name; float x; int floor; };
static const Hide HIDES[3] = {{"Closet", 108, 0}, {"Wardrobe", 300, 1}, {"Curtain", 330, 0}};
static float px, alert, noise, timeLeft, grabT, endT, resX, resDir, resPause, resWalk, dogWake;
static int pFloor, resFloor, phase, bagN, bagVal, grabbing, hideAt, resTarget;   // phase 0 brief, 1 play, 2 caught, 3 escaped
static bool hidden, facingL, caughtFlag;
static int resident = -1;
static const char* endMsg = "";

void heistStart() {
    const Loot L[6] = {{"Flat-Screen TV", 150, 0, 120, 1.2f, false}, {"Game Console", 226, 0, 90, 0.8f, false}, {"Piggy Bank", 296, 0, 40, 0.5f, false},
                       {"Jewelry Box", 128, 1, 200, 1.0f, false}, {"Laptop", 222, 1, 150, 0.9f, false}, {"Fancy Painting", 330, 1, 180, 1.6f, false}};
    for (int i = 0; i < 6; i++) loot[i] = L[i];
    px = EXIT_X - 6; pFloor = 0; alert = 0; noise = 0; timeLeft = 100; grabT = 0; grabbing = -1; hidden = false; facingL = true;
    resFloor = 1; resX = 250; resDir = -1; resPause = 2; resWalk = 0; resTarget = -1; dogWake = 0;
    phase = 0; bagN = 0; bagVal = 0; endT = 0; caughtFlag = false; hideAt = -1;
    resident = -1;
    for (int tries = 0; tries < 40 && resident < 0; tries++) { int s = irand((int)G.sims.size()); if (s != G.mc && s != G.cop && !G.sims[s].household && G.sims[s].isAdult()) resident = s; }
    musicPlay(2);
}

static int nearLoot() { for (int i = 0; i < 6; i++) if (!loot[i].taken && loot[i].floor == pFloor && std::fabs(loot[i].x - px) < 18) return i; return -1; }
static int nearHide() { for (int i = 0; i < 3; i++) if (HIDES[i].floor == pFloor && std::fabs(HIDES[i].x - px) < 16) return i; return -1; }
static bool atStairs() { return px < STAIR_X1 && px > STAIR_X0 - 10; }
static void finish(bool caught, const char* msg) {
    if (phase >= 2) return;
    phase = caught ? 2 : 3; endT = 0; caughtFlag = caught; endMsg = msg;
    if (caught) sfxNo(); else sfxYay();
}

// touch buttons
static const Btn hbGrab{6, 196, 72, 36}, hbHide{82, 196, 72, 36}, hbUp{158, 196, 50, 36}, hbDown{212, 196, 50, 36}, hbExit{266, 196, 50, 36};

bool heistUpdate(const Input& in) {
    const float dt = in.dt;
    if (phase == 0) { if ((in.down & KEY_A) || in.touch) { phase = 1; sfxTap(); } return true; }
    if (phase >= 2) {
        endT += dt;
        if (endT > 1.0f && ((in.down & KEY_A) || in.touch)) {
            if (G.track >= 0) musicPlay(G.track);
            crimeHeistResult(caughtFlag, caughtFlag ? 0 : bagVal);
            return false;
        }
        return true;
    }
    timeLeft -= dt;
    if (timeLeft <= 0) { finish(true, "Sunrise! The crew drove off without you..."); return true; }
    const int ml = mischiefLevel();
    // ---- player
    float mx = 0;
    if (in.held & KEY_DLEFT) mx = -1; if (in.held & KEY_DRIGHT) mx = 1;
    if (std::abs(in.cp.dx) > 18) mx = in.cp.dx > 0 ? 1.f : -1.f;
    const bool sneak = (in.held & (KEY_L | KEY_R)) != 0, run = (in.held & KEY_B) != 0;
    bool tGrab = in.touch && hbGrab.in(in.tp), tHide = in.touch && hbHide.in(in.tp), tUp = in.touch && hbUp.in(in.tp), tDown = in.touch && hbDown.in(in.tp), tExit = in.touch && hbExit.in(in.tp);
    if ((in.down & KEY_X) || tHide) {
        if (hidden) { hidden = false; sfxTap(); }
        else if (nearHide() >= 0) { hidden = true; hideAt = nearHide(); px = HIDES[hideAt].x; sfxTap(); }
    }
    if (!hidden) {
        if (mx != 0 && grabbing < 0) {
            float sp = sneak ? 34.f + ml * 2 : run ? 120.f : 68.f;
            px = clampv(px + mx * sp * dt, 30.f, 372.f);
            facingL = mx < 0;
            g_walkPhase = std::fmod(g_walkPhase + dt * (sneak ? 1.2f : run ? 3.2f : 2.2f), 1.f);
            noise += (sneak ? 0.03f : run ? 0.85f : 0.22f) * (1.f - ml * 0.05f) * dt * 2.f;
        }
        if (((in.down & KEY_DUP) || tUp) && pFloor == 0 && atStairs()) { pFloor = 1; px = 60; noise += 0.12f; sfxTap(); }
        if (((in.down & KEY_DDOWN) || tDown) && pFloor == 1 && atStairs()) { pFloor = 0; px = 60; noise += 0.12f; sfxTap(); }
        // grab: hold A (or keep the button held)
        bool holdGrab = (in.held & KEY_A) || (tGrab) || (grabbing >= 0 && (in.held & KEY_TOUCH) && hbGrab.in(in.tp));
        int nl = nearLoot();
        if (holdGrab && nl >= 0 && bagN < 4) {
            if (grabbing != nl) { grabbing = nl; grabT = 0; }
            grabT += dt * (1.f + ml * 0.06f);
            noise += 0.06f * dt;
            if (grabT >= loot[nl].grab) { loot[nl].taken = true; bagN++; bagVal += loot[nl].value; grabbing = -1; sfx(700, 1100, 90); }
        } else if (!(in.held & KEY_A)) grabbing = -1;
        if (holdGrab && nl >= 0 && bagN >= 4 && (in.down & KEY_A)) sfxNo();
        if ((((in.down & KEY_A) && nl < 0) || tExit) && pFloor == 0 && px > EXIT_X - 16) { finish(false, bagN ? "Out the window and into the night. Clean getaway!" : "You slipped out empty-handed. The crew is NOT impressed."); return true; }
    }
    noise = clampv(noise - 0.35f * dt, 0.f, 1.f);
    // ---- the dog downstairs wakes up to loud noises
    if (dogWake <= 0 && pFloor == 0 && noise > 0.55f && std::fabs(px - 262) < 150) { dogWake = 4.f; resTarget = 0; sfx(300, 200, 120); }
    if (dogWake > 0) dogWake -= dt;
    // ---- the homeowner patrols with a flashlight
    float target = -1;
    if (resTarget >= 0) {   // investigating a noise / the dog
        if (resFloor != resTarget) { target = 52; if (std::fabs(resX - 52) < 6) { resFloor = resTarget; resX = 60; } }
        else { target = resTarget == pFloor && !hidden ? px : 262; if (std::fabs(resX - target) < 10) { resTarget = -1; resPause = 2.5f; } }
    } else if (resPause > 0) resPause -= dt;
    else {
        resWalk += dt;
        resX += resDir * 32 * dt;
        if (resX < 100) { resX = 100; resDir = 1; resPause = 1.5f + frand() * 2; }
        if (resX > 352) { resX = 352; resDir = -1; resPause = 1.5f + frand() * 2; }
        if (resWalk > 14 && irand(100) < 2) { resWalk = 0; resTarget = 1 - resFloor; }
    }
    if (target >= 0) { float d = target - resX; resDir = d < 0 ? -1.f : 1.f; resX += resDir * std::min(std::fabs(d), 55.f * dt); }
    if (noise > 0.5f && resFloor == pFloor && std::fabs(resX - px) < 170 && !hidden) { resDir = px < resX ? -1.f : 1.f; resPause = std::max(resPause, 0.6f); }
    // ---- spotted?
    bool seen = false;
    if (!hidden && resFloor == pFloor) {
        float d = (px - resX) * resDir;
        if (d > 0 && d < 112) seen = true;
        if (std::fabs(px - resX) < 14) seen = true;
    }
    alert += seen ? (1.9f - (std::fabs(px - resX) / 112.f)) * dt : -0.3f * dt;
    alert = clampv(alert, 0.f, 1.f);
    if (alert >= 1.f) finish(true, "The lights snap on. \"BURGLAR!!\" Sirens are already on the way...");
    return true;
}

static void drawRoomBox(float x, float y, float w, float h, RGB wall) {
    rect(x, y, w, h, C(wall));
    for (int i = 0; i < (int)(w / 24); i++) rect(x + i * 24.f + 4, y, 10, h, C(255, 255, 255, 10));
}
static void drawLootIcon(const Loot& l, float t) {
    float x = l.x, y = FLOOR_Y[l.floor];
    u32 K = C(UI_INK);
    switch ((int)(&l - loot)) {
    case 0: rect(x - 15, y - 34, 30, 20, K); rect(x - 13.5f, y - 32.5f, 27, 17, C(60, 80, 120)); rect(x - 2, y - 14, 4, 6, K); rect(x - 10, y - 9, 20, 9, C(120, 80, 60)); break;
    case 1: rrect(x - 10, y - 8, 20, 8, 3, K); rrect(x - 9, y - 7, 18, 6, 3, C(240, 240, 250)); circle(x + 5, y - 4, 1.4f, C(60, 200, 255)); break;
    case 2: circle(x, y - 7, 8, K); circle(x, y - 7, 6.8f, C(255, 160, 190)); circle(x + 6, y - 9, 2.4f, C(255, 160, 190)); rect(x - 2, y - 15, 4, 1.5f, K); break;
    case 3: rect(x - 9, y - 24, 18, 4, K); rect(x - 10, y - 12, 20, 12, C(120, 80, 60)); rrect(x - 8, y - 20, 16, 9, 2, K); rrect(x - 7, y - 19, 14, 7, 2, C(255, 90, 160)); sparkle(x + 5, y - 21, 3 + std::sin(t * 4), C(255, 255, 255)); break;
    case 4: rect(x - 10, y - 12, 20, 12, C(120, 80, 60)); quad(x - 8, y - 12, x + 8, y - 12, x + 9, y - 22, x - 7, y - 22, K); quad(x - 7, y - 13, x + 7, y - 13, x + 8, y - 21, x - 6, y - 21, C(180, 220, 255)); break;
    default: rect(x - 15, y - 76, 30, 24, K); rect(x - 13.5f, y - 74.5f, 27, 21, C(GOLD)); rect(x - 11, y - 72, 22, 16, C(120, 180, 230)); circle(x + 4, y - 66, 4, C(255, 200, 80)); break;
    }
    sparkle(x + 12, y - (l.floor == 1 && (&l - loot) == 5 ? 80 : 26), 2.5f + std::fabs(std::sin(t * 3 + l.x)) * 2, C(255, 240, 120));
}

void heistDrawTop(float t) {
    // night sky behind the house
    grad(0, 0, TOP_W, TOP_H, C(14, 14, 40), C(40, 30, 80));
    for (int i = 0; i < 16; i++) sparkle(std::fmod(i * 71.f, 400.f), 6 + (i * 13 % 20), 1 + std::fabs(std::sin(t * 2 + i)), C(255, 255, 220, 200));
    // the house cross-section
    rect(18 - 2, 22 - 2, 364 + 4, 202 + 4, C(UI_INK));
    tri(10, 24, 390, 24, 200, 4, C(UI_INK)); tri(18, 23, 382, 23, 200, 7, C(120, 50, 70));
    drawRoomBox(20, 24, 360, FLOOR_Y[1] - 24, RGB{70, 70, 120});
    drawRoomBox(20, FLOOR_Y[1] + 4, 360, FLOOR_Y[0] - FLOOR_Y[1] - 4, RGB{60, 80, 110});
    rect(20, FLOOR_Y[1], 360, 4, C(UI_INK)); rect(20, FLOOR_Y[0], 360, 4, C(UI_INK));
    // stairs
    for (int i = 0; i < 8; i++) rect(STAIR_X0 + i * 6.5f, FLOOR_Y[0] - (i + 1) * 12.5f, 14, 4, C(150, 110, 80));
    line(STAIR_X0, FLOOR_Y[0], STAIR_X1 + 6, FLOOR_Y[1], 2, C(UI_INK));
    // exit window
    rect(EXIT_X - 2, FLOOR_Y[0] - 58, 22, 40, C(UI_INK)); rect(EXIT_X, FLOOR_Y[0] - 56, 18, 36, C(30, 40, 90));
    if (phase == 1) sparkle(EXIT_X + 9, FLOOR_Y[0] - 64, 4 + std::sin(t * 4) * 1.5f, C(120, 255, 160));
    // hiding spots
    for (const Hide& h : HIDES) {
        float y = FLOOR_Y[h.floor];
        if (h.name[0] == 'C' && h.name[1] == 'u') { rect(h.x - 12, y - 70, 24, 70, C(UI_INK)); rect(h.x - 10.5f, y - 68, 21, 66, C(150, 60, 90)); for (int k = 0; k < 4; k++) rect(h.x - 9 + k * 5.f, y - 68, 1.5f, 66, C(110, 40, 70)); }
        else { rect(h.x - 15, y - 74, 30, 74, C(UI_INK)); rect(h.x - 13.5f, y - 72.5f, 27, 72.5f, C(130, 90, 60)); line(h.x, y - 72, h.x, y, 1, C(UI_INK)); circle(h.x - 3, y - 36, 1.4f, C(GOLD)); circle(h.x + 3, y - 36, 1.4f, C(GOLD)); }
    }
    // beds & furniture for flavour
    rect(150 - 2, FLOOR_Y[1] - 20, 64, 20, C(UI_INK)); rect(150, FLOOR_Y[1] - 18.5f, 60, 18, C(200, 160, 220)); rect(150, FLOOR_Y[1] - 26, 14, 10, C(255, 255, 255));
    // the dog
    {
        float dx = 262, dy = FLOOR_Y[0];
        ellipse(dx, dy - 6, 26, 12, C(UI_INK)); ellipse(dx, dy - 6, 24, 10, C(200, 150, 90));
        circle(dx + 12, dy - 10, 6.5f, C(UI_INK)); circle(dx + 12, dy - 10, 5.5f, C(200, 150, 90));
        tri(dx + 8, dy - 14, dx + 12, dy - 14, dx + 9, dy - 20, C(150, 100, 60));
        if (dogWake > 0) { circle(dx + 14, dy - 11, 1.2f, C(UI_INK)); textOutline(std::fmod(t, 0.5f) < 0.25f ? "WOOF!" : "woof!", dx + 10, dy - 40, 0.45f, C(255, 236, 120), C(UI_INK), 1); }
        else textOutline("z", dx + 18 + std::fmod(t, 2.f) * 3, dy - 24 - std::fmod(t, 2.f) * 5, 0.4f, C(220, 220, 255), C(UI_INK), 0, 0.6f);
    }
    for (int i = 0; i < 6; i++) if (!loot[i].taken) drawLootIcon(loot[i], t);
    // darkness, then the flashlight beam
    rect(20, 24, 360, FLOOR_Y[0] - 20, C(0, 0, 30, 90));
    const float ry = FLOOR_Y[resFloor];
    {
        float x0 = resX + resDir * 6, y0 = ry - 52;
        tri(x0, y0, x0 + resDir * 112, ry - 4, x0 + resDir * 112, ry - 80, C(255, 240, 150, alert > 0.5f ? 110 : 70));
    }
    // sims
    bool lay = artLayerBegin(1, g_artStyle);
    if (resident >= 0) { Sim r = G.sims[resident]; r.outfit = 5; drawSim(r, resX, ry, 0.62f, t, 0, resPause > 0 && resTarget < 0 ? PO_IDLE : PO_WALK, resDir < 0); }
    {
        Sim me = MC(); me.outfit = 31;
        if (hidden) {} else drawSim(me, px, FLOOR_Y[pFloor], 0.62f, t, 0, grabbing >= 0 ? PO_PHONE : PO_WALK, facingL);
    }
    if (lay) artLayerEnd(0, 0, 400, 240);
    if (hidden && hideAt >= 0) { const Hide& h = HIDES[hideAt]; circle(h.x - 3, FLOOR_Y[h.floor] - 52, 1.3f, C(255, 255, 255)); circle(h.x + 3, FLOOR_Y[h.floor] - 52, 1.3f, C(255, 255, 255)); }
    if (alert > 0.05f && phase == 1) textOutline(alert > 0.6f ? "!!" : "?", resX, ry - 92, 0.7f, C(255, 70, 90), C(UI_INK), 1);
    if (grabbing >= 0) { float k = grabT / loot[grabbing].grab; rrect(px - 16, FLOOR_Y[pFloor] - 88, 32, 6, 3, C(UI_INK)); rrect(px - 15, FLOOR_Y[pFloor] - 87, 30 * k, 4, 2, C(120, 255, 160)); }
    // HUD
    flashPanel(4, 4, 120, 18, RGB{40, 192, 210}, 6);
    char b[48]; snprintf(b, sizeof b, "Sunrise in %ds", (int)std::max(0.f, timeLeft));
    textOutline(b, 64, 4.5f, 0.4f, C(255, 255, 255), C(UI_INK), 1);
    if (phase == 0 || phase >= 2) {
        rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, 120));
        flashPanel(40, 60, 320, 110, phase == 2 ? RGB{255, 70, 100} : phase == 3 ? RGB{96, 206, 96} : RGB{150, 96, 230}, 14);
        textOutline(phase == 0 ? "MIDNIGHT HEIST" : phase == 2 ? "CAUGHT!" : "GETAWAY!", 200, 68, 0.8f, C(255, 255, 255), C(UI_INK), 1, 1.3f);
        std::string msg = phase == 0 ? "The crew's target: 42 Maple Lane. Grab the goods, stay out of the flashlight, don't wake the dog, and get out the window before sunrise." : endMsg;
        auto ls = wrapText(msg, 0.44f, 290);
        for (size_t i = 0; i < ls.size() && i < 4; i++) textOutline(ls[i].c_str(), 200, 102 + i * 15.f, 0.44f, C(255, 255, 255), C(UI_INK), 1, 0.8f);
    }
    if (phase == 2 && std::fmod(t, 0.5f) < 0.25f) rect(0, 0, TOP_W, TOP_H, C(255, 30, 60, 50));
}

void heistDrawBottom(float t) {
    bottomBackdrop(t);
    flashPanel(6, 6, 308, 20, RGB{150, 96, 230}, 7);
    char b[64]; snprintf(b, sizeof b, "LOOT BAG  %d/4    $%d", bagN, bagVal);
    textOutline(b, 160, 6.5f, 0.46f, C(255, 255, 255), C(UI_INK), 1);
    for (int i = 0; i < 6; i++) {
        float x = 8 + (i % 2) * 156.f, y = 32 + (i / 2) * 22.f;
        flashPanel(x, y, 148, 18, loot[i].taken ? RGB{150, 240, 160} : RGB{255, 255, 255}, 5, 235);
        snprintf(b, sizeof b, "%s%s $%d", loot[i].taken ? "* " : "", loot[i].name, loot[i].value);
        text(b, x + 5, y + 1.5f, 0.38f, C(UI_INK));
        text(loot[i].floor ? "up" : "down", x + 143, y + 2, 0.32f, C(120, 100, 140), 2);
    }
    // meters
    auto meter = [&](float y, const char* nm, float v, RGB c) {
        textOutline(nm, 8, y, 0.4f, C(255, 255, 255), C(UI_INK));
        rrect(66, y + 2, 248, 12, 5, C(UI_INK)); rrect(67.5f, y + 3.5f, 245, 9, 4, C(20, 50, 70));
        if (v > 0.01f) rrect(67.5f, y + 3.5f, 245 * v, 9, 4, C(c));
    };
    meter(102, "Noise", noise, RGB{255, 200, 40});
    meter(120, "Alert", alert, RGB{255, 60, 90});
    textOutline("D-Pad move  L/R sneak  B run  A grab/escape", 160, 140, 0.36f, C(220, 250, 255), C(UI_INK), 1, 0.7f);
    textOutline("X hide  Up/Down at the stairs", 160, 154, 0.36f, C(220, 250, 255), C(UI_INK), 1, 0.7f);
    if (phase == 1 && nearHide() >= 0 && !hidden) textOutline("Hiding spot here! (X)", 160, 172, 0.42f, C(150, 255, 160), C(UI_INK), 1);
    else if (phase == 1 && nearLoot() >= 0) textOutline(bagN >= 4 ? "Your bag is full!" : "Hold A to grab it!", 160, 172, 0.42f, C(255, 236, 120), C(UI_INK), 1);
    else if (phase == 1 && pFloor == 0 && px > EXIT_X - 16) textOutline("The window! Press A to escape", 160, 172, 0.42f, C(150, 255, 160), C(UI_INK), 1);
    button(hbGrab, "Grab", false, 2, 0.44f);
    button(hbHide, hidden ? "Leave" : "Hide", hidden, 3, 0.44f);
    button(hbUp, "Up", false, 1, 0.44f);
    button(hbDown, "Down", false, 1, 0.44f);
    button(hbExit, "Exit", false, 0, 0.44f);
    if (phase == 0) textOutline("Press A to start the job!", 160, 172, 0.5f, C(255, 236, 120), C(UI_INK), 1);
    if (phase >= 2 && endT > 1.f) textOutline("Press A to continue", 160, 172, 0.5f, C(255, 236, 120), C(UI_INK), 1);
}
