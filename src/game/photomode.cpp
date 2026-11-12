#include "photomode.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "tween.hpp"
#include "growup.hpp"
#include "extras.hpp"
#include <cstdio>
#include <cmath>
#include <sys/stat.h>
#include <vector>
#include <string>
using namespace col;

bool g_photoClean = false;
static int sel = 0, filt = 0, frame = 0, stick = 0, pendingSnap = 0, shots = 0;
static std::vector<int> poseOf;   // per actor slot, -1 = none
static float flash = 0;
static const int POSES[] = {-1, TW_WAVE, TW_CHEER, TW_ARMSUP, TW_CLAP, TW_LEANL, TW_LEANR, TW_BOW, TW_KNEEL, TW_SWAY, TW_HOP, TW_SPIN, TW_STOMP};
static const char* POSE_N[] = {"Natural", "Wave", "Cheer", "Arms Up", "Clap", "Lean Left", "Lean Right", "Bow", "Kneel", "Sway", "Hop", "Spin", "Stomp"};
constexpr int NPOSE = sizeof POSES / sizeof POSES[0];
static const char* FILT_N[6] = {"None", "Y2K Pink", "Sepia", "B&W", "Disposable Cam", "Dreamy Blue"};
static const char* FRAME_N[5] = {"None", "Polaroid", "Hearts", "Sparkle", "Film Strip"};
static const char* STICK_N[5] = {"None", "Stars", "Butterflies", "XOXO", "Date Stamp"};

static std::vector<Actor*> sims() { std::vector<Actor*> v; for (auto& a : curRoom().actors) if (a.kind == 0 && a.id < G.sims.size()) v.push_back(&a); return v; }

void photoOpen() { g_photoClean = true; sel = 0; poseOf.assign(32, 0); toast("Photo Mode! Pose everyone, then tap SNAP."); }

static Btn B(int c, int r) { return Btn{6.f + c * 103, 30.f + r * 34, 98, 30}; }

bool photoUpdate(const Input& in) {
    auto v = sims(); if (poseOf.size() < v.size()) poseOf.resize(v.size(), 0);
    if (!v.empty()) sel = (sel % (int)v.size() + (int)v.size()) % (int)v.size();
    for (size_t i = 0; i < v.size(); i++) if (POSES[poseOf[i] % NPOSE] >= 0) tweenLoop(v[i]->id, POSES[poseOf[i] % NPOSE]);
    flash = std::max(0.f, flash - in.dt * 3); tweenTick(in.dt);
    const touchPosition tp = in.tp;
    auto hit = [&](int c, int r) { return in.touch && B(c, r).in(tp); };
    if ((in.down & KEY_B) || (in.touch && Btn{6, 204, 98, 30}.in(tp))) { g_photoClean = false; return false; }
    if (in.down & (KEY_L)) sel--; if (in.down & KEY_R) sel++;
    if (hit(0, 0)) sel--; if (hit(2, 0)) sel++;
    if (v.empty()) sel = 0; else sel = (sel % (int)v.size() + (int)v.size()) % (int)v.size();   // wrap BEFORE using it (sel-- could make it -1)
    if (!v.empty() && sel < (int)poseOf.size()) {
        Actor* a = v[sel];
        if (in.held & KEY_DLEFT) a->x = std::max(20.f, a->x - 90 * in.dt), a->tx = a->x;
        if (in.held & KEY_DRIGHT) a->x = std::min(380.f, a->x + 90 * in.dt), a->tx = a->x;
        if (hit(0, 1) || (in.down & KEY_DUP)) poseOf[sel] = (poseOf[sel] + NPOSE - 1) % NPOSE;
        if (hit(2, 1) || (in.down & KEY_DDOWN)) poseOf[sel] = (poseOf[sel] + 1) % NPOSE;
    }
    if (hit(0, 2)) filt = (filt + 1) % 6;
    if (hit(1, 2)) frame = (frame + 1) % 5;
    if (hit(2, 2)) stick = (stick + 1) % 5;
    if (hit(0, 3)) for (size_t i = 0; i < poseOf.size(); i++) poseOf[i] = 1 + (int)((i * 3 + G.day) % (NPOSE - 1));   // everyone pose!
    if (hit(1, 3)) for (auto& p : poseOf) p = 0;
    if ((in.down & KEY_A) || (in.touch && Btn{110, 204, 204, 30}.in(tp))) { pendingSnap = 2; sfxYay(); }
    return true;
}

