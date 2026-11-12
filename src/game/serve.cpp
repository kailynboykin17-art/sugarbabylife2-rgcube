// Dinner Rush: a waitressing minigame at the Sunny Side Diner (bottom screen, touch).
// Seat customers (tap them in line, then tap a free table), take their order when they
// raise a hand, grab plates from the kitchen window, serve, collect the bill (tips depend
// on how many hearts they have left), and wipe the table. Hearts drain while people wait.
#include "serve.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "goals.hpp"
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>

using namespace col;

namespace {
enum CState { C_LINE, C_MENU, C_ORDER, C_FOOD, C_EAT, C_PAY, C_GONE };
struct Cust { int sim; int state; int table; float t; float hearts; float x, y; bool angry; };
struct Table { float x, y; int cust; bool dirty; };
struct Order { int table; float left; bool ready; };
struct Act { int kind; int arg; };   // 0 table, 1 kitchen
const float WIN_X = 288, WIN_Y = 62, LINE_X = 286, LINE_Y = 196;
std::vector<Cust> cs;
Table tables[4] = {{64, 92, -1, false}, {170, 92, -1, false}, {64, 176, -1, false}, {170, 176, -1, false}};
std::vector<Order> orders;
std::vector<Act> queue;
std::vector<int> hands;       // tables whose plates we're carrying (max 2)
float px = 240, py = 130, shiftLeft = 0, spawnT = 0, busy = 0, endT = 0;
int phase = 0, served = 0, angry = 0, tips = 0, spawned = 0, sel = -1, combo = 0;
std::string g_msg; float msgT = 0;

void msg(const std::string& m) { g_msg = m; msgT = 1.6f; }
int lineCount() { int n = 0; for (auto& c : cs) if (c.state == C_LINE) n++; return n; }
void spawn() {
    std::vector<int> pool;
    for (size_t i = 0; i < G.sims.size(); i++) if (!G.sims[i].household && G.sims[i].stage >= ST_CHILD) pool.push_back((int)i);
    if (pool.empty()) return;
    Cust c; c.sim = pool[irand((int)pool.size())]; c.state = C_LINE; c.table = -1; c.t = 0; c.hearts = 5; c.angry = false;
    c.x = LINE_X; c.y = LINE_Y - lineCount() * 28.f;
    cs.push_back(c); spawned++;
}
void leave(Cust& c, bool mad) {
    if (c.table >= 0) { tables[c.table].cust = -1; if (!mad) tables[c.table].dirty = true; }
    c.state = C_GONE;
    if (mad) { angry++; combo = 0; msg(G.sims[c.sim].name + " stormed out!"); sfxNo(); }
}
void doAction(const Act& a) {
    if (a.kind == 1) {   // kitchen window: pick up ready plates
        for (auto it = orders.begin(); it != orders.end() && hands.size() < 2;) {
            if (it->ready) { hands.push_back(it->table); it = orders.erase(it); } else ++it;
        }
        if (hands.empty()) msg("Nothing's ready yet!");
        sfxTap();
        return;
    }
    Table& tb = tables[a.arg];
    if (tb.dirty && tb.cust < 0) { tb.dirty = false; busy = 0.6f; msg("Sparkling clean!"); sfxTap(); return; }
    if (tb.cust < 0) return;
    Cust& c = cs[tb.cust];
    switch (c.state) {
    case C_ORDER: c.state = C_FOOD; c.t = 0; c.hearts = std::fmin(5.f, c.hearts + 0.5f); orders.push_back({a.arg, 4.f + frand() * 2.5f, false}); busy = 0.4f; sfxTap(); break;
    case C_FOOD: {
        auto it = std::find(hands.begin(), hands.end(), a.arg);
        if (it == hands.end()) { msg("Their food isn't in your hands!"); break; }
        hands.erase(it); c.state = C_EAT; c.t = 0; c.hearts = std::fmin(5.f, c.hearts + 1.f); sfx(700, 1000, 90);
        break;
    }
    case C_PAY: {
        combo++;
        int tip = 3 + (int)(c.hearts * 2.5f) + std::min(combo, 5);
        tips += tip; served++;
        char b[64]; snprintf(b, sizeof b, "+$%d tip!%s", tip, combo >= 3 ? " COMBO!" : ""); msg(b);
        leave(c, false); sfxYay(); busy = 0.3f;
        break;
    }
    default: break;
    }
}
}   // namespace

