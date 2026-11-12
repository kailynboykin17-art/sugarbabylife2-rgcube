// Otome Ver. Hold the 3DS sideways like a book (rotate it to the left: the top
// screen becomes the left page, the touch screen the right page). Everything here is drawn in
// "page" coordinates (240 wide, 400 / 320 tall) and rotated onto the screens.
#include "otome.hpp"
#include "otome_story.hpp"
#include "game.hpp"
#include "draw.hpp"
#include "oc.hpp"
#include "audio.hpp"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace col;
using namespace otome;

namespace {
enum Phase { PH_FLIP, PH_PICK, PH_PLAY, PH_CHOICE, PH_CHEND, PH_ENDING };
int phase = PH_FLIP, route = 0, chap = 0, pos = 0, picked = -1, gained = 0;
bool bonus = false;
float flipT = 0, typeT = 0;
struct Line { int who; std::string text; };
std::vector<Line> lines;   // the chapter, flattened: beats, [choice], reply, after-beats
int choiceAt = -1;
const int TOPW = 400, BOTW = 320, PW = 240;   // page width (screen height)

// rotate the page onto the screen (device turned 90 degrees to the left)
void pageBegin(int screenW) { C2D_ViewReset(); C2D_ViewTranslate((float)screenW, 0); C2D_ViewRotate(1.5707963f); }
void pageEnd() { C2D_ViewReset(); }
bool pageTouch(const Input& in, float& px, float& py) { if (!in.touch) return false; px = in.tp.py; py = (float)BOTW - in.tp.px; return true; }
struct PBtn { float x, y, w, h; bool in(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; } };

const Chapter& curChapter() { return bonus ? BONUS : ROUTES[route].ch[chap]; }
const char* whoName(int who) {
    if (who == 0) return "Kailyn";
    if (who >= 1 && who <= NUM_ROUTES) return ROUTES[who - 1].name;
    return nullptr;
}
void buildLines() {
    lines.clear(); choiceAt = -1;
    const Chapter& c = curChapter();
    for (const Beat& b : c.beats) if (b.text) lines.push_back({b.who, b.text});
    choiceAt = (int)lines.size();
    lines.push_back({-2, ""});   // the choice
}
void startChapter(int r, int c) {
    bonus = r < 0; route = r < 0 ? 0 : r; chap = c; pos = 0; picked = -1; gained = 0; typeT = 0;
    buildLines();
    phase = PH_PLAY;
}
void choose(int i) {
    const Chapter& c = curChapter();
    picked = i; gained = c.choice.aff[i];
    int who = route + 1;
    if (bonus) {   // at the cafe: your favourite answers, or the group, or Luca
        int best = 0; for (int k = 1; k < NUM_ROUTES; k++) if (G.oto.aff[k] > G.oto.aff[best]) best = k;
        who = i == 0 ? best + 1 : i == 1 ? 5 : 7;
    }
    lines.resize(choiceAt);
    lines.push_back({0, c.choice.opt[i]});
    lines.push_back({who, c.choice.reply[i]});
    for (const Beat& b : c.after) if (b.text) lines.push_back({b.who, b.text});
    pos = choiceAt; typeT = 0;
    if (!bonus) G.oto.aff[route] = (int8_t)std::max(-20, std::min(40, (int)G.oto.aff[route] + gained));
    phase = PH_PLAY;
    sfxTap();
}
int endingFor(int r) { int a = G.oto.aff[r]; return a >= 10 ? 0 : a >= 3 ? 1 : 2; }
void finishChapter() {
    if (bonus) { G.oto.bonus = 1; phase = PH_CHEND; return; }
    if (G.oto.ch[route] <= chap) G.oto.ch[route] = (uint8_t)(chap + 1);
    phase = PH_CHEND;
}
void advance() {
    if (phase == PH_CHEND) {
        if (bonus) { phase = PH_PICK; return; }
        if (chap + 1 < 4) startChapter(route, chap + 1);
        else { G.oto.ended[route] = (uint8_t)(endingFor(route) + 1); phase = PH_ENDING; sfxYay(); }
        return;
    }
    if (phase == PH_ENDING) { phase = PH_PICK; return; }
    if (phase != PH_PLAY) return;
    if (pos < (int)lines.size()) {
        size_t full = lines[pos].text.size();
        if (typeT * 60 < full) { typeT = full / 60.f + 1; return; }   // finish the typewriter first
    }
    pos++; typeT = 0;
    if (pos == choiceAt && picked < 0) { phase = PH_CHOICE; return; }
    if (pos >= (int)lines.size()) finishChapter();
}

// ------------------------------------------------------------------ backgrounds (page space)
void scene(int sc, float t, float H) {
    switch (sc) {
    case SC_LIBRARY:
        grad(0, 0, PW, H, C(70, 50, 70), C(120, 86, 96));
        for (int s = 0; s < 6; s++) { float y = 20 + s * 62; rect(0, y + 50, PW, 6, C(90, 60, 50)); for (int b = 0; b < 14; b++) { static const RGB BK[5] = {{200, 90, 110}, {110, 140, 190}, {230, 200, 120}, {140, 180, 140}, {190, 140, 200}}; rect(4 + b * 17.f, y + 18 + (b * 7 % 9), 13, 32 - (b * 7 % 9), C(BK[(b + s) % 5])); } }
        for (int i = 0; i < 3; i++) circle(40 + i * 80.f, 10, 18 + std::sin(t + i) * 2, C(255, 230, 170, 60));
        break;
    case SC_RAINY:
        grad(0, 0, PW, H, C(40, 50, 80), C(90, 90, 130));
        rect(30, 40, 180, 150, C(30, 40, 66)); outline(30, 40, 180, 150, 4, C(200, 180, 170));
        for (int i = 0; i < 40; i++) { float x = 32 + std::fmod(i * 37.f, 176.f), y = 40 + std::fmod(i * 53.f + t * 220, 150.f); line(x, y, x - 2, y + 8, 1, C(200, 220, 255, 140)); }
        for (int i = 0; i < 4; i++) circle(50 + i * 50.f, H - 60, 6 + std::sin(t * 3 + i), C(255, 200, 120, 160));
        break;
    case SC_LOFT:
        grad(0, 0, PW, H, C(40, 24, 60), C(90, 40, 100));
        rect(14, 30, 6, H - 60, C(255, 80, 180, 200)); rect(PW - 20, 30, 6, H - 60, C(80, 200, 255, 200));
        for (int i = 0; i < 6; i++) { static const RGB P[4] = {{255, 120, 200}, {80, 220, 230}, {255, 220, 90}, {170, 120, 255}}; circle(30 + i * 36.f, H - 40 + (i % 2) * 10, 10, C(P[i % 4], 200)); }
        rrect(150, 50, 70, 90, 4, C(240, 236, 226)); for (int i = 0; i < 5; i++) line(156, 60 + i * 16.f, 214, 66 + i * 14.f, 3, C(255, 100 + i * 30, 180, 200));
        break;
    case SC_WAREHOUSE:
        grad(0, 0, PW, H, C(36, 40, 50), C(70, 70, 80));
        for (int b = 0; b < 12; b++) for (int r = 0; r < 18; r++) rect((r % 2 ? 10 : 0) + b * 20.f, r * 22.f, 18, 20, C(90 + (b * r) % 20, 60, 56, 120));
        for (int i = 0; i < 30; i++) { float x = std::fmod(i * 41.f, PW), y = std::fmod(i * 67.f + t * 260, H); line(x, y, x - 3, y + 10, 1, C(180, 200, 230, 120)); }
        break;
    case SC_STARS:
        grad(0, 0, PW, H, C(20, 16, 60), C(90, 50, 120));
        for (int i = 0; i < 50; i++) sparkle(std::fmod(i * 47.f, PW), std::fmod(i * 83.f, H * 0.7f), 1 + std::fabs(std::sin(t * 2 + i)) * 2, C(255, 255, 255, 220));
        circle(190, 50, 20, C(255, 246, 210));
        for (int i = 0; i < 12; i++) rect(i * 20.f, H - 80 + (i * 13 % 30), 18, 80, C(30, 24, 60));
        for (int i = 0; i < 30; i++) rect(3 + std::fmod(i * 29.f, PW), H - 60 + std::fmod(i * 17.f, 50), 3, 3, C(255, 220, 120, 200));
        break;
    case SC_STADIUM:
        grad(0, 0, PW, H, C(20, 30, 70), C(40, 80, 60));
        for (int i = 0; i < 2; i++) { rect(20 + i * 180.f, 20, 20, 30, C(250, 250, 230)); for (int k = 0; k < 6; k++) circle(30 + i * 180.f, 35, 30 + k * 10, C(255, 255, 220, 14)); }
        rect(0, H * 0.62f, PW, H * 0.38f, C(60, 150, 80)); for (int i = 0; i < 6; i++) rect(0, H * 0.62f + i * 22, PW, 2, C(255, 255, 255, 120));
        break;
    case SC_VILLA:
        grad(0, 0, PW, H, C(255, 170, 120), C(255, 110, 140));
        circle(PW / 2, H * 0.45f, 40, C(255, 230, 140));
        rect(0, H * 0.5f, PW, H * 0.5f, C(80, 120, 180, 220)); for (int i = 0; i < 8; i++) rect(30 + i * 22.f, H * 0.5f + 6 + i * 8, 40, 2, C(255, 220, 160, 180));
        for (int s = -1; s <= 1; s += 2) { float x = s < 0 ? 0 : PW - 34; rect(x, 0, 34, H, C(255, 255, 255, 90)); }
        break;
    case SC_CAFE:
        grad(0, 0, PW, H, C(250, 220, 226), C(230, 190, 200));
        rect(0, H * 0.7f, PW, H * 0.3f, C(150, 100, 90));
        for (int i = 0; i < 3; i++) { rrect(20 + i * 74.f, 40, 52, 70, 6, C(200, 230, 250)); outline(20 + i * 74.f, 40, 52, 70, 3, C(255, 255, 255)); }
        for (int i = 0; i < 4; i++) circle(30 + i * 60.f, 18, 6, C(255, 220, 140, 200));
        break;
    case SC_STREET:
        grad(0, 0, PW, H, C(60, 40, 90), C(140, 80, 130));
        for (int i = 0; i < 6; i++) { float h = 120 + (i * 37 % 90); rect(i * 42.f, H - h - 40, 38, h, C(50, 34, 70)); for (int w = 0; w < 6; w++) rect(i * 42.f + 6 + (w % 2) * 16, H - h - 30 + (w / 2) * 22, 8, 10, C(255, 220, 140, 180)); }
        rect(0, H - 40, PW, 40, C(40, 30, 50));
        break;
    default:   // campus
        grad(0, 0, PW, H, C(170, 210, 250), C(250, 210, 230));
        rect(30, H * 0.35f, 180, H * 0.3f, C(230, 200, 190)); tri(20, H * 0.35f, 220, H * 0.35f, 120, H * 0.2f, C(190, 110, 110));
        for (int i = 0; i < 5; i++) rect(46 + i * 34.f, H * 0.42f, 16, 40, C(120, 150, 200));
        rect(0, H * 0.65f, PW, H * 0.35f, C(140, 200, 130));
        for (int i = 0; i < 6; i++) { ellipse(20 + i * 40.f, H * 0.66f, 22, 30, C(90, 160, 100)); }
        break;
    }
}
void hearts(float x, float y, int aff) {
    int n = aff >= 10 ? 5 : aff >= 7 ? 4 : aff >= 4 ? 3 : aff >= 1 ? 2 : aff >= -2 ? 1 : 0;
    for (int i = 0; i < 5; i++) heart(x + i * 14.f, y, 5, i < n ? C(255, 70, 150) : C(255, 255, 255, 90));
}
void pagePanel(float x, float y, float w, float h) { rrect(x, y, w, h, 10, C(255, 255, 255, 235)); outline(x + 3, y + 3, w - 6, h - 6, 1, C(255, 170, 210)); }
void pButton(const PBtn& b, const char* label, int style, float sc = 0.42f) {
    u32 face = style == 1 ? C(255, 80, 160) : style == 2 ? C(150, 110, 230) : C(255, 255, 255);
    u32 ink = style ? C(255, 255, 255) : C(110, 50, 110);
    rrect(b.x, b.y + 2, b.w, b.h, 8, C(110, 50, 110, 120));
    rrect(b.x, b.y, b.w, b.h, 8, face);
    auto ls = wrapText(label, sc, b.w - 14);
    float y0 = b.y + b.h / 2 - ls.size() * 7.5f - 1;
    for (size_t k = 0; k < ls.size() && k < 3; k++) text(ls[k].c_str(), b.x + b.w / 2, y0 + k * 15.f, sc, ink, 1);
}
void wing(float x, float y, float d, u32 c) { for (int k = 0; k < 3; k++) ellipse(x + d * (8 + k * 7), y - 4 + k * 4, 10 - k * 2, 5, c); }
void cupidHearts(float t, float cx, float cy) {
    // a winged heart pierced by cupid's arrow on each side of the logo, little hearts floating up
    for (int s = -1; s <= 1; s += 2) {
        float x = cx + s * 150, y = cy + std::sin(t * 2 + s) * 3;
        wing(x, y - 2, (float)s, C(255, 255, 255, 230));
        heart(x, y, 13, C(255, 70, 150));
        heart(x - 3, y - 3, 4, C(255, 200, 230));
        line(x - 22, y + 14, x + 22, y - 14, 2, C(200, 150, 60));
        tri(x + 22, y - 14, x + 14, y - 13, x + 20, y - 6, C(200, 150, 60));
        tri(x - 22, y + 14, x - 27, y + 9, x - 25, y + 17, C(255, 120, 190));
    }
    for (int i = 0; i < 10; i++) { float ph = std::fmod(t * 0.4f + i * 0.1f, 1.f); heart(cx - 180 + i * 40, cy + 40 - ph * 60, 4 + (i % 3), C(255, 120 + i * 10, 190, (uint8_t)(230 * (1 - ph)))); }
}
PBtn cardB(int i) { return PBtn{8 + (i % 2) * 116.f, 34 + (i / 2) * 64.f, 108, 58}; }
const PBtn bTown{8, 286, 100, 26}, bBonus{132, 286, 100, 26}, bChoice[3] = {{14, 80, 212, 54}, {14, 142, 212, 54}, {14, 204, 212, 54}};
}   // namespace

