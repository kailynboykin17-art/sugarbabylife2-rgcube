// Story cutscenes: letterboxed little Flash-cartoon scenes starring your sims.
// Each scene is a list of shots: a backdrop, who's on stage (and where they walk),
// a line of dialogue and some effects. A advances, START skips.
#include "cutscene.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "crime.hpp"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

using namespace col;

const char* CS_NAMES[CS_COUNT] = {"Welcome to Starlight Bay", "Sunset Stroll", "The Big Day", "Bundle of Joy", "Busted!", "Scandal!"};

enum Bg { BG_STREET_BUS, BG_STREET, BG_HOUSE, BG_SUNSET, BG_WEDDING, BG_NURSERY, BG_NIGHT, BG_CARPET, BG_MAG };
enum Fx { FX_HEARTS = 1, FX_CONFETTI = 2, FX_SIREN = 4, FX_FLASH = 8, FX_FIREWORKS = 16, FX_SPARKLE = 32, FX_TITLE = 64, FX_CAMERAS = 128 };
// cast slots: 0 you, 1 your partner, 2 the baby, 3 Officer Reyes, 4-5 townies
struct CsActor { int8_t slot; float x0, x1, y; uint8_t pose; bool flip; float sc; };
struct Shot { uint8_t bg; int8_t speaker; const char* line; uint8_t fx; CsActor a[4]; };
#define NA {-1, 0, 0, 0, 0, false, 1}
static const Shot S_INTRO[] = {
    {BG_STREET_BUS, -1, "A brand-new town. A brand-new life.", FX_TITLE, {NA, NA, NA, NA}},
    {BG_STREET, 0, "So THIS is Starlight Bay... it's even cuter than the pictures!", FX_SPARKLE, {{0, -20, 190, 226, PO_WALK, false, 1.05f}, NA, NA, NA}},
    {BG_STREET, 4, "Heyyy! You must be the new neighbor! Welcome to the block!", 0, {{0, 170, 170, 226, PO_IDLE, false, 1.05f}, {4, 430, 270, 228, PO_WAVE, true, 1.05f}, NA, NA}},
    {BG_STREET, 0, "Thanks! I have a feeling I'm going to love it here.", FX_HEARTS, {{0, 170, 170, 226, PO_WAVE, false, 1.05f}, {4, 270, 270, 228, PO_IDLE, true, 1.05f}, NA, NA}},
    {BG_HOUSE, -1, "Home sweet home. Time to make some memories...", FX_SPARKLE, {{0, 120, 200, 228, PO_WALK, false, 1.05f}, NA, NA, NA}},
};
static const Shot S_DATE[] = {
    {BG_SUNSET, -1, "The sun is melting into the ocean...", FX_TITLE, {{0, -20, 165, 226, PO_WALK, false, 1.05f}, {1, 420, 235, 226, PO_WALK, true, 1.05f}, NA, NA}},
    {BG_SUNSET, 1, "So... are we, like, officially a thing now?", 0, {{0, 165, 165, 226, PO_IDLE, false, 1.05f}, {1, 235, 235, 226, PO_IDLE, true, 1.05f}, NA, NA}},
    {BG_SUNSET, 0, "Yeah. We're officially a thing.", FX_HEARTS, {{0, 165, 172, 226, PO_IDLE, false, 1.05f}, {1, 235, 228, 226, PO_IDLE, true, 1.05f}, NA, NA}},
    {BG_SUNSET, -1, "Somewhere down the beach, a firework goes off. Perfect timing.", FX_FIREWORKS | FX_HEARTS, {{0, 176, 178, 226, PO_IDLE, false, 1.05f}, {1, 224, 222, 226, PO_IDLE, true, 1.05f}, NA, NA}},
};
static const Shot S_WEDDING[] = {
    {BG_WEDDING, -1, "The flowers are perfect. The guests are crying already.", FX_TITLE, {{4, 60, 60, 232, PO_IDLE, false, 0.95f}, {5, 342, 342, 232, PO_IDLE, true, 0.95f}, {0, -20, 172, 222, PO_WALK, false, 1.0f}, {1, 420, 228, 222, PO_WALK, true, 1.0f}}},
    {BG_WEDDING, -1, "\"Do you take each other - for better, for worse, for brunch and for drama?\"", 0, {{4, 60, 60, 232, PO_IDLE, false, 0.95f}, {5, 342, 342, 232, PO_IDLE, true, 0.95f}, {0, 172, 172, 222, PO_IDLE, false, 1.0f}, {1, 228, 228, 222, PO_IDLE, true, 1.0f}}},
    {BG_WEDDING, 1, "I do!", FX_HEARTS, {{4, 60, 60, 232, PO_WAVE, false, 0.95f}, {5, 342, 342, 232, PO_IDLE, true, 0.95f}, {0, 172, 172, 222, PO_IDLE, false, 1.0f}, {1, 228, 228, 222, PO_IDLE, true, 1.0f}}},
    {BG_WEDDING, 0, "I DO!!", FX_CONFETTI | FX_HEARTS, {{4, 60, 60, 232, PO_DANCE, false, 0.95f}, {5, 342, 342, 232, PO_WAVE, true, 0.95f}, {0, 172, 180, 222, PO_IDLE, false, 1.0f}, {1, 228, 220, 222, PO_IDLE, true, 1.0f}}},
};
static const Shot S_BABY[] = {
    {BG_NURSERY, -1, "The nursery is quiet. Someone very small just arrived.", FX_TITLE | FX_SPARKLE, {{0, 120, 140, 228, PO_WALK, false, 1.05f}, {1, 300, 268, 228, PO_WALK, true, 1.05f}, {2, 204, 204, 168, PO_SLEEP, false, 1.4f}, NA}},
    {BG_NURSERY, 0, "Look at those tiny hands... Hi, {B}. We're your family.", FX_HEARTS, {{0, 140, 140, 228, PO_IDLE, false, 1.05f}, {1, 268, 268, 228, PO_IDLE, true, 1.05f}, {2, 204, 204, 168, PO_IDLE, false, 1.4f}, NA}},
    {BG_NURSERY, -1, "{B} giggles. Everything just changed - in the best way.", FX_HEARTS | FX_SPARKLE, {{0, 140, 146, 228, PO_WAVE, false, 1.05f}, {1, 268, 262, 228, PO_IDLE, true, 1.05f}, {2, 204, 204, 168, PO_WAVE, false, 1.4f}, NA}},
};
static const Shot S_BUSTED[] = {
    {BG_NIGHT, -1, "Starlight Bay, after dark. Somebody's being sneaky...", FX_TITLE, {{0, 60, 190, 226, PO_WALK, false, 1.05f}, NA, NA, NA}},
    {BG_NIGHT, 3, "FREEZE! Hands where I can see 'em!", FX_SIREN, {{0, 190, 180, 226, PO_WAVE, true, 1.05f}, {3, 430, 280, 226, PO_WALK, true, 1.1f}, NA, NA}},
    {BG_NIGHT, 0, "Officer, I can explain! It's... a fashion emergency?", FX_SIREN, {{0, 180, 180, 226, PO_IDLE, false, 1.05f}, {3, 280, 280, 226, PO_IDLE, true, 1.1f}, NA, NA}},
    {BG_NIGHT, 3, "Uh-huh. Tell it to the judge, sweetheart.", FX_SIREN, {{0, 180, 180, 226, PO_IDLE, false, 1.05f}, {3, 280, 270, 226, PO_PHONE, true, 1.1f}, NA, NA}},
};
static const Shot S_SCANDAL[] = {
    {BG_CARPET, -1, "Word travels fast in a small town.", FX_TITLE | FX_FLASH | FX_CAMERAS, {{4, 70, 70, 232, PO_PHONE, false, 1.0f}, {5, 335, 335, 232, PO_PHONE, true, 1.0f}, {0, 150, 200, 226, PO_WALK, false, 1.08f}, NA}},
    {BG_CARPET, 4, "Over here! Over HERE! Is it TRUE?!", FX_FLASH | FX_CAMERAS, {{4, 70, 80, 232, PO_PHONE, false, 1.0f}, {5, 335, 325, 232, PO_PHONE, true, 1.0f}, {0, 200, 200, 226, PO_PHONE, false, 1.08f}, NA}},
    {BG_MAG, -1, "By morning it's on every phone in Starlight Bay.", 0, {NA, NA, NA, NA}},
    {BG_CARPET, 0, "This is NOT happening right now...", FX_FLASH | FX_CAMERAS, {{4, 80, 80, 232, PO_PHONE, false, 1.0f}, {5, 325, 325, 232, PO_PHONE, true, 1.0f}, {0, 200, 200, 226, PO_IDLE, false, 1.08f}, NA}},
};
struct Scene { const Shot* shots; int n; };
static const Scene SCENES[CS_COUNT] = {
    {S_INTRO, 5}, {S_DATE, 4}, {S_WEDDING, 4}, {S_BABY, 3}, {S_BUSTED, 4}, {S_SCANDAL, 4},
};