void serveStart() {
    cs.clear(); orders.clear(); queue.clear(); hands.clear();
    for (auto& t : tables) { t.cust = -1; t.dirty = false; }
    px = 240; py = 130; shiftLeft = 120; spawnT = 1; busy = 0; phase = 0; served = angry = tips = spawned = combo = 0; sel = -1; endT = 0; msgT = 0;
}

bool serveUpdate(const Input& in) {
    const float dt = in.dt;
    if (msgT > 0) msgT -= dt;
    if (phase == 0) { if ((in.down & KEY_A) || in.touch) { phase = 1; sfxTap(); } return true; }
    if (phase == 2) {
        endT += dt;
        if (endT > 1.f && ((in.down & KEY_A) || in.touch)) {
            int pay = 30 + tips;
            G.money += pay;
            Sim& me = MC();
            skillGain(me, SK_CHARISMA, 2.f + served * 0.3f);
            me.needs[N_ENERGY] = std::fmax(0.f, me.needs[N_ENERGY] - 25);
            if (angry == 0 && served >= 6) addMoodlet(me, "Waitress of the Year", EM_CONFIDENT, 2, 480);
            else if (angry >= 4) addMoodlet(me, "Rough Shift", EM_TENSE, 1, 240);
            else addMoodlet(me, "Good Shift", EM_HAPPY, 1, 240);
            char b[96]; snprintf(b, sizeof b, "Shift done! $30 wage + $%d tips", tips); rewardPopup("DINNER RUSH", b);
            goalNotify(GE_SHIFT, 1);
            return false;
        }
        return true;
    }
    if (in.down & KEY_B) { shiftLeft = 0; }
    // ---- clock & customers
    shiftLeft -= dt;
    spawnT -= dt;
    if (spawnT <= 0 && shiftLeft > 12 && lineCount() < 3) { spawn(); spawnT = 5.5f + frand() * 4.f - std::min(2.5f, served * 0.15f); }
    for (auto& c : cs) {
        if (c.state == C_GONE) continue;
        c.t += dt;
        const bool waiting = c.state == C_LINE || c.state == C_ORDER || c.state == C_FOOD || c.state == C_PAY;
        if (waiting) c.hearts -= dt / (c.state == C_FOOD ? 7.5f : 5.5f);
        if (c.hearts <= 0) { leave(c, true); continue; }
        if (c.state == C_MENU && c.t > 2.5f) { c.state = C_ORDER; c.t = 0; }
        if (c.state == C_EAT && c.t > 4.5f) { c.state = C_PAY; c.t = 0; }
    }
    // line shuffles forward
    { int k = 0; for (auto& c : cs) if (c.state == C_LINE) { float ty = LINE_Y - k * 28.f; c.y += (ty - c.y) * std::fmin(1.f, dt * 6); k++; } }
    for (auto& o : orders) if (!o.ready) { o.left -= dt; if (o.left <= 0) { o.ready = true; sfx(1200, 1400, 60); } }
    // ---- input: tap a customer in line, then a table; tap tables / the kitchen to queue up jobs
    if (in.touch) {
        const float tx = in.tp.px, ty = in.tp.py;
        bool used = false;
        for (size_t i = 0; i < cs.size() && !used; i++) {
            Cust& c = cs[i];
            if (c.state == C_LINE && std::fabs(tx - c.x) < 16 && std::fabs(ty - (c.y - 12)) < 18) { sel = (int)i; used = true; sfxTap(); }
        }
        for (int k = 0; k < 4 && !used; k++) {
            Table& tb = tables[k];
            if (std::fabs(tx - tb.x) < 36 && std::fabs(ty - tb.y) < 30) {
                used = true;
                if (sel >= 0 && sel < (int)cs.size() && cs[sel].state == C_LINE) {
                    if (tb.cust >= 0 || tb.dirty) { msg(tb.dirty ? "Clean that table first!" : "That table's taken!"); sfxNo(); }
                    else { Cust& c = cs[sel]; c.state = C_MENU; c.t = 0; c.table = k; tb.cust = sel; c.hearts = std::fmin(5.f, c.hearts + 0.5f); sel = -1; sfxTap(); }
                } else if (queue.size() < 4) { queue.push_back({0, k}); sfxTap(); }
            }
        }
        if (!used && std::fabs(tx - WIN_X) < 30 && std::fabs(ty - WIN_Y) < 26 && queue.size() < 4) { queue.push_back({1, 0}); sfxTap(); used = true; }
        if (!used) sel = -1;
    }
    // ---- the waitress walks to her next job
    if (busy > 0) busy -= dt;
    else if (!queue.empty()) {
        const Act& a = queue.front();
        float gx = a.kind == 1 ? WIN_X - 30 : tables[a.arg].x + 30, gy = a.kind == 1 ? WIN_Y + 22 : tables[a.arg].y + 8;
        float dx = gx - px, dy = gy - py, d = std::sqrt(dx * dx + dy * dy);
        float sp = 150.f + skillLevel(MC(), SK_FITNESS) * 8.f;
        if (d < 3) { Act done = a; queue.erase(queue.begin()); doAction(done); }
        else { px += dx / d * std::fmin(d, sp * dt); py += dy / d * std::fmin(d, sp * dt); g_walkPhase = std::fmod(g_walkPhase + dt * 2.4f, 1.f); }
    }
    if (shiftLeft <= 0) {
        bool anyone = false;
        for (auto& c : cs) if (c.state != C_GONE && c.state != C_LINE) anyone = true;
        for (auto& c : cs) if (c.state == C_LINE) leave(c, false);
        if (!anyone || shiftLeft < -25) { phase = 2; endT = 0; sfxYay(); }
    }
    cs.erase(std::remove_if(cs.begin(), cs.end(), [](const Cust&) { return false; }), cs.end());
    return true;
}