void otomeOpen() { phase = PH_FLIP; flipT = 0; }

bool otomeUpdate(const Input& in) {
    typeT += in.dt;
    if (phase == PH_FLIP) { flipT += in.dt; if (flipT > 1.6f || (in.down & (KEY_A | KEY_TOUCH))) phase = PH_PICK; return true; }
    float px = 0, py = 0;
    bool tap = pageTouch(in, px, py);
    if (in.down & KEY_START) return false;
    if (phase == PH_PICK) {
        if (in.down & KEY_B) return false;
        if (!tap) return true;
        if (bTown.in(px, py)) { sfxTap(); return false; }
        if (bBonus.in(px, py) && (G.oto.ended[0] | G.oto.ended[1] | G.oto.ended[2] | G.oto.ended[3] | G.oto.ended[4] | G.oto.ended[5] | G.oto.ended[6])) { sfxTap(); startChapter(-1, 0); return true; }
        for (int i = 0; i < NUM_ROUTES; i++) if (cardB(i).in(px, py)) {
            sfxTap();
            int c = G.oto.ch[i] >= 4 ? 0 : G.oto.ch[i];
            if (c == 0) G.oto.aff[i] = 0;   // replaying a route starts its feelings over
            startChapter(i, c);
            return true;
        }
        return true;
    }
    if (phase == PH_CHOICE) {
        if (in.down & KEY_B) { phase = PH_PICK; return true; }
        if (tap) for (int i = 0; i < 3; i++) if (bChoice[i].in(px, py)) { choose(i); break; }
        return true;
    }
    if (in.down & KEY_B) { phase = PH_PICK; return true; }
    if (tap && bTown.in(px, py)) { phase = PH_PICK; sfxTap(); return true; }
    if ((in.down & KEY_A) || tap) advance();
    return true;
}