static int pending = -1, cur = -1, shot = 0;
static float st = 0, typed = 0;
static int cast[6];

void cutscenePlay(int id, bool replay) {
    if (id < 0 || id >= CS_COUNT) return;
    if (!replay && (G.csSeen & (1u << id))) return;
    pending = id;
}
bool cutscenePending() { return pending >= 0; }

static int randomTownie(int avoid) {
    for (int tries = 0; tries < 40; tries++) {
        int s = irand((int)G.sims.size());
        if (s != G.mc && s != avoid && s != G.cop && !G.sims[s].household && G.sims[s].stage >= ST_TEEN) return s;
    }
    return -1;
}
void cutsceneBegin() {
    cur = pending; pending = -1;
    if (cur < 0) return;
    G.csSeen |= (uint16_t)(1u << cur);
    shot = 0; st = 0; typed = 0;
    cast[0] = G.mc;
    int p = partnerOf(G.mc);
    if (p < 0 && G.date.partner >= 0) p = G.date.partner;
    cast[1] = p >= 0 ? p : randomTownie(-1);
    cast[2] = -1;
    for (int i = (int)G.sims.size() - 1; i >= 0; i--) if (G.sims[i].household && G.sims[i].stage <= ST_TODDLER) { cast[2] = i; break; }
    cast[3] = (cur == CS_BUSTED) ? copSim() : -1;
    cast[4] = randomTownie(cast[1]);
    cast[5] = randomTownie(cast[4]);
    musicPlay(cur == CS_BUSTED || cur == CS_SCANDAL ? 2 : 1);
}