static void heartsBar(float x, float y, float h) {
    for (int i = 0; i < 5; i++) heart(x + i * 7.f, y, 2.8f, i < (int)std::ceil(h) ? C(255, 70, 120) : C(120, 100, 110, 150));
}

void serveDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(150, 230, 230), C(120, 210, 214));
    for (int i = 0; i < 20; i++) rect(i * 20.f, 0, 20, 10, C(i % 2 ? RGB{255, 255, 255} : RGB{255, 60, 100}));
    flashPanel(90, 18, 220, 40, RGB{255, 60, 100}, 14);
    textOutline("DINNER RUSH", 200, 22, 0.85f, C(255, 255, 255), C(UI_INK), 1, 1.3f);
    textOutline("Sunny Side Diner", 200, 62, 0.5f, C(255, 255, 255), C(UI_INK), 1);
    char b[96];
    flashPanel(30, 90, 340, 96, RGB{255, 255, 255}, 10, 235);
    snprintf(b, sizeof b, "Time left: %d s", (int)std::max(0.f, shiftLeft)); text(b, 48, 98, 0.5f, C(UI_INK));
    snprintf(b, sizeof b, "Tips: $%d", tips); text(b, 48, 120, 0.5f, C(20, 140, 60));
    snprintf(b, sizeof b, "Served: %d     Walked out: %d", served, angry); text(b, 48, 142, 0.5f, C(UI_INK));
    snprintf(b, sizeof b, "Combo x%d   Carrying: %d plate%s", combo, (int)hands.size(), hands.size() == 1 ? "" : "s"); text(b, 48, 164, 0.44f, C(150, 60, 160));
    int stars = served >= 10 && angry == 0 ? 3 : served >= 6 ? 2 : served >= 3 ? 1 : 0;
    for (int i = 0; i < 3; i++) sparkle(300 + i * 22.f, 120, 9, C(i < stars ? RGB{255, 210, 40} : RGB{200, 200, 210}));
    if (phase == 0 || phase == 2) {
        rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, 110));
        flashPanel(40, 60, 320, 120, phase == 0 ? RGB{255, 60, 100} : RGB{96, 206, 96}, 14);
        textOutline(phase == 0 ? "YOUR SHIFT" : "SHIFT OVER!", 200, 68, 0.75f, C(255, 255, 255), C(UI_INK), 1, 1.2f);
        const char* ls[4] = {"Tap a customer in line, then a free table.", "Tap their table when a hand goes up to take the order.", "Tap the kitchen window for plates, then serve.", "Collect the bill, then tap to clean the table!"};
        if (phase == 0) for (int i = 0; i < 4; i++) textOutline(ls[i], 200, 102 + i * 17.f, 0.4f, C(255, 255, 255), C(UI_INK), 1, 0.8f);
        else { snprintf(b, sizeof b, "Served %d  -  $30 wage + $%d tips", served, tips); textOutline(b, 200, 110, 0.5f, C(255, 255, 255), C(UI_INK), 1); textOutline(endT > 1 ? "Tap to clock out" : "", 200, 140, 0.44f, C(255, 236, 120), C(UI_INK), 1); }
    }
    (void)t;
}