// ------------------------------------------------------------------ left page: the scene & who's talking
void otomeDrawTop(float t) {
    if (phase == PH_FLIP) {
        grad(0, 0, TOPW, 240, C(255, 170, 210), C(190, 150, 240));
        for (int i = 0; i < 16; i++) sparkle(std::fmod(i * 61.f, 400.f), std::fmod(i * 37.f, 240.f), 2 + std::fabs(std::sin(t * 2 + i)) * 3, C(255, 255, 255, 220));
        cupidHearts(t, 200, 68);
        textOutline("Otome Ver.", 200, 46, 1.1f, C(255, 255, 255), C(150, 40, 120), 1, 1.7f);
        textOutline("~ Sugar Bunny Life 2 ~", 200, 96, 0.5f, C(255, 230, 250), C(150, 40, 120), 1, 1.0f);
        // a little 3DS turning on its side
        float a = std::fmin(1.f, flipT / 1.2f) * 1.5708f;
        C2D_ViewReset(); C2D_ViewTranslate(200, 170); C2D_ViewRotate(-a);
        rrect(-26, -34, 52, 68, 6, C(255, 255, 255)); rect(-20, -30, 40, 26, C(120, 200, 255)); rect(-17, 2, 34, 24, C(255, 170, 210));
        C2D_ViewReset();
        text("Turn your 3DS sideways like a book!", 200, 214, 0.5f, C(110, 40, 110), 1);
        return;
    }
    pageBegin(TOPW);
    const float H = TOPW;
    if (phase == PH_PICK) {
        grad(0, 0, PW, H, C(255, 214, 232), C(214, 196, 250));
        for (int i = 0; i < 18; i++) sparkle(std::fmod(i * 53.f, PW), std::fmod(i * 71.f, H), 2 + std::fabs(std::sin(t * 2 + i)) * 2, C(255, 255, 255, 220));
        for (int i = 0; i < 6; i++) heart(20 + std::fmod(i * 47.f, 200.f), 70 + i * 52.f, 6, C(255, 140, 200, 160));
        heart(22, 24, 7, C(255, 80, 160)); heart(PW - 22, 24, 7, C(255, 80, 160));
        textOutline("CHOOSE YOUR PATH", PW / 2, 12, 0.62f, C(255, 255, 255), C(130, 60, 140), 1, 1.2f);
        rrect(18, 46, 204, 290, 12, C(255, 240, 248, 220));
        ocOtomeDraw(0, 30, 54, 2.5f, 112, 8, 72);
        textOutline("MC: KAILYN", PW / 2, 346, 0.62f, C(255, 255, 255), C(130, 60, 140), 1, 1.2f);
        text("Sophomore, Starlight Bay University", PW / 2, 376, 0.36f, C(120, 60, 120), 1);
        pageEnd();
        return;
    }
    const Chapter& c = curChapter();
    scene(c.scene, t, H);
    int who = route + 1;
    if (phase == PH_PLAY && pos < (int)lines.size() && lines[pos].who >= 0 && lines[pos].who <= NUM_ROUTES) who = lines[pos].who;
    if (bonus && phase == PH_PLAY && (pos >= (int)lines.size() || lines[pos].who < 0)) who = 0;
    if (phase == PH_ENDING) who = endingFor(route) == 2 ? 0 : route + 1;
    // portrait: big bust, slightly bobbing
    ellipse(PW / 2, 352, 90, 16, C(0, 0, 0, 50));
    ocOtomeDraw(who, 24, 102 + std::sin(t * 1.4f) * 1.5f, 3.f, 84, 16, 64);
    rect(0, 352, PW, 48, C(255, 255, 255, 40));
    // header
    pagePanel(8, 8, PW - 16, 50);
    text(bonus ? "Otome Ver." : ROUTES[route].archetype, PW / 2, 12, 0.4f, C(150, 70, 150), 1);
    text(c.title, PW / 2, 32, 0.44f, C(90, 40, 100), 1);
    if (!bonus) { rrect(8, 64, PW - 16, 28, 8, C(90, 40, 100, 170)); text(ROUTES[route].name, 18, 68, 0.48f, C(255, 255, 255)); hearts(PW - 84, 78, G.oto.aff[route]); }
    const char* nm = phase == PH_PLAY && pos < (int)lines.size() ? whoName(lines[pos].who) : nullptr;
    if (nm) { rrect(14, 360, 120, 28, 8, C(255, 80, 160)); text(nm, 74, 365, 0.5f, C(255, 255, 255), 1); }
    pageEnd();
}