static std::string fillLine(const char* s) {
    std::string o = s ? s : "";
    size_t k;
    while ((k = o.find("{B}")) != std::string::npos) o.replace(k, 3, cast[2] >= 0 ? G.sims[cast[2]].name : std::string("little one"));
    return o;
}

bool cutsceneUpdate(const Input& in) {
    if (cur < 0) return false;
    st += in.dt; typed += in.dt * 45;
    const Scene& sc = SCENES[cur];
    if (in.down & KEY_START) { cur = -1; if (G.track >= 0) musicPlay(G.track); return false; }
    if ((in.down & KEY_A) || in.touch) {
        std::string ln = fillLine(sc.shots[shot].line);
        if (typed < ln.size()) { typed = 999; return true; }
        shot++; st = 0; typed = 0; sfxTap();
        if (shot >= sc.n) { cur = -1; if (G.track >= 0) musicPlay(G.track); return false; }
    }
    return true;
}

// ---------------------------------------------------------------- backdrops
static void house(float x, float y, float w, float h, RGB wall, RGB roof) {
    rect(x - 1.5f, y - 1.5f, w + 3, h + 3, C(UI_INK)); rect(x, y, w, h, C(wall));
    tri(x - 8, y + 1, x + w + 8, y + 1, x + w / 2, y - h * 0.55f, C(UI_INK)); tri(x - 5, y, x + w + 5, y, x + w / 2, y - h * 0.5f, C(roof));
    rect(x + w * 0.4f - 1.5f, y + h * 0.45f - 1.5f, w * 0.2f + 3, h * 0.55f + 1.5f, C(UI_INK)); rect(x + w * 0.4f, y + h * 0.45f, w * 0.2f, h * 0.55f, C(shade(roof, 0.8f)));
    for (int k = 0; k < 2; k++) { float wx = x + w * (k ? 0.68f : 0.1f); rect(wx - 1.5f, y + h * 0.2f - 1.5f, w * 0.22f + 3, h * 0.22f + 3, C(UI_INK)); rect(wx, y + h * 0.2f, w * 0.22f, h * 0.22f, C(180, 230, 255)); }
}
static void drawBg(int bg, float t, float prog) {
    switch (bg) {
    case BG_STREET_BUS: case BG_STREET: case BG_HOUSE: {
        grad(0, 0, TOP_W, 170, C(90, 200, 245), C(210, 242, 255));
        flashSun(340, 40, 18, t); flashCloud(80 + std::fmod(t * 5, 60.f), 36, 0.9f); flashCloud(230, 58, 0.7f);
        if (bg == BG_HOUSE) {
            rect(0, 170, TOP_W, 70, C(110, 200, 100)); rect(0, 169, TOP_W, 2, C(UI_INK));
            house(120, 70, 160, 110, RGB{255, 220, 236}, RGB{255, 70, 150});
            quad(184, 180, 216, 180, 240, 240, 160, 240, C(236, 210, 170));
            for (int i = 0; i < 8; i++) { float fx = 20 + i * 12 + (i > 3) * 260; circle(fx, 178, 4, C(UI_INK)); circle(fx, 178, 3, C(i % 2 ? RGB{255, 90, 160} : RGB{255, 220, 60})); }
        } else {
            house(10, 100, 90, 70, RGB{180, 230, 255}, RGB{60, 170, 220}); house(130, 96, 100, 74, RGB{255, 236, 170}, RGB{255, 140, 60}); house(270, 100, 100, 70, RGB{220, 200, 255}, RGB{150, 96, 230});
            rect(0, 170, TOP_W, 70, C(200, 200, 210)); rect(0, 169, TOP_W, 2, C(UI_INK)); rect(0, 200, TOP_W, 40, C(90, 90, 104));
            for (int i = 0; i < 8; i++) rect(i * 56.f + 10, 218, 30, 3, C(255, 220, 80));
            if (bg == BG_STREET_BUS) {   // the bus pulls in
                float bx = -160 + std::min(1.f, prog) * 300;
                rrect(bx - 1.5f, 168.5f, 143, 53, 9, C(UI_INK)); rrect(bx, 170, 140, 50, 8, C(255, 200, 40));
                for (int k = 0; k < 4; k++) { rect(bx + 10 + k * 30 - 1, 178 - 1, 24, 16, C(UI_INK)); rect(bx + 10 + k * 30, 178, 22, 14, C(180, 230, 255)); }
                circle(bx + 30, 222, 9, C(UI_INK)); circle(bx + 110, 222, 9, C(UI_INK)); circle(bx + 30, 222, 4, C(200, 200, 210)); circle(bx + 110, 222, 4, C(200, 200, 210));
                textOutline("STARLIGHT BAY", bx + 70, 196, 0.36f, C(255, 255, 255), C(UI_INK), 1, 0.7f);
            }
        }
        break;
    }
    case BG_SUNSET: {
        grad(0, 0, TOP_W, 130, C(120, 70, 170), C(255, 150, 120));
        circle(200, 128, 34, C(255, 200, 90)); circle(200, 128, 28, C(255, 230, 140));
        grad(0, 120, TOP_W, 50, C(255, 140, 140), C(120, 90, 170));
        for (int i = 0; i < 8; i++) ellipse(i * 56.f + std::fmod(t * 10, 56.f), 132 + (i % 2) * 14, 34, 3, C(255, 220, 180, 150));
        grad(0, 166, TOP_W, 74, C(250, 200, 150), C(230, 170, 120)); rect(0, 165, TOP_W, 2, C(UI_INK));
        line(40, 230, 52, 120, 6, C(UI_INK)); line(40, 230, 52, 120, 4, C(150, 100, 60));
        for (int k = 0; k < 5; k++) { float a = -2.6f + k * 0.55f; quad(52, 120, 52 + std::cos(a) * 30, 120 + std::sin(a) * 12, 52 + std::cos(a) * 40, 124 + std::sin(a) * 16 + 8, 52, 124, C(70, 140, 80)); }
        for (int i = 0; i < 12; i++) sparkle(std::fmod(i * 47.f, 400.f), 10 + (i * 23 % 60), 1.5f + std::fabs(std::sin(t * 2 + i)) * 2, C(255, 255, 255, 180));
        break;
    }
    case BG_WEDDING: {
        grad(0, 0, TOP_W, 160, C(150, 220, 255), C(230, 248, 255));
        flashCloud(70, 40, 0.8f); flashCloud(330, 30, 0.7f);
        rect(0, 160, TOP_W, 80, C(130, 210, 110)); rect(0, 159, TOP_W, 2, C(UI_INK));
        quad(170, 240, 230, 240, 214, 160, 186, 160, C(255, 255, 255)); quad(176, 240, 224, 240, 210, 160, 190, 160, C(255, 200, 220));
        // the arch
        for (int i = 0; i <= 16; i++) { float a = 3.14159f * i / 16; float x = 200 - std::cos(a) * 62, y = 160 - std::sin(a) * 100; circle(x, y, 7, C(UI_INK)); circle(x, y, 5.5f, C(255, 255, 255)); if (i % 2) { circle(x, y - 2, 3.5f, C(255, 90, 160)); } }
        rect(136, 160, 6, 64, C(UI_INK)); rect(258, 160, 6, 64, C(UI_INK)); rect(137, 160, 4, 64, C(255, 255, 255)); rect(259, 160, 4, 64, C(255, 255, 255));
        for (int r = 0; r < 2; r++) for (int k = 0; k < 3; k++) { rrect(14 + k * 34.f, 196 + r * 22.f, 28, 14, 3, C(UI_INK)); rrect(15 + k * 34.f, 197 + r * 22.f, 26, 12, 3, C(255, 255, 255)); rrect(296 + k * 34.f, 196 + r * 22.f, 28, 14, 3, C(UI_INK)); rrect(297 + k * 34.f, 197 + r * 22.f, 26, 12, 3, C(255, 255, 255)); }
        for (int i = 0; i < 14; i++) { float px = std::fmod(i * 61.f + t * 20, 400.f), py = std::fmod(i * 37.f + t * 30, 240.f); ellipse(px, py, 4, 2.5f, C(255, 160, 200, 200)); }
        break;
    }
    case BG_NURSERY: {
        grad(0, 0, TOP_W, 180, C(255, 230, 240), C(240, 220, 255));
        for (int i = 0; i < 10; i++) rect(i * 40.f, 0, 18, 180, C(255, 255, 255, 60));
        rect(0, 180, TOP_W, 60, C(240, 214, 180)); rect(0, 179, TOP_W, 2, C(UI_INK));
        rect(300, 30, 70, 60, C(UI_INK)); rect(302, 32, 66, 56, C(40, 50, 110));
        for (int i = 0; i < 5; i++) sparkle(310 + i * 12, 44 + (i % 2) * 20, 2 + std::sin(t * 3 + i), C(255, 240, 160));
        circle(352, 48, 7, C(255, 245, 200));
        // crib
        rect(168, 148, 72, 40, C(UI_INK)); rect(170, 150, 68, 36, C(255, 255, 255));
        for (int i = 0; i < 8; i++) rect(172 + i * 9.f, 150, 2, 36, C(220, 200, 230));
        rect(166, 140, 4, 54, C(UI_INK)); rect(238, 140, 4, 54, C(UI_INK));
        // mobile
        line(204, 0, 204, 60, 1.5f, C(UI_INK));
        for (int i = 0; i < 4; i++) { float a = t + i * 1.57f; float mx = 204 + std::cos(a) * 22, my = 64 + std::sin(a) * 4; line(204, 60, mx, my, 1, C(UI_INK)); sparkle(mx, my + 6, 5, C(i % 2 ? RGB{255, 200, 60} : RGB{150, 200, 255})); }
        break;
    }
    case BG_NIGHT: {
        grad(0, 0, TOP_W, 170, C(16, 18, 52), C(50, 40, 100));
        for (int i = 0; i < 20; i++) sparkle(std::fmod(i * 67.f, 400.f), 8 + (i * 29 % 110), 1 + std::fabs(std::sin(t * 2 + i)) * 1.5f, C(255, 255, 220, 200));
        circle(330, 40, 14, C(250, 246, 220));
        rect(0, 120, 120, 50, C(30, 30, 60)); rect(280, 110, 120, 60, C(34, 30, 64));
        for (int i = 0; i < 5; i++) rect(20 + i * 20.f, 130, 8, 10, C(255, 220, 120, i % 2 ? 220 : 60));
        rect(0, 170, TOP_W, 70, C(60, 60, 76)); rect(0, 169, TOP_W, 2, C(UI_INK));
        line(100, 170, 100, 90, 3, C(UI_INK)); circle(100, 90, 6, C(255, 230, 150)); quad(70, 170, 130, 170, 106, 92, 94, 92, C(255, 230, 150, 30));
        // police car
        rrect(300 - 1.5f, 186 - 1.5f, 98, 31, 7, C(UI_INK)); rrect(300, 186, 95, 28, 6, C(250, 250, 250)); rect(300, 200, 95, 8, C(44, 62, 120));
        circle(318, 216, 7, C(UI_INK)); circle(378, 216, 7, C(UI_INK));
        bool on = std::fmod(t, 0.5f) < 0.25f;
        rrect(336, 178, 10, 8, 2, C(on ? RGB{255, 40, 60} : RGB{120, 30, 40})); rrect(348, 178, 10, 8, 2, C(on ? RGB{60, 80, 160} : RGB{60, 140, 255}));
        textOutline("POLICE", 347, 199, 0.32f, C(255, 255, 255), C(UI_INK), 1, 0.6f);
        break;
    }
    case BG_CARPET: {
        grad(0, 0, TOP_W, 170, C(40, 20, 50), C(90, 40, 90));
        for (int i = 0; i < 9; i++) { circle(20 + i * 46.f, 150, 18, C(30, 16, 40)); ellipse(20 + i * 46.f, 175, 40, 30, C(30, 16, 40)); }
        rect(0, 176, TOP_W, 64, C(70, 20, 40));
        quad(140, 176, 260, 176, 320, 240, 80, 240, C(220, 30, 60)); quad(140, 176, 260, 176, 262, 180, 138, 180, C(255, 90, 110));
        for (int i = 0; i < 6; i++) { float x = i < 3 ? 30 + i * 30 : 280 + (i - 3) * 30; rect(x - 1, 186, 4, 30, C(GOLD)); circle(x + 1, 186, 3, C(GOLD)); }
        textOutline("STARLIGHT BAY GALA", 200, 10, 0.5f, C(255, 220, 120), C(UI_INK), 1);
        break;
    }
    case BG_MAG: {
        rect(0, 0, TOP_W, TOP_H, C(30, 20, 40));
        rect(100 - 2, 6 - 2, 200 + 4, 228 + 4, C(UI_INK)); grad(100, 6, 200, 228, C(255, 90, 170), C(255, 46, 120));
        textOutline("THE TEA", 200, 8, 1.0f, C(255, 255, 255), C(UI_INK), 1, 1.4f);
        circle(200, 120, 52, C(255, 255, 255)); drawSimPortrait(MC(), 200, 120, 50, t);
        std::string h = G.tea.empty() ? MC().name + " in HOT water!" : scandalHeadline(G.tea.front());
        auto ls = wrapText(h, 0.5f, 180);
        for (size_t i = 0; i < ls.size() && i < 3; i++) textOutline(ls[i].c_str(), 200, 176 + i * 16.f, 0.5f, C(255, 236, 100), C(UI_INK), 1, 1.0f);
        sparkle(126, 60, 8, C(255, 255, 255)); sparkle(276, 90, 6, C(255, 255, 255));
        textOutline("EXCLUSIVE!", 150, 46, 0.42f, C(255, 255, 255), C(UI_INK), 1);
        break;
    }
    }
}