void serveDrawBottom(float t) {
    // checkerboard diner floor
    for (int r = 0; r < 12; r++) for (int k = 0; k < 16; k++) rect(k * 20.f, r * 20.f, 20, 20, C((r + k) % 2 ? RGB{30, 26, 30} : RGB{250, 250, 250}));
    rect(0, 0, BOT_W, 34, C(150, 230, 230)); rect(0, 34, BOT_W, 2, C(UI_INK));
    // kitchen window
    rrect(WIN_X - 30, 18, 60, 40, 4, C(UI_INK)); rrect(WIN_X - 28, 20, 56, 36, 3, C(255, 236, 200));
    textOutline("KITCHEN", WIN_X, 2, 0.36f, C(255, 255, 255), C(UI_INK), 1, 0.7f);
    int k = 0;
    for (auto& o : orders) { float x = WIN_X - 20 + (k % 3) * 18.f, y = 38 + (k / 3) * 10.f; ellipse(x, y, 14, 6, C(255, 255, 255)); if (o.ready) { circle(x, y - 2, 3.5f, C(240, 160, 60)); sparkle(x + 6, y - 6, 3, C(255, 230, 80)); } else circle(x, y - 2, 2.5f, C(200, 200, 200, 160)); k++; }
    // tables
    for (int i = 0; i < 4; i++) {
        const Table& tb = tables[i];
        ellipse(tb.x, tb.y + 4, 64, 34, C(UI_INK)); ellipse(tb.x, tb.y + 4, 60, 30, C(255, 70, 110)); ellipse(tb.x, tb.y + 1, 56, 24, C(255, 255, 255));
        if (tb.dirty) for (int j = 0; j < 4; j++) circle(tb.x - 12 + j * 8.f, tb.y + (j % 2) * 3, 2.5f, C(150, 110, 70));
        if (tb.cust >= 0 && tb.cust < (int)cs.size()) {
            const Cust& c = cs[tb.cust];
            drawSim(G.sims[c.sim], tb.x - 22, tb.y + 14, 0.42f, t, 0, PO_IDLE, false);
            if (c.state == C_EAT) { ellipse(tb.x, tb.y, 14, 6, C(255, 255, 255)); circle(tb.x, tb.y - 2, 3.5f, C(240, 160, 60)); }
            const char* bub = c.state == C_ORDER ? "!" : c.state == C_PAY ? "$" : c.state == C_MENU ? "..." : c.state == C_FOOD ? "~" : nullptr;
            if (bub) { circle(tb.x + 4, tb.y - 30, 9, C(UI_INK)); circle(tb.x + 4, tb.y - 30, 7.5f, C(255, 255, 255)); text(bub, tb.x + 4, tb.y - 38, 0.45f, C(c.state == C_PAY ? RGB{20, 140, 60} : RGB{255, 46, 140}), 1); }
            heartsBar(tb.x - 16, tb.y + 22, c.hearts);
        }
    }
    // the line by the door
    rect(LINE_X - 26, 120, 52, 120, C(255, 236, 140, 90));
    textOutline("WAITING", LINE_X, 110, 0.32f, C(255, 255, 255), C(UI_INK), 1, 0.6f);
    for (size_t i = 0; i < cs.size(); i++) {
        const Cust& c = cs[i];
        if (c.state != C_LINE) continue;
        if ((int)i == sel) ellipse(c.x, c.y, 30, 10, C(255, 230, 80, 200));
        drawSim(G.sims[c.sim], c.x, c.y, 0.38f, t, 0, PO_IDLE, true);
        heartsBar(c.x - 16, c.y + 4, c.hearts);
    }
    // you, in a diner uniform
    {
        Sim me = MC(); me.look.top = RGB{255, 90, 130}; me.look.topStyle = TS_POLO; me.outfit = 0;
        drawSim(me, px, py + 18, 0.45f, t, 0, queue.empty() ? PO_IDLE : PO_WALK, false);
        for (size_t h = 0; h < hands.size(); h++) { ellipse(px + 10 + h * 8.f, py - 14 - h * 4.f, 12, 5, C(255, 255, 255)); circle(px + 10 + h * 8.f, py - 16 - h * 4.f, 3, C(240, 160, 60)); }
    }
    if (msgT > 0) { float w = textW(g_msg.c_str(), 0.42f) + 20; flashPanel(160 - w / 2, 204, w, 20, RGB{255, 46, 140}, 8); textOutline(g_msg.c_str(), 160, 205, 0.42f, C(255, 255, 255), C(UI_INK), 1); }
    textOutline("B: end shift early", 4, 38, 0.32f, C(UI_INK), C(255, 255, 255), 0, 0.6f);
}
