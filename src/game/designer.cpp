#include "designer.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "emote.hpp"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include "vfs.hpp"
#define fopen vfopen
using namespace col;

namespace {
const RGB PAL[16] = {{0, 0, 0}, {255, 255, 255}, {30, 28, 34}, {255, 60, 150}, {255, 182, 210}, {225, 35, 50}, {255, 150, 40}, {255, 225, 60},
                     {170, 230, 60}, {40, 160, 80}, {120, 200, 255}, {40, 80, 200}, {200, 170, 255}, {130, 60, 190}, {130, 80, 45}, {215, 175, 70}};
struct DS { uint32_t magic = 0x44533031; uint8_t used[DS_SLOTS] = {}; uint8_t px[DS_SLOTS][DS_N * DS_N] = {}; uint32_t ver[DS_SLOTS] = {}; };
DS D;
bool editing = false; int slot = 0, colour = 3, tool = 0; bool mirror = false; int pickSlot = -1;   // tool 0 pen, 1 fill, 2 eraser
uint8_t undo[DS_N * DS_N]; bool painting = false;
const float GX = 4, GY = 22, CELL = 6;   // canvas on the touch screen (192 x 192)
Btn palB(int i) { return Btn{204.f + (i % 4) * 28, 22.f + (i / 4) * 28, 26, 26}; }
Btn toolB(int i) { return Btn{204.f + (i % 2) * 56, 138.f + (i / 2) * 24, 54, 22}; }
const char* TOOL_N[6] = {"Pen", "Fill", "Eraser", "Mirror", "Undo", "Template"};
void setPx(int x, int y, uint8_t c) { if (x < 0 || y < 0 || x >= DS_N || y >= DS_N) return; D.px[slot][y * DS_N + x] = c; if (mirror) D.px[slot][y * DS_N + (DS_N - 1 - x)] = c; }
void fill(int x, int y, uint8_t c) {
    const uint8_t from = D.px[slot][y * DS_N + x]; if (from == c) return;
    std::vector<int> st{y * DS_N + x};
    while (!st.empty()) { const int p = st.back(); st.pop_back(); if (D.px[slot][p] != from) continue; D.px[slot][p] = c; const int px = p % DS_N, py = p / DS_N;
        if (px > 0) st.push_back(p - 1); if (px < DS_N - 1) st.push_back(p + 1); if (py > 0) st.push_back(p - DS_N); if (py < DS_N - 1) st.push_back(p + DS_N); }
}
void templ(int k) {
    uint8_t* p = D.px[slot];
    for (int y = 0; y < DS_N; y++) for (int x = 0; x < DS_N; x++) {
        uint8_t c = 0;
        switch (k) {
        case 0: c = (y / 4) % 2 ? 1 : 3; break;                                   // stripes
        case 1: c = ((x / 4) + (y / 4)) % 2 ? 1 : 2; break;                       // checker
        case 2: { const float hx = (x % 16) - 7.5f, hy = (y % 16) - 6.f; const float v = hx * hx * 0.04f + (hy * 0.18f - std::sqrt(std::fabs(hx)) * 0.32f) * (hy * 0.18f - std::sqrt(std::fabs(hx)) * 0.32f); c = v < 1 ? 3 : 4; break; }   // hearts
        case 3: c = ((x + y) % 8 < 2 || (x - y + 64) % 8 < 2) ? 13 : 12; break;     // argyle-ish
        case 4: c = (x * 7 + y * 13) % 11 == 0 ? 15 : 2; break;                     // starry night
        case 5: c = 0; break;                                                       // blank
        }
        p[y * DS_N + x] = c;
    }
}
int tpl = 0;
}

bool dsPixel(int s, float u, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (s < 0 || s >= DS_SLOTS || !D.used[s]) return false;
    const int x = std::max(0, std::min(DS_N - 1, (int)(u * DS_N))), y = std::max(0, std::min(DS_N - 1, (int)(v * DS_N)));
    const uint8_t c = D.px[s][y * DS_N + x]; if (!c) return false;
    r = PAL[c].r; g = PAL[c].g; b = PAL[c].b; return true;
}
uint32_t dsVersion(int s) { return s >= 0 && s < DS_SLOTS ? D.ver[s] : 0; }
bool dsEditing() { return editing; }