// ------------------------------------------------------------------ right page: words & choices
void otomeDrawBottom(float t) {
    if (phase == PH_FLIP) {
        grad(0, 0, BOTW, 240, C(255, 200, 226), C(220, 190, 250));
        text("Left page: story & portraits", 160, 80, 0.45f, C(110, 40, 110), 1);
        text("Right page: tap to read & choose", 160, 104, 0.45f, C(110, 40, 110), 1);
        text("START or the Town button: back to Starlight Bay", 160, 150, 0.38f, C(150, 70, 150), 1);
        return;
    }
    pageBegin(BOTW);
    const float H = BOTW;
    grad(0, 0, PW, H, C(255, 236, 246), C(236, 224, 252));
    for (int i = 0; i < 8; i++) heart(std::fmod(i * 67.f, PW), std::fmod(i * 43.f, H), 4, C(255, 180, 220, 90));
    if (phase == PH_PICK) {
        text("Who will you fall for?", PW / 2, 8, 0.48f, C(130, 60, 140), 1);
        for (int i = 0; i < NUM_ROUTES; i++) {
            PBtn b = cardB(i);
            rrect(b.x, b.y + 2, b.w, b.h, 8, C(130, 60, 140, 90));
            rrect(b.x, b.y, b.w, b.h, 8, C(255, 255, 255));
            ocOtomeDraw(i + 1, b.x + 3, b.y + 4, 1.2f, 41, 24, 40);
            char lb[40]; snprintf(lb, sizeof lb, "%d. %s", i + 1, ROUTES[i].archetype + 4);
            text(lb, b.x + 56, b.y + 6, 0.3f, C(110, 50, 110));
            text(ROUTES[i].name, b.x + 56, b.y + 22, 0.42f, C(255, 70, 150));
            uint8_t e = G.oto.ended[i];
            if (e) text(e == 1 ? "True Love!" : e == 2 ? "Friends" : "Red Flag", b.x + 56, b.y + 40, 0.3f, C(e == 1 ? RGB{230, 40, 120} : e == 2 ? RGB{90, 140, 200} : RGB{150, 120, 120}));
            else if (G.oto.ch[i]) { char cb[16]; snprintf(cb, sizeof cb, "Ch. %d/4", G.oto.ch[i] + 1); text(cb, b.x + 56, b.y + 40, 0.3f, C(150, 100, 160)); }
            else text("New!", b.x + 56, b.y + 40, 0.3f, C(150, 100, 160));
        }
        pButton(bTown, "Back to Town", 2, 0.38f);
        bool any = false; for (uint8_t e : G.oto.ended) any |= e != 0;
        pButton(bBonus, any ? "Bonus Chapter" : "Bonus (locked)", any ? 1 : 0, 0.36f);
        pageEnd();
        return;
    }
    if (phase == PH_CHOICE) {
        pagePanel(8, 8, PW - 16, 60);
        text("What do you do?", PW / 2, 14, 0.48f, C(130, 60, 140), 1);
        text("Your choice changes how he feels.", PW / 2, 40, 0.34f, C(150, 100, 160), 1);
        const Chapter& c = curChapter();
        for (int i = 0; i < 3; i++) pButton(bChoice[i], c.choice.opt[i], i == 0 ? 1 : 0, 0.4f);
        pageEnd();
        return;
    }
    if (phase == PH_CHEND) {
        pagePanel(14, 40, PW - 28, 200);
        textOutline(bonus ? "Bonus Complete!" : "Chapter Complete!", PW / 2, 56, 0.58f, C(255, 255, 255), C(200, 60, 140), 1, 1.1f);
        if (!bonus) {
            char b[48]; snprintf(b, sizeof b, gained > 0 ? "%s liked that. +%d" : gained < 0 ? "%s didn't like that. %d" : "%s is thinking...", ROUTES[route].name, gained);
            text(b, PW / 2, 100, 0.42f, C(110, 50, 110), 1);
            hearts(PW / 2 - 34, 140, G.oto.aff[route]);
            text(chap + 1 < 4 ? "Tap for the next chapter" : "Tap to see your ending...", PW / 2, 190, 0.4f, C(200, 60, 140), 1);
        } else text("You hold all the cards.", PW / 2, 120, 0.44f, C(110, 50, 110), 1);
        pButton(bTown, "Paths", 2, 0.38f);
        pageEnd();
        return;
    }
    if (phase == PH_ENDING) {
        int e = endingFor(route);
        pagePanel(10, 10, PW - 20, 268);
        textOutline(ROUTES[route].endTitle[e], PW / 2, 22, 0.5f, C(255, 255, 255), C(e == 0 ? RGB{220, 40, 120} : e == 1 ? RGB{80, 120, 200} : RGB{120, 90, 100}), 1, 1.0f);
        float y = 60;
        for (int k = 0; k < 3; k++) {
            auto ls = wrapText(ROUTES[route].end[e][k], 0.4f, PW - 44);
            for (auto& l : ls) { text(l.c_str(), 22, y, 0.4f, C(k == 2 ? RGB{200, 60, 140} : RGB{90, 50, 100})); y += 17; }
            y += 8;
        }
        if (e == 0) for (int i = 0; i < 6; i++) heart(30 + i * 36.f, 250 + std::sin(t * 3 + i) * 4, 7, C(255, 80, 160));
        pButton(bTown, "Paths", 2, 0.38f);
        pageEnd();
        return;
    }
    // reading
    pagePanel(8, 8, PW - 16, 268);
    if (pos < (int)lines.size()) {
        const Line& l = lines[pos];
        const char* nm = whoName(l.who);
        if (l.who == 8) nm = nullptr;
        if (nm) { text(nm, 22, 16, 0.5f, C(255, 70, 150)); }
        std::string s = l.text;
        if (l.who >= 0) s = "\"" + s + "\"";
        size_t shown = std::min(s.size(), (size_t)(typeT * 60));
        auto ls = wrapText(s.substr(0, shown), 0.46f, PW - 44);
        for (size_t k = 0; k < ls.size() && k < 12; k++) text(ls[k].c_str(), 22, (nm ? 44 : 22) + k * 19.f, 0.46f, C(l.who < 0 ? RGB{120, 80, 130} : RGB{70, 40, 80}));
        if (shown >= s.size()) tri(PW - 30, 258 + std::sin(t * 6) * 2, PW - 18, 258 + std::sin(t * 6) * 2, PW - 24, 266 + std::sin(t * 6) * 2, C(255, 80, 160));
    }
    pButton(bTown, "Paths", 2, 0.38f);
    char pg[24]; snprintf(pg, sizeof pg, "%d / %d", std::min(pos + 1, (int)lines.size()), (int)lines.size());
    text(pg, PW - 20, 292, 0.34f, C(150, 100, 160), 2);
    pageEnd();
}