static void drawFx(int fx, float t, float prog) {
    if (fx & FX_HEARTS) for (int i = 0; i < 8; i++) { float y = 200 - std::fmod(t * 30 + i * 30, 200.f); heart(140 + (i * 37) % 130 + std::sin(t * 2 + i) * 8, y, 5 + (i % 3), C(255, 70, 150, 220)); }
    if (fx & FX_CONFETTI) { static const RGB CC[5] = {{255, 46, 140}, {255, 200, 40}, {40, 192, 210}, {150, 96, 230}, {96, 206, 96}}; for (int i = 0; i < 40; i++) { float x = std::fmod(i * 53.f + std::sin(t + i) * 20, 400.f), y = std::fmod(i * 31.f + t * 60, 240.f); rect(x, y, 4, 2.5f, C(CC[i % 5])); } }
    if (fx & FX_SIREN) { bool on = std::fmod(t, 0.5f) < 0.25f; rect(0, 0, TOP_W, TOP_H, on ? C(255, 30, 60, 50) : C(40, 80, 255, 50)); }
    if (fx & FX_FIREWORKS) for (int k = 0; k < 3; k++) { float ph = std::fmod(t * 0.7f + k * 0.33f, 1.f); float cx = 80 + k * 120, cy = 40 + (k % 2) * 25; for (int i = 0; i < 12; i++) { float a = i * 0.5236f; circle(cx + std::cos(a) * ph * 34, cy + std::sin(a) * ph * 34, 2.2f * (1 - ph) + 0.5f, C(k == 0 ? RGB{255, 90, 170} : k == 1 ? RGB{255, 220, 80} : RGB{120, 220, 255}, (uint8_t)(255 * (1 - ph)))); } }
    if (fx & FX_FLASH) { float f = std::fmod(t * 1.7f, 1.f); if (f < 0.08f) rect(0, 0, TOP_W, TOP_H, C(255, 255, 255, 170)); }
    if (fx & FX_SPARKLE) for (int i = 0; i < 10; i++) sparkle(std::fmod(i * 41.f, 400.f), 20 + (i * 53 % 160), 2 + std::fabs(std::sin(t * 3 + i)) * 4, C(255, 255, 255, 220));
    (void)prog;
}

