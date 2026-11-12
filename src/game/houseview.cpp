#include "houseview.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include <vector>
#include <cmath>
using namespace col;

namespace {
int sel = 0;
std::vector<int> rooms() {
    const int ht = G.houseType[G.house % 2];
    if (ht == 0) return {R_BEDROOM, R_BATHROOM, R_CLOSET, R_LIVING, R_KITCHEN, R_STUDY};   // apartment: one floor + a loft
    if (ht == 1) return {R_BEDROOM, R_BEDROOM2, R_NURSERY, R_BATHROOM, R_CLOSET, R_STUDY, R_LIVING, R_KITCHEN, R_YARD};
    return {R_BEDROOM, R_BEDROOM2, R_TEENROOM, R_NURSERY, R_BATHROOM, R_CLOSET, R_LIVING, R_KITCHEN, R_STUDY, R_BASEMENT, R_YARD};
}
const float TOPY = 34, BW = 100, GAP = 4;
float BH = 50;
Btn box(int i) { const int r = i / 3, c = i % 3; return Btn{6 + c * (BW + 4), TOPY + r * (BH + GAP), BW, BH}; }
void layout(int n) { const int rowsN = (n + 2) / 3; BH = std::min(56.f, (BOT_H - TOPY - 6 - rowsN * GAP) / rowsN); }
void miniRoom(int room, float x, float y, float w, float h, float t) {
    const int keep = G.room; G.room = room;
    const float s = std::min(w / TOP_W, h / TOP_H);
    C3D_Mtx m; C2D_ViewSave(&m);
    C2D_ViewTranslate(x + (w - TOP_W * s) / 2, y + (h - TOP_H * s) / 2); C2D_ViewScale(s, s);
    drawRoom(G, t, -1, -1, false);
    C2D_ViewRestore(&m);
    G.room = keep;
}
}

void houseViewOpen() { auto v = rooms(); sel = 0; for (size_t i = 0; i < v.size(); i++) if (v[i] == G.room) sel = (int)i; }

bool houseViewUpdate(const Input& in) {
    auto v = rooms(); layout((int)v.size());
    if (in.down & KEY_B) return false;
    if (in.down & KEY_DRIGHT) sel = (sel + 1) % (int)v.size();
    if (in.down & KEY_DLEFT) sel = (sel + (int)v.size() - 1) % (int)v.size();
    if (in.down & KEY_DDOWN) sel = std::min((int)v.size() - 1, sel + 3);
    if (in.down & KEY_DUP) sel = std::max(0, sel - 3);
    int go = -1;
    if (in.down & KEY_A) go = sel;
    if (in.touch) { for (size_t i = 0; i < v.size(); i++) if (box((int)i).in(in.tp)) { if ((int)i == sel) go = (int)i; else { sel = (int)i; sfxTap(); } } }
    if (in.touch && Btn{228, 4, 88, 24}.in(in.tp)) return false;
    if (go >= 0) { sfxTap(); if (v[go] != G.room) travelTo(v[go]); return false; }
    return true;
}

void houseViewDrawTop(float t) {
    auto v = rooms(); const int r = v[sel % v.size()];
    const int keep = G.room; G.room = r; drawRoom(G, t, -1, -1, false); G.room = keep;
    rrect(110, 4, 180, 22, 10, C(UI_INK, 200)); text(ROOM_NAMES[r], 200, 7, 0.5f, C(WHITE), 1);
    int n = 0; for (const Actor& a : G.rooms[G.house][r].actors) if (a.kind == 0) n++;
    char b[40]; snprintf(b, sizeof b, n ? "%d inside  -  tap again / A to go in" : "Empty  -  tap again / A to go in", n); text(b, 200, 222, 0.38f, C(WHITE), 1);
}

void houseViewDrawBottom(float t) {
    auto v = rooms(); const bool apt = G.houseType[G.house % 2] == 0; layout((int)v.size());
    grad(0, 0, BOT_W, BOT_H, C(20, 24, 70), C(40, 30, 90));
    for (int i = 0; i < 30; i++) { const float x = std::fmod(i * 67.f, 320.f), y = std::fmod(i * 23.f, 30.f) + 2; const float a = 0.5f + 0.5f * std::sin(t * 2 + i); circle(x, y, 1 + a * 0.6f, C(255, 255, 200, (uint8_t)(120 + a * 135))); }
    const int rowsN = ((int)v.size() + 2) / 3; const float bottomY = TOPY + rowsN * (BH + GAP);
    // the house shell
    if (apt) { rect(2, TOPY - 10, 316, 8, C(170, 160, 190)); for (int i = 0; i < 12; i++) rect(8 + i * 26.f, TOPY - 26, 18, 16, C(i % 3 ? RGB{60, 60, 110} : RGB{255, 220, 120})); }
    else { tri(0, TOPY - 2, 160, 2, 320, TOPY - 2, C(230, 90, 100)); tri(14, TOPY - 2, 160, 10, 306, TOPY - 2, C(255, 225, 200)); circle(160, TOPY - 13, 7, C(30, 40, 110)); circle(160, TOPY - 13, 5, C(70, 90, 180)); }
    rect(2, TOPY - 3, 316, bottomY - TOPY + 3, C(250, 245, 250));
    rect(0, bottomY - 2, BOT_W, BOT_H, C(120, 190, 110));
    for (size_t i = 0; i < v.size(); i++) {
        Btn b = box((int)i); const bool on = (int)i == sel;
        rect(b.x - 1, b.y - 1, b.w + 2, b.h + 2, C(on ? RGB{255, 80, 170} : RGB{90, 80, 120}));
        rect(b.x, b.y, b.w, b.h, C(245, 230, 240));
        miniRoom(v[i], b.x, b.y, b.w, b.h, t);
        { const RGB fr = on ? RGB{255, 80, 170} : RGB{250, 245, 250}; rect(b.x - 4, b.y - 1, 4, b.h + 2, C(fr)); rect(b.x + b.w, b.y - 1, 4, b.h + 2, C(fr)); rect(b.x - 1, b.y + b.h, b.w + 2, 3, C(fr)); }   // trim hides any spill
        if (G.room == v[i]) text("YOU", b.x + b.w - 3, b.y + 1, 0.28f, C(255, 80, 170), 2);
        rrect(b.x + 2, b.y + b.h - 11, std::min(b.w - 4, textW(ROOM_NAMES[v[i]], 0.28f) + 6), 10, 3, C(UI_INK, 170));
        text(ROOM_NAMES[v[i]], b.x + 5, b.y + b.h - 11, 0.28f, C(WHITE));
    }
    button(Btn{228, 4, 88, 24}, "Back (B)", false, 1, 0.36f);
    text(apt ? "Your apartment" : "Your house", 8, 8, 0.4f, C(WHITE));
}