LxMenu dsApp() {
    LxMenu m;
    for (int k = 0; k < DS_SLOTS; k++) m.push_back({D.used[k] ? "Design " + std::to_string(k + 1) : "(empty) New Design " + std::to_string(k + 1), 25100 + k});
    m.push_back({"Take Off Design (you)", 25050});
    return m;
}

LxMenu dsOption(int id) {
    LxMenu m;
    if (id >= 25100 && id < 25100 + DS_SLOTS) {
        const int k = id - 25100; pickSlot = k;
        if (!D.used[k]) { slot = k; D.used[k] = 1; memset(D.px[k], 0, sizeof D.px[k]); tpl = 0; templ(5); editing = true; return {}; }
        m.push_back({"Edit Design " + std::to_string(k + 1), 25200 + k}); m.push_back({"Wear It", 25300 + k}); m.push_back({"Give to a Household Member", 25400 + k}); m.push_back({"Delete", 25500 + k}); return m;
    }
    if (id >= 25200 && id < 25200 + DS_SLOTS) { slot = id - 25200; editing = true; return {}; }
    if (id >= 25300 && id < 25300 + DS_SLOTS) { MC().look.topPattern = (uint8_t)(100 + id - 25300); emotePlay(G.mc, EMO_HAPPY, "Custom!"); toast("You're wearing your own design!"); return {}; }
    if (id >= 25400 && id < 25400 + DS_SLOTS) { pickSlot = id - 25400; for (int i = 0; i < (int)G.sims.size() && i < 99; i++) if (G.sims[i].household) m.push_back({"Dress " + G.sims[i].name, 25600 + i}); return m; }
    if (id >= 25500 && id < 25500 + DS_SLOTS) { const int k = id - 25500; D.used[k] = 0; D.ver[k]++; for (Sim& s : G.sims) if (s.look.topPattern == 100 + k) s.look.topPattern = 0; toast("Design deleted."); return {}; }
    if (id >= 25600 && id < 25700 && pickSlot >= 0 && id - 25600 < (int)G.sims.size()) { G.sims[id - 25600].look.topPattern = (uint8_t)(100 + pickSlot); emotePlay(id - 25600, EMO_LOVE, "Cute!"); toast(G.sims[id - 25600].name.substr(0, G.sims[id - 25600].name.find(' ')) + " loves the new design!"); return {}; }
    if (id == 25050) { if (MC().look.topPattern >= 100) MC().look.topPattern = 0; toast("Back to your regular outfit."); return {}; }
    return m;
}

bool dsUpdate(const Input& in) {
    if (!editing) return false;
    const touchPosition tp = in.tp;
    if (in.down & KEY_B) { editing = false; D.ver[slot]++; toast("Design saved! Wear it from the Design Studio app."); return false; }
    if (in.touch || (in.held & KEY_TOUCH)) {
        const float fx = (tp.px - GX) / CELL, fy = (tp.py - GY) / CELL;
        if (fx >= 0 && fy >= 0 && fx < DS_N && fy < DS_N) {
            if (!painting) { memcpy(undo, D.px[slot], sizeof undo); painting = true; }
            const int x = (int)fx, y = (int)fy;
            if (tool == 1) { if (in.touch) { fill(x, y, (uint8_t)colour); if (mirror) fill(DS_N - 1 - x, y, (uint8_t)colour); } }
            else setPx(x, y, tool == 2 ? 0 : (uint8_t)colour);
            D.ver[slot]++;
            return true;
        }
    } else painting = false;
    if (in.touch) {
        for (int i = 0; i < 16; i++) if (palB(i).in(tp)) { colour = i; if (tool == 2) tool = 0; sfxTap(); }
        for (int i = 0; i < 6; i++) if (toolB(i).in(tp)) {
            sfxTap();
            if (i <= 2) tool = i; else if (i == 3) mirror = !mirror; else if (i == 4) { memcpy(D.px[slot], undo, sizeof undo); D.ver[slot]++; }
            else { memcpy(undo, D.px[slot], sizeof undo); templ(tpl); tpl = (tpl + 1) % 6; D.ver[slot]++; }
        }
        if (Btn{204, 212, 110, 24}.in(tp)) { editing = false; D.ver[slot]++; toast("Design saved! Wear it from the Design Studio app."); return false; }
    }
    if (in.down & KEY_L) colour = (colour + 15) % 16; if (in.down & KEY_R) colour = (colour + 1) % 16;
    return true;
}