void cutsceneDrawTop(float t) {
    if (cur < 0) return;
    const Shot& sh = SCENES[cur].shots[shot];
    const float prog = std::min(1.f, st / 2.2f);
    const float e = prog * prog * (3 - 2 * prog);
    drawBg(sh.bg, t, prog);
    bool lay = artLayerBegin(1, g_artStyle);
    for (const CsActor& a : sh.a) {
        if (a.slot < 0 || cast[a.slot] < 0 || cast[a.slot] >= (int)G.sims.size()) continue;
        float x = a.x0 + (a.x1 - a.x0) * e;
        Pose p = (Pose)a.pose;
        if (p == PO_WALK && prog >= 1.f) p = PO_IDLE;
        g_walkPhase = std::fmod(t * 1.7f, 1.f);
        drawSim(G.sims[cast[a.slot]], x, a.y, a.sc, t + a.slot, 0, p, a.flip);
        if ((sh.fx & FX_CAMERAS) && a.slot >= 4) { float cx = x + (a.flip ? -10 : 10); rrect(cx - 7, a.y - 92, 14, 10, 2, C(UI_INK)); circle(cx, a.y - 87, 3, C(180, 200, 220)); }
    }
    if (lay) artLayerEnd(0, 0, 400, 240);
    drawFx(sh.fx, t, prog);
    // letterbox + title card
    rect(0, 0, TOP_W, 16, C(0, 0, 0)); rect(0, TOP_H - 16, TOP_W, 16, C(0, 0, 0));
    if ((sh.fx & FX_TITLE) && st < 3.f) {
        float a = st < 2.4f ? 1.f : (3.f - st) / 0.6f;
        flashPanel(80, 96, 240, 40, RGB{255, 46, 140}, 14, (uint8_t)(235 * a));
        textOutline(CS_NAMES[cur], 200, 103, 0.7f, C(255, 255, 255, (uint8_t)(255 * a)), C(UI_INK, (uint8_t)(255 * a)), 1, 1.2f);
    }
    if (st < 0.35f) rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, (uint8_t)(255 * (1 - st / 0.35f))));
}