void photoDrawOverlay(float t) {
    // filter
    switch (filt) {
    case 1: rect(0, 0, TOP_W, TOP_H, C(255, 120, 190, 55)); break;
    case 2: rect(0, 0, TOP_W, TOP_H, C(150, 100, 50, 80)); break;
    case 3: rect(0, 0, TOP_W, TOP_H, C(128, 128, 128, 140)); break;
    case 4: rect(0, 0, TOP_W, TOP_H, C(255, 200, 120, 45)); for (int i = 0; i < 4; i++) rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, 0)); rect(0, 0, 40, TOP_H, C(255, 120, 40, 40)); break;
    case 5: rect(0, 0, TOP_W, TOP_H, C(120, 170, 255, 60)); rect(0, 0, TOP_W, 50, C(255, 255, 255, 50)); break;
    }
    // stickers
    if (stick == 1) for (int i = 0; i < 9; i++) { const float x = 30 + i * 43.f, y = 20 + (i * 37 % 60); rect(x - 1, y - 5, 2, 10, C(255, 230, 120)); rect(x - 5, y - 1, 10, 2, C(255, 230, 120)); }
    if (stick == 2) for (int i = 0; i < 5; i++) { const float x = 40 + i * 80.f, y = 30 + (i % 2) * 20; circle(x - 4, y, 5, C(170, 200, 255)); circle(x + 4, y, 5, C(255, 170, 220)); rect(x - 1, y - 5, 2, 10, C(60, 40, 60)); }
    if (stick == 3) { textOutline("XOXO", 340, 190, 0.8f, C(255, 90, 170), C(255, 255, 255), 1); }
    if (stick == 4) { char b[24]; snprintf(b, sizeof b, "'0%d  %02d  %02d", 3 + G.day / 28 % 7, 1 + G.day / 7 % 12, 1 + G.day % 28); text(b, 386, 220, 0.5f, C(255, 150, 40), 2); }
    // frame
    if (frame == 1) { rect(0, 0, TOP_W, 8, C(250, 250, 245)); rect(0, 0, 8, TOP_H, C(250, 250, 245)); rect(TOP_W - 8, 0, 8, TOP_H, C(250, 250, 245)); rect(0, TOP_H - 28, TOP_W, 28, C(250, 250, 245)); text(MC().name.c_str(), 200, TOP_H - 22, 0.5f, C(80, 60, 70), 1); }
    if (frame == 2) for (int i = 0; i < 20; i++) { const float x = i * 21.f + 10; circle(x - 3, 6, 4, C(255, 90, 150)); circle(x + 3, 6, 4, C(255, 90, 150)); circle(x - 3, TOP_H - 6, 4, C(255, 90, 150)); circle(x + 3, TOP_H - 6, 4, C(255, 90, 150)); }
    if (frame == 3) for (int i = 0; i < 30; i++) { const float x = std::fmod(i * 53.f, (float)TOP_W), y = i % 2 ? 6 : TOP_H - 6; const float s = 2 + std::sin(t * 4 + i) * 1.5f; rect(x - s, y - 0.5f, s * 2, 1, C(255, 255, 255)); rect(x - 0.5f, y - s, 1, s * 2, C(255, 255, 255)); }
    if (frame == 4) { rect(0, 0, TOP_W, 14, C(20, 20, 20)); rect(0, TOP_H - 14, TOP_W, 14, C(20, 20, 20)); for (int i = 0; i < 25; i++) { rect(4 + i * 16.f, 4, 8, 6, C(240, 240, 240)); rect(4 + i * 16.f, TOP_H - 10, 8, 6, C(240, 240, 240)); } }
    if (flash > 0) rect(0, 0, TOP_W, TOP_H, C(255, 255, 255, (int)(flash * 255)));
}