static void shirt(float cx, float cy, float s) {   // a flat tee showing the design, like a Tomodachi/New Leaf preview
    const float w = 64 * s, h = 70 * s, x0 = cx - w / 2, y0 = cy - h / 2;
    for (int y = 0; y < DS_N; y++) for (int x = 0; x < DS_N; x++) {
        const uint8_t c = D.px[slot][y * DS_N + x]; const RGB col = c ? PAL[c] : RGB{250, 248, 250};
        rect(x0 + x * w / DS_N, y0 + y * h / DS_N, w / DS_N + 0.6f, h / DS_N + 0.6f, C(col));
    }
    tri(x0 - 22 * s, y0 + 6 * s, x0, y0, x0, y0 + 26 * s, C(235, 230, 240)); tri(x0 + w + 22 * s, y0 + 6 * s, x0 + w, y0, x0 + w, y0 + 26 * s, C(235, 230, 240));
    circle(cx, y0, 9 * s, C(60, 50, 80));
}
void dsDrawTop(float t) {
    (void)t; grad(0, 0, TOP_W, TOP_H, C(255, 225, 240), C(220, 200, 255));
    for (int i = 0; i < 20; i++) circle(std::fmod(i * 53.f, 400.f), std::fmod(i * 31.f, 240.f), 2, C(255, 255, 255, 120));
    textOutline("DESIGN STUDIO", 200, 6, 0.8f, C(255, 90, 170), C(WHITE), 1);
    shirt(120, 128, 1.6f);
    drawSimPortrait(MC(), 300, 128, 52, t);
    char b[48]; snprintf(b, sizeof b, "Design %d  -  B or Done to save", slot + 1); text(b, 200, 222, 0.4f, C(80, 50, 90), 1);
}
void dsDrawBottom(float t) {
    (void)t; rect(0, 0, 320, 240, C(60, 46, 80));
    text("Draw with the stylus!", 100, 4, 0.4f, C(255, 200, 230), 1);
    for (int y = 0; y < DS_N; y++) for (int x = 0; x < DS_N; x++) {
        const uint8_t c = D.px[slot][y * DS_N + x];
        const RGB col = c ? PAL[c] : ((x + y) % 2 ? RGB{235, 232, 240} : RGB{220, 216, 228});
        rect(GX + x * CELL, GY + y * CELL, CELL, CELL, C(col));
    }
    if (mirror) rect(GX + DS_N * CELL / 2 - 0.5f, GY, 1, DS_N * CELL, C(255, 90, 170, 160));
    for (int i = 0; i < 16; i++) { Btn b = palB(i); rect(b.x - 1, b.y - 1, b.w + 2, b.h + 2, C(i == colour ? RGB{255, 255, 255} : RGB{40, 30, 50}));
        if (i == 0) { rect(b.x, b.y, b.w, b.h, C(220, 216, 228)); text("base", b.x + b.w / 2, b.y + 8, 0.26f, C(90, 80, 100), 1); } else rect(b.x, b.y, b.w, b.h, C(PAL[i])); }
    for (int i = 0; i < 6; i++) button(toolB(i), TOOL_N[i], (i <= 2 && tool == i) || (i == 3 && mirror), i <= 2 ? 0 : 1, 0.32f);
    button(Btn{204, 212, 110, 24}, "Done & Save", false, 2, 0.38f);
}

void dsSaveTo(const std::string& p) { FILE* f = fopen(p.c_str(), "wb"); if (!f) return; fwrite(&D, sizeof D, 1, f); fclose(f); }
void dsLoadFrom(const std::string& p) { D = DS{}; FILE* f = fopen(p.c_str(), "rb"); if (!f) return; DS d; if (fread(&d, 1, sizeof d, f) == sizeof d && d.magic == D.magic) D = d; fclose(f); }