void cutsceneDrawBottom(float t) {
    if (cur < 0) return;
    bottomBackdrop(t);
    const Shot& sh = SCENES[cur].shots[shot];
    flashPanel(8, 8, 304, 22, RGB{255, 46, 140}, 8);
    char b[64]; snprintf(b, sizeof b, "%s   %d/%d", CS_NAMES[cur], shot + 1, SCENES[cur].n);
    textOutline(b, 160, 10, 0.44f, C(255, 255, 255), C(UI_INK), 1);
    flashPanel(10, 50, 300, 130, RGB{255, 255, 255}, 12);
    int spk = sh.speaker >= 0 ? cast[sh.speaker] : -1;
    std::string nm = spk >= 0 && spk < (int)G.sims.size() ? G.sims[spk].name : std::string("Narrator");
    float nw = textW(nm.c_str(), 0.46f) + 22;
    flashPanel(22, 40, nw, 20, RGB{40, 192, 210}, 8);
    textOutline(nm.c_str(), 22 + nw / 2, 41, 0.46f, C(255, 255, 255), C(UI_INK), 1);
    float tx = 24;
    if (spk >= 0 && spk < (int)G.sims.size()) { circle(50, 110, 26, C(UI_INK)); circle(50, 110, 24, C(255, 236, 244)); drawSimPortrait(G.sims[spk], 50, 110, 24, t); tx = 84; }
    std::string ln = fillLine(sh.line);
    std::string shown = ln.substr(0, std::min(ln.size(), (size_t)typed));
    auto ls = wrapText(shown, 0.5f, 300 - (tx - 10) - 14);
    for (size_t i = 0; i < ls.size() && i < 5; i++) text(ls[i].c_str(), tx, 70 + i * 19.f, 0.5f, C(UI_INK));
    textOutline("A: next     START: skip", 160, 196, 0.42f, C(255, 255, 255), C(UI_INK), 1);
}