void photoDrawBottom(float t) {
    (void)t; auto v = sims();
    if (poseOf.size() < v.size()) poseOf.resize(v.size(), 0);
    if (v.empty() || sel < 0 || sel >= (int)v.size()) sel = 0;
    rect(0, 0, 320, 240, C(40, 30, 50));
    text("PHOTO MODE", 160, 6, 0.55f, C(255, 160, 210), 1);
    auto btn = [&](Btn b, const std::string& s, RGB c) { rrect(b.x, b.y, b.w, b.h, 8, C(c)); text(s.c_str(), b.x + b.w / 2, b.y + 9, 0.36f, C(WHITE), 1); };
    std::string who = v.empty() ? "(nobody here)" : G.sims[v[sel]->id].name.substr(0, G.sims[v[sel]->id].name.find(' '));
    btn(B(0, 0), "< Prev", {90, 70, 120}); btn(B(1, 0), who, {150, 90, 160}); btn(B(2, 0), "Next >", {90, 70, 120});
    btn(B(0, 1), "< Pose", {90, 70, 120}); btn(B(1, 1), v.empty() ? "-" : POSE_N[poseOf[sel] % NPOSE], {200, 100, 150}); btn(B(2, 1), "Pose >", {90, 70, 120});
    btn(B(0, 2), std::string("Filter: ") + FILT_N[filt], {70, 120, 170}); btn(B(1, 2), std::string("Frame: ") + FRAME_N[frame], {70, 150, 130}); btn(B(2, 2), std::string("Sticker: ") + STICK_N[stick], {170, 120, 60});
    btn(B(0, 3), "Everyone Pose!", {220, 90, 140}); btn(B(1, 3), "Reset Poses", {110, 100, 120}); btn(B(2, 3), std::to_string(shots) + " photos", {60, 60, 80});
    text("D-pad L/R: move  Up/Down: pose  L/R: pick", 160, 170, 0.32f, C(220, 200, 230), 1);
    btn(Btn{6, 204, 98, 30}, "Done", {110, 90, 110}); btn(Btn{110, 204, 204, 30}, "SNAP! (A)", {255, 90, 160});
}

void photoAfterFrame() {
    if (!pendingSnap) return;
    if (--pendingSnap) return;   // wait one frame so the captured image is the clean, finished one
#ifdef __3DS__
    u16 fw = 0, fh = 0; u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &fw, &fh);   // 240 x 400, rotated, BGR8
    if (fb && fw >= 240 && fh >= 400 && gfxGetScreenFormat(GFX_TOP) == GSP_BGR8_OES) {
        mkdir("sdmc:/LifeSim", 0777); mkdir("sdmc:/LifeSim/photos", 0777);
        char path[96]; int n = 0; FILE* f = nullptr;
        for (n = 1; n < 10000; n++) { snprintf(path, sizeof path, "sdmc:/LifeSim/photos/photo_%04d.bmp", n); FILE* t = fopen(path, "rb"); if (!t) break; fclose(t); }
        f = fopen(path, "wb");
        if (f) {
            const int W = 400, H = 240, row = W * 3; const uint32_t sz = 54 + row * H;
            uint8_t h[54] = {'B', 'M'}; auto u32w = [&](int o, uint32_t v) { h[o] = v; h[o + 1] = v >> 8; h[o + 2] = v >> 16; h[o + 3] = v >> 24; };
            u32w(2, sz); u32w(10, 54); u32w(14, 40); u32w(18, W); u32w(22, H); h[26] = 1; h[28] = 24; u32w(34, row * H);
            fwrite(h, 1, 54, f);
            static uint8_t line[400 * 3];
            for (int y = H - 1; y >= 0; y--) {   // BMP rows are bottom-up; framebuffer pixel (x,y) lives at (x*240 + 239-y)
                for (int x = 0; x < W; x++) { const uint8_t* p = fb + (x * 240 + (239 - y)) * 3; line[x * 3] = p[0]; line[x * 3 + 1] = p[1]; line[x * 3 + 2] = p[2]; }
                fwrite(line, 1, row, f);
            }
            fclose(f); shots++;
            char cap[40]; snprintf(cap, sizeof cap, "Photo %04d (SD: LifeSim/photos)", n); guPhoto(cap);
            toast("Saved photo_" + std::to_string(n) + ".bmp to your SD card!");
        } else toast("Couldn't save the photo - check the SD card.");
    }
#else
    shots++; guPhoto("Photo (host build)");
#endif
    flash = 1;
}
