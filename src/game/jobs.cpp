// Playable work shifts. Each career gets a short (45 second) mini-game:
//   Hostess    - seat each party at a table that fits
//   Courier    - pick up orders at the food court and ride them to houses
//   Teacher    - answer raised hands, refocus distracted students
//   Rideshare  - dodge traffic and keep your passenger happy
//   Model      - strike the pose the photographer calls, right on the flash
//   Barista    - build the exact drink each customer orders
//   Chef       - stop the timing bar in the sweet spot for every cooking step
// Score -> 0..5 stars -> performance bonus + tips.
#include "jobs.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "goals.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace col;

float g_shiftHours = 0;

static int car = 0, lvl = 0, score = 0, stars = 0;
static float timer = 0, total = 45, msgT = 0, flashT = 0, shakeT = 0;
static bool done = false;
static std::string msg;
static const float SHIFT_SECS = 45;

static void pop(const std::string& s, bool good) { msg = s; msgT = 1.3f; if (good) sfxTap(); else sfxNo(); }
static int clampi(int v, int a, int b) { return v < a ? a : v > b ? b : v; }
static float frand(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }
static float dist2(float ax, float ay, float bx, float by) { return (ax - bx) * (ax - bx) + (ay - by) * (ay - by); }
static int randomTownie() {
    std::vector<int> pool;
    for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != G.mc && G.sims[i].stage >= ST_CHILD) pool.push_back((int)i);
    return pool.empty() ? G.mc : pool[irand((int)pool.size())];
}

bool shiftHasGame(int c) { return c == CR_HOSTESS || c == CR_DELIVERY || c == CR_TEACHER || c == CR_RIDESHARE || c == CR_MODEL || c == CR_BARISTA || c == CR_CHEF; }

// =========================================================== HOSTESS
struct Party { int size = 0; float pat = 0, patMax = 1; int sim = 0; bool on = false; };
struct Table { int seats; float busy; int size; };
static Party hq[4];
static Table ht[6];
static int hSel = -1;
static float hSpawn = 0;
static Btn hCard(int i) { return Btn{4, 26 + i * 46.f, 96, 42}; }
static Btn hTable(int i) { return Btn{106 + (i % 3) * 71.f, 26 + (i / 3) * 92.f, 66, 86}; }
static void hostStart() {
    static const int S[6] = {2, 2, 2, 4, 4, 6};
    for (int i = 0; i < 6; i++) ht[i] = Table{S[i], 0, 0};
    for (Party& p : hq) p.on = false;
    hSel = -1; hSpawn = 0.5f;
}
static void hostUpdate(const Input& in) {
    hSpawn -= in.dt;
    if (hSpawn <= 0) {
        for (Party& p : hq) if (!p.on) {
            int r = irand(100);
            p.size = r < 15 ? 1 : r < 55 ? 2 : r < 72 ? 3 : r < 88 ? 4 : 5 + irand(2);
            p.patMax = p.pat = frand(13.f, 17.f) - lvl * 1.2f;
            p.sim = randomTownie(); p.on = true;
            break;
        }
        hSpawn = frand(2.2f, 3.6f) - lvl * 0.2f;
    }
    for (int i = 0; i < 4; i++) {
        Party& p = hq[i];
        if (!p.on) continue;
        p.pat -= in.dt;
        if (p.pat <= 0) { p.on = false; score -= 5; if (hSel == i) hSel = -1; pop("A party got tired of waiting and left!", false); }
    }
    for (Table& t : ht) if (t.busy > 0) t.busy -= in.dt;
    if (!in.touch) return;
    for (int i = 0; i < 4; i++) if (hq[i].on && hCard(i).in(in.tp)) { hSel = i; sfxTap(); return; }
    for (int j = 0; j < 6; j++) {
        if (!hTable(j).in(in.tp)) continue;
        if (hSel < 0) { pop("Tap a waiting party first!", false); return; }
        Party& p = hq[hSel];
        Table& t = ht[j];
        if (t.busy > 0) { pop("That table is still eating!", false); return; }
        if (t.seats < p.size) { pop("Too small for that party!", false); return; }
        int extra = t.seats - p.size;
        int pts = 10 + (extra == 0 ? 6 : extra == 1 ? 3 : 0) + (int)(p.pat / p.patMax * 5);
        score += pts;
        t.busy = 7 + p.size * 1.6f; t.size = p.size;
        p.on = false; hSel = -1;
        char b[48]; snprintf(b, sizeof b, extra == 0 ? "Perfect fit! +%d" : "Seated! +%d", pts);
        pop(b, true);
        return;
    }
}
static void heads(float cx, float cy, int n, float r, float t) {
    static const RGB SK[6] = {{255, 224, 200}, {232, 180, 140}, {196, 136, 96}, {150, 100, 70}, {104, 68, 50}, {244, 200, 168}};
    for (int k = 0; k < n; k++) {
        float a = k * 6.2832f / n - 1.5708f;
        float x = cx + std::cos(a) * r, y = cy + std::sin(a) * r + std::sin(t * 3 + k) * 0.6f;
        circle(x, y, 5.5f, C(SK[(k * 7 + n) % 6]));
        ellipse(x, y - 3, 11, 6, C(k % 3 == 0 ? RGB{40, 28, 30} : k % 3 == 1 ? RGB{150, 96, 50} : RGB{230, 190, 110}));
    }
}
static void hostDrawBottom(float t) {
    for (int i = 0; i < 4; i++) {
        Btn b = hCard(i);
        const Party& p = hq[i];
        rrect(b.x, b.y, b.w, b.h, 7, C(p.on ? (hSel == i ? GOLD : CREAM) : RGB{90, 64, 56}, p.on ? 255 : 120));
        if (!p.on) { text("...", b.x + b.w / 2, b.y + 12, 0.45f, C(CREAM, 120), 1); continue; }
        for (int k = 0; k < p.size; k++) { circle(b.x + 12 + k * 13.f, b.y + 14, 5, C(HOT)); circle(b.x + 12 + k * 13.f, b.y + 11, 3, C(255, 220, 200)); }
        float k = clampv(p.pat / p.patMax, 0.f, 1.f);
        rrect(b.x + 6, b.y + 28, b.w - 12, 7, 3, C(UI2));
        rrect(b.x + 6, b.y + 28, (b.w - 12) * k, 7, 3, C(k > 0.5f ? RGB{120, 220, 140} : k > 0.25f ? RGB{240, 200, 80} : RGB{240, 80, 90}));
    }
    for (int j = 0; j < 6; j++) {
        Btn b = hTable(j);
        const Table& tb = ht[j];
        rrect(b.x, b.y, b.w, b.h, 8, C(255, 240, 244, 230));
        float cx = b.x + b.w / 2, cy = b.y + 38;
        for (int s = 0; s < tb.seats; s++) { float a = s * 6.2832f / tb.seats - 1.5708f; circle(cx + std::cos(a) * 24, cy + std::sin(a) * 24, 4.5f, C(BERRY)); }
        circle(cx, cy, 16, C(CREAM)); circle(cx, cy, 14, C(255, 255, 255));
        if (tb.busy > 0) { heads(cx, cy, tb.size, 24, t); circle(cx, cy, 5, C(HOT)); }
        else sparkle(cx, cy, 4, C(GOLD));
        char b2[16]; snprintf(b2, sizeof b2, "%d-top", tb.seats);
        text(tb.busy > 0 ? "eating" : b2, cx, b.y + b.h - 20, 0.4f, C(tb.busy > 0 ? UIM : BERRY), 1);
    }
}

// =========================================================== COURIER
struct Order { int house; float t, tMax; int state; bool on; };
static Order dorders[3];
static float dx = 60, dy = 130, dSpawn = 0, dFace = 1;
static int dBag = 0;
static const float HOUSES[8][2] = {{150, 50}, {230, 48}, {296, 70}, {290, 140}, {250, 208}, {160, 214}, {110, 160}, {196, 128}};
static const float RX = 40, RY = 76;   // the food court
static void delStart() { for (Order& o : dorders) o.on = false; dx = RX + 10; dy = RY + 40; dSpawn = 0.3f; dBag = 0; }
static void delUpdate(const Input& in) {
    dSpawn -= in.dt;
    if (dSpawn <= 0) {
        for (Order& o : dorders) if (!o.on) {
            int h; bool used;
            do { h = irand(8); used = false; for (Order& q : dorders) if (q.on && q.house == h) used = true; } while (used);
            o = Order{h, 0, 0, 0, true};
            o.tMax = o.t = frand(17.f, 21.f) - lvl * 1.2f;
            break;
        }
        dSpawn = frand(3.f, 5.f);
    }
    float mx = 0, my = 0;
    if (in.held & KEY_LEFT) mx -= 1; if (in.held & KEY_RIGHT) mx += 1;
    if (in.held & KEY_UP) my -= 1; if (in.held & KEY_DOWN) my += 1;
    if (std::abs(in.cp.dx) > 20) mx = in.cp.dx / 150.f;
    if (std::abs(in.cp.dy) > 20) my = -in.cp.dy / 150.f;
    if ((in.held & KEY_TOUCH) && in.tp.py > 22) {   // hold the stylus where you want to ride
        float ddx = in.tp.px - dx, ddy = in.tp.py - dy, l = std::sqrt(ddx * ddx + ddy * ddy);
        if (l > 4) { mx = ddx / l; my = ddy / l; }
    }
    float sp = 120 + lvl * 6;
    dx = clampv(dx + mx * sp * in.dt, 10.f, 310.f);
    dy = clampv(dy + my * sp * in.dt, 30.f, 230.f);
    if (mx) dFace = mx > 0 ? 1.f : -1.f;
    if (dist2(dx, dy, RX, RY) < 22 * 22) {
        int n = 0;
        for (Order& o : dorders) if (o.on && o.state == 0) { o.state = 1; n++; }
        if (n) { dBag += n; pop(n > 1 ? "Picked up the orders!" : "Order picked up!", true); }
    }
    for (Order& o : dorders) {
        if (!o.on) continue;
        o.t -= in.dt;
        if (o.state == 1 && dist2(dx, dy, HOUSES[o.house][0], HOUSES[o.house][1]) < 18 * 18) {
            int pts = 12 + (int)(o.t / o.tMax * 8);
            score += pts; o.on = false; dBag--;
            char b[40]; snprintf(b, sizeof b, "Delivered! +%d", pts);
            pop(b, true); flashT = 0.3f;
        } else if (o.t <= 0) {
            if (o.state == 1) dBag--;
            o.on = false; score -= 6; pop("Order cancelled - too slow!", false);
        }
    }
}
static void delDrawBottom(float t) {
    rect(0, 22, BOT_W, BOT_H - 22, C(150, 210, 140));
    for (int i = 0; i < 14; i++) circle(std::fmod(i * 53.f, 320.f), 30 + std::fmod(i * 37.f, 200.f), 3, C(130, 196, 120));
    // streets
    rect(0, 94, BOT_W, 18, C(196, 190, 200)); rect(70, 22, 18, 218, C(196, 190, 200)); rect(200, 22, 18, 218, C(196, 190, 200)); rect(0, 178, BOT_W, 18, C(196, 190, 200));
    for (int i = 0; i < 16; i++) { rect(i * 22.f, 102, 10, 2, C(255, 255, 255, 180)); rect(i * 22.f, 186, 10, 2, C(255, 255, 255, 180)); }
    // food court
    rrect(RX - 26, RY - 26, 52, 44, 5, C(HOT)); rect(RX - 26, RY - 26, 52, 10, C(BERRY));
    text("FOOD", RX, RY - 8, 0.38f, C(WHITE), 1);
    // houses
    static const RGB HC[4] = {{255, 214, 230}, {214, 230, 255}, {255, 240, 200}, {224, 214, 255}};
    for (int h = 0; h < 8; h++) {
        float x = HOUSES[h][0], y = HOUSES[h][1];
        rect(x - 12, y - 8, 24, 16, C(HC[h % 4]));
        tri(x - 15, y - 8, x + 15, y - 8, x, y - 20, C(h % 2 ? BERRY : MOCHA));
        rect(x - 3, y + 1, 6, 7, C(UI2));
    }
    for (const Order& o : dorders) {
        if (!o.on) continue;
        float x = HOUSES[o.house][0], y = HOUSES[o.house][1] - 30 + std::sin(t * 5 + o.house) * 2;
        bool carried = o.state == 1;
        heart(x, y, 7, C(carried ? HOT : RGB{160, 150, 160}));
        float k = clampv(o.t / o.tMax, 0.f, 1.f);
        rect(x - 12, y + 9, 24, 3, C(UI2)); rect(x - 12, y + 9, 24 * k, 3, C(k > 0.4f ? RGB{120, 220, 140} : RGB{240, 80, 90}));
    }
    // you, on a pink scooter
    float bob = std::sin(t * 12) * 0.8f;
    ellipse(dx, dy + 9, 26, 6, C(0, 0, 0, 50));
    rrect(dx - 12, dy + bob, 24, 7, 3, C(HOT));
    circle(dx - 9, dy + 8, 4, C(CHOC)); circle(dx + 9, dy + 8, 4, C(CHOC));
    line(dx + 8 * dFace, dy + bob, dx + 11 * dFace, dy - 10 + bob, 2, C(LIGHTGRAY));
    rect(dx - 4, dy - 14 + bob, 8, 14, C(MC().look.top));
    circle(dx, dy - 18 + bob, 5, C(GOLD));
    rect(dx - 6 * dFace - 4, dy - 12 + bob, 8, 8, C(CREAM));   // delivery bag
    if (flashT > 0) circle(dx, dy, 30 * (1 - flashT / 0.3f), C(255, 255, 255, 120));
}

// =========================================================== TEACHER
struct Desk { int state; float t; int kind; int skin, hair; };
static Desk desks[6];
static int tOpen = -1, tAns = 0;
static float tSpawn = 0, focus = 100;
static std::string tQ, tOpt[3];
static Btn tDesk(int i) { return Btn{6 + (i % 3) * 104.f, 26 + (i / 3) * 84.f, 98, 78}; }
static Btn tAnsB(int i) { return Btn{12 + i * 100.f, 168, 94, 36}; }
static void makeQuestion() {
    if (irand(2) == 0) {
        int a = 2 + irand(9 + lvl), b = 2 + irand(9), op = irand(3), r;
        char q[32];
        if (op == 0) { r = a + b; snprintf(q, sizeof q, "%d + %d = ?", a, b); }
        else if (op == 1) { if (b > a) std::swap(a, b); r = a - b; snprintf(q, sizeof q, "%d - %d = ?", a, b); }
        else { r = a * b; snprintf(q, sizeof q, "%d x %d = ?", a, b); }
        tQ = q;
        int opts[3] = {r, r + 1 + irand(3), r - 1 - irand(3)};
        if (op == 2) opts[1] = r + a;
        tAns = irand(3);
        std::swap(opts[0], opts[tAns]);
        for (int i = 0; i < 3; i++) tOpt[i] = std::to_string(opts[i]);
    } else {
        static const char* W[][3] = {{"necessary", "neccessary", "necesary"}, {"beautiful", "beutiful", "beautifull"},
                                     {"friend", "freind", "frend"}, {"because", "becuase", "becaus"}, {"believe", "beleive", "belive"},
                                     {"tomorrow", "tommorow", "tomorow"}, {"separate", "seperate", "separete"}, {"definitely", "definately", "definitly"},
                                     {"rhythm", "rythm", "rhythym"}, {"fashion", "fashon", "fasion"}, {"calendar", "calender", "calandar"}, {"different", "diffrent", "diferent"}};
        int w = irand(12);
        tQ = "Which spelling is right?";
        tAns = irand(3);
        int k = 1;
        for (int i = 0; i < 3; i++) tOpt[i] = i == tAns ? W[w][0] : W[w][k++];
    }
}
static void teachStart() {
    for (Desk& d : desks) d = Desk{0, 0, 0, irand(6), irand(5)};
    tOpen = -1; tSpawn = 1.f; focus = 100;
}
static void teachUpdate(const Input& in) {
    tSpawn -= in.dt;
    if (tSpawn <= 0) {
        int free[6], n = 0;
        for (int i = 0; i < 6; i++) if (desks[i].state == 0 && i != tOpen) free[n++] = i;
        if (n) {
            Desk& d = desks[free[irand(n)]];
            d.state = irand(100) < 55 ? 2 : 1; d.t = d.state == 2 ? 9.f : 0.f; d.kind = irand(3);
        }
        tSpawn = frand(1.6f, 2.8f) - lvl * 0.15f;
    }
    for (int i = 0; i < 6; i++) {
        Desk& d = desks[i];
        if (d.state == 1) { focus -= 3.2f * in.dt; d.t += in.dt; }
        if (d.state == 2 && i != tOpen) { d.t -= in.dt; if (d.t <= 0) { d.state = 0; focus -= 6; pop("You missed a question!", false); } }
    }
    focus = clampv(focus, 0.f, 100.f);
    if (focus <= 0 && !done) { timer = 0; pop("Class got out of control!", false); }
    if (!in.touch) return;
    if (tOpen >= 0) {
        for (int i = 0; i < 3; i++) if (tAnsB(i).in(in.tp)) {
            if (i == tAns) { score += 10; focus = std::fmin(100.f, focus + 5); pop("Correct! Great teaching. +10", true); }
            else { score -= 3; pop("Oops, that wasn't right.", false); }
            desks[tOpen].state = 0; tOpen = -1;
            return;
        }
        return;
    }
    for (int i = 0; i < 6; i++) {
        if (!tDesk(i).in(in.tp)) continue;
        Desk& d = desks[i];
        if (d.state == 1) { d.state = 0; score += 3; focus = std::fmin(100.f, focus + 3); pop("Eyes up front! +3", true); }
        else if (d.state == 2) { tOpen = i; makeQuestion(); sfxTap(); }
        return;
    }
}
static void teachDrawBottom(float t) {
    rect(0, 22, BOT_W, BOT_H - 22, C(232, 214, 186));
    static const RGB SK[6] = {{255, 224, 200}, {232, 180, 140}, {196, 136, 96}, {150, 100, 70}, {104, 68, 50}, {244, 200, 168}};
    static const RGB HR[5] = {{40, 28, 30}, {110, 64, 38}, {230, 190, 110}, {160, 52, 38}, {240, 120, 172}};
    for (int i = 0; i < 6; i++) {
        Btn b = tDesk(i);
        const Desk& d = desks[i];
        float cx = b.x + b.w / 2;
        float bob = d.state == 2 ? std::sin(t * 8) * 1.5f : 0;
        circle(cx, b.y + 30 + bob, 11, C(SK[d.skin]));
        ellipse(cx, b.y + 23 + bob, 24, 12, C(HR[d.hair]));
        rect(cx - 9, b.y + 40, 18, 12, C(i % 2 ? HOT : BABYBLUE));
        rrect(b.x + 6, b.y + 50, b.w - 12, 22, 4, C(196, 140, 92)); rect(b.x + 8, b.y + 52, b.w - 16, 3, C(220, 170, 120));
        if (d.state == 0) { circle(cx - 4, b.y + 30, 1.4f, C(BLACK)); circle(cx + 4, b.y + 30, 1.4f, C(BLACK)); }
        if (d.state == 2) {   // hand up
            line(cx + 10, b.y + 40, cx + 16, b.y + 14 + bob, 4, C(SK[d.skin]));
            rrect(cx + 18, b.y + 2, 18, 18, 6, C(255, 255, 255)); text("?", cx + 27, b.y + 3, 0.55f, C(HOT), 1);
            float k = clampv(d.t / 9.f, 0.f, 1.f); rect(b.x + 8, b.y + 74, (b.w - 16) * k, 3, C(HOT));
        } else if (d.state == 1) {
            static const char* IC[3] = {"zzz", "*txt*", "doodle"};
            rrect(cx - 26, b.y + 4, 52, 16, 6, C(150, 140, 160, 220));
            text(IC[d.kind], cx, b.y + 5, 0.38f, C(WHITE), 1);
            line(cx - 5, b.y + 30, cx - 2, b.y + 30, 1.2f, C(BLACK)); line(cx + 2, b.y + 30, cx + 5, b.y + 30, 1.2f, C(BLACK));
        }
    }
    // class focus meter
    rrect(6, 196, 200, 12, 5, C(UI2));
    rrect(7, 197, 198 * focus / 100.f, 10, 4, C(focus > 50 ? RGB{120, 220, 140} : focus > 25 ? RGB{240, 200, 80} : RGB{240, 80, 90}));
    text("Class focus", 212, 194, 0.4f, C(UI1));
    if (tOpen >= 0) {
        rect(0, 22, BOT_W, BOT_H - 22, C(0, 0, 0, 120));
        rrect(8, 60, 304, 156, 12, C(40, 80, 60)); rrect(12, 64, 296, 148, 10, C(52, 104, 78));   // chalkboard
        text(desks[tOpen].kind == 0 ? "Teacher, can you help?" : "I have a question!", 160, 72, 0.42f, C(255, 255, 255, 200), 1);
        text(tQ.c_str(), 160, 100, 0.75f, C(WHITE), 1);
        for (int i = 0; i < 3; i++) button(tAnsB(i), tOpt[i].c_str(), false, 2, 0.5f);
    }
}

// =========================================================== RIDESHARE
struct Obs { int lane; float y; int kind; };
static std::vector<Obs> obs;
static int lane = 1, pax = 0, chatMood = -1, chatAns = 0;
static float rSpawn = 0, rideT = 0, chatT = 0, chatWait = 0, scroll = 0, rating = 5.f;
static std::string chatLine, chatOpt[3];
static const float LANE_X[3] = {150, 200, 250};
static Btn rLaneB(int d) { return Btn{d < 0 ? 4.f : 236.f, 196, 80, 40}; }
static Btn rChatB(int i) { return Btn{100, 52 + i * 44.f, 214, 38}; }
static void newPassenger() { pax = randomTownie(); rideT = 0; chatT = frand(2.f, 4.f); chatMood = -1; }
static void rideStart() { obs.clear(); lane = 1; rSpawn = 1.f; scroll = 0; rating = 5.f; newPassenger(); }
static void newChat() {
    static const char* LINES[4][3] = {
        {"So, have you seen any good movies lately?!", "OMG you will not BELIEVE my week.", "I love your car! Is that a new air freshener?"},
        {"(they put in their earbuds)", "(quietly scrolls their phone)", "(gazes out the window)"},
        {"I'm SO late, can we hurry?!", "My meeting starts in five minutes!!", "Is there a faster way?"},
        {"Sorry... it's been a rough day.", "My date just cancelled on me...", "(sniffles quietly)"}};
    static const char* REPLIES[4] = {"Ooh, tell me everything!", "*turns the music down low*", "Shortcut - hold on tight!", "Want me to play a happy song?"};
    chatMood = irand(4);
    chatLine = LINES[chatMood][irand(3)];
    int ids[3] = {chatMood, (chatMood + 1 + irand(3)) % 4, 0};
    do { ids[2] = irand(4); } while (ids[2] == ids[0] || ids[2] == ids[1]);
    chatAns = irand(3);
    std::swap(ids[0], ids[chatAns]);
    for (int i = 0; i < 3; i++) chatOpt[i] = REPLIES[ids[i]];
    chatWait = 6.f;
}
static void rideUpdate(const Input& in) {
    float sp = 150 + lvl * 15;
    scroll = std::fmod(scroll + sp * in.dt, 40.f);
    if (in.down & (KEY_LEFT | KEY_L)) lane = std::max(0, lane - 1);
    if (in.down & (KEY_RIGHT | KEY_R)) lane = std::min(2, lane + 1);
    if (in.touch && rLaneB(-1).in(in.tp)) lane = std::max(0, lane - 1);
    if (in.touch && rLaneB(1).in(in.tp)) lane = std::min(2, lane + 1);
    rSpawn -= in.dt;
    if (rSpawn <= 0) { obs.push_back({irand(3), -20, irand(3)}); rSpawn = frand(0.9f, 1.6f) - lvl * 0.08f; }
    for (Obs& o : obs) o.y += sp * in.dt;
    for (size_t i = 0; i < obs.size();) {
        if (obs[i].lane == lane && std::fabs(obs[i].y - 196) < 22) {
            score -= 6; rating = std::fmax(1.f, rating - 0.4f); shakeT = 0.4f;
            pop(obs[i].kind == 0 ? "Bonk! A traffic cone!" : obs[i].kind == 1 ? "Ouch, a pothole!" : "Watch the other cars!", false);
            obs.erase(obs.begin() + i); continue;
        }
        if (obs[i].y > 260) { obs.erase(obs.begin() + i); continue; }
        i++;
    }
    rideT += in.dt;
    if (rideT > 13) {
        int pts = (int)(6 + rating * 2.4f);
        score += pts;
        char b[48]; snprintf(b, sizeof b, "Dropped off! %.1f stars, +%d", rating, pts);
        pop(b, true); flashT = 0.4f;
        newPassenger();
    }
    if (chatMood < 0) { chatT -= in.dt; if (chatT <= 0) newChat(); }
    else {
        chatWait -= in.dt;
        if (chatWait <= 0) { chatMood = -1; chatT = frand(3.f, 5.f); rating = std::fmax(1.f, rating - 0.2f); pop("You left them hanging...", false); }
        else if (in.touch) for (int i = 0; i < 3; i++) if (rChatB(i).in(in.tp)) {
            if (i == chatAns) { score += 8; rating = std::fmin(5.f, rating + 0.3f); pop("They loved that! +8", true); }
            else { rating = std::fmax(1.f, rating - 0.3f); pop("Awkward...", false); }
            chatMood = -1; chatT = frand(3.f, 5.f);
            break;
        }
    }
}
static void rideDrawTop(float t) {
    float sx = shakeT > 0 ? std::sin(t * 60) * 3 : 0;
    grad(0, 0, TOP_W, TOP_H, C(60, 40, 90), C(240, 140, 160));   // sunset city
    for (int i = 0; i < 9; i++) { float h = 40 + (i * 37 % 50); rect(i * 46.f, 120 - h, 40, h, C(70, 50, 90)); for (int w = 0; w < 4; w++) rect(i * 46.f + 6 + (w % 2) * 18, 128 - h + (w / 2) * 14, 8, 6, C(255, 220, 140, 200)); }
    rect(0, 110, TOP_W, 130, C(70, 70, 80));
    rect(110 + sx, 110, 180, 130, C(90, 90, 100));
    for (int i = 0; i < 2; i++) for (int k = -1; k < 4; k++) rect(173 + i * 50 + sx, 110 + k * 40 + scroll, 4, 20, C(255, 255, 255, 200));
    rect(108 + sx, 110, 3, 130, C(255, 220, 120)); rect(289 + sx, 110, 3, 130, C(255, 220, 120));
    for (const Obs& o : obs) {
        float x = LANE_X[o.lane] + sx;
        if (o.kind == 0) { tri(x - 8, o.y + 8, x + 8, o.y + 8, x, o.y - 10, C(255, 140, 40)); rect(x - 5, o.y - 2, 10, 3, C(WHITE)); }
        else if (o.kind == 1) ellipse(x, o.y, 26, 10, C(40, 40, 46));
        else { rrect(x - 13, o.y - 18, 26, 36, 6, C(o.lane == 1 ? BABYBLUE : MINT)); rect(x - 10, o.y - 12, 20, 8, C(60, 80, 110)); }
    }
    // your car
    float x = LANE_X[lane] + sx, y = 196;
    rrect(x - 15, y - 22, 30, 44, 8, C(HOT)); rect(x - 11, y - 14, 22, 10, C(80, 110, 150)); rect(x - 11, y + 8, 22, 6, C(80, 110, 150));
    circle(x - 15, y - 12, 3, C(CHOC)); circle(x + 15, y - 12, 3, C(CHOC)); circle(x - 15, y + 14, 3, C(CHOC)); circle(x + 15, y + 14, 3, C(CHOC));
    // rating & ride progress
    rrect(8, 128, 92, 52, 8, C(UI1, 220));
    char b[32]; snprintf(b, sizeof b, "%.1f", rating); text(b, 54, 132, 0.7f, C(GOLD), 1);
    for (int i = 0; i < 5; i++) sparkle(22 + i * 16.f, 166, i < (int)(rating + 0.5f) ? 4.f : 2.f, C(i < (int)(rating + 0.5f) ? GOLD : CREAM));
    rrect(300, 128, 92, 52, 8, C(UI1, 220));
    text("Ride", 346, 132, 0.45f, C(CREAM), 1);
    rect(310, 160, 72, 6, C(UI2)); rect(310, 160, 72 * clampv(rideT / 13.f, 0.f, 1.f), 6, C(MINT));
    if (flashT > 0) rect(0, 0, TOP_W, TOP_H, C(255, 255, 255, (uint8_t)(flashT * 200)));
}
static void rideDrawBottom(float t) {
    rect(0, 22, BOT_W, BOT_H - 22, C(60, 50, 70));
    drawSimPortrait(G.sims[pax], 48, 82, 36, t);
    text(G.sims[pax].name.c_str(), 48, 122, 0.45f, C(BLUSH), 1);
    if (chatMood >= 0) {
        rrect(98, 24, 218, 24, 8, C(255, 255, 255));
        auto lines = wrapText(chatLine, 0.38f, 206);
        text(lines.empty() ? "" : lines[0].c_str(), 207, 28, 0.38f, C(UI1), 1);
        for (int i = 0; i < 3; i++) button(rChatB(i), chatOpt[i].c_str(), false, 0, 0.42f);
        rect(98, 186, 218 * clampv(chatWait / 6.f, 0.f, 1.f), 4, C(HOT));
    } else text("Drive safe! L / R or tap < > to change lanes.", 207, 90, 0.38f, C(CREAM), 1);
    button(rLaneB(-1), "<", false, 1, 0.7f);
    button(rLaneB(1), ">", false, 1, 0.7f);
}

// =========================================================== MODEL
static const char* POSE_NAMES[4] = {"Hand on Hip", "Peace Sign", "Hair Flip", "Selfie Pout"};
static const u32 POSE_KEYS[4] = {KEY_A, KEY_B, KEY_X, KEY_Y};
static const char* POSE_BTN[4] = {"A", "B", "X", "Y"};
static int mWant = 0, mPose = 0, mStreak = 0;
static float mRing = 90, mBeat = 1.7f, mHoldT = 0;
static bool mJudged = false;
static Btn mPoseB(int i) { return Btn{8 + (i % 2) * 154.f, 140 + (i / 2) * 48.f, 150, 44}; }
static void modelNext() { mWant = irand(4); mRing = 90; mJudged = false; mBeat = std::max(0.95f, 1.7f - lvl * 0.1f - mStreak * 0.03f); }
static void modelStart() { mStreak = 0; mPose = 0; modelNext(); }
static void modelJudge(int p) {
    mPose = p; mHoldT = 0.8f; mJudged = true;
    float off = std::fabs(mRing - 24);
    if (p != mWant) { score -= 3; mStreak = 0; pop("Wrong pose!", false); }
    else if (off < 7) { score += 12 + std::min(mStreak, 5); mStreak++; flashT = 0.35f; pop("PERFECT! Stunning!", true); }
    else if (off < 16) { score += 7; mStreak++; flashT = 0.25f; pop("Nice shot!", true); }
    else { score -= 2; mStreak = 0; pop(mRing > 24 ? "Too early!" : "Too late!", false); }
}
static void modelUpdate(const Input& in) {
    if (mJudged) { mHoldT -= in.dt; if (mHoldT <= 0) modelNext(); return; }
    mRing -= (90.f / mBeat) * in.dt;
    for (int i = 0; i < 4; i++) {
        if ((in.down & POSE_KEYS[i]) || (in.touch && mPoseB(i).in(in.tp))) { modelJudge(i); return; }
    }
    if (mRing < 0) { score -= 2; mStreak = 0; mJudged = true; mHoldT = 0.5f; pop("Missed the shot!", false); }
}
static void modelDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(250, 236, 244), C(230, 210, 230));
    ellipse(200, 236, 300, 40, C(255, 255, 255, 160));
    for (int s = 0; s < 2; s++) {   // softbox lights
        float x = s ? 352 : 48;
        line(x, 90, x, 230, 3, C(60, 60, 70));
        quad(x - 26, 40, x + 26, 40, x + 18, 92, x - 18, 92, C(255, 255, 255, 230)); quad(x - 26, 40, x + 26, 40, x + 20, 46, x - 20, 46, C(80, 80, 90));
    }
    static const Pose PP[4] = {PO_IDLE, PO_WAVE, PO_DANCE, PO_PHONE};
    drawSim(MC(), 200, 228, 1.45f, t, 0, mJudged ? PP[mPose] : PO_IDLE, false);
    rrect(110, 54, 180, 26, 10, C(UI1, 220));
    char b[48]; snprintf(b, sizeof b, "\"Give me... %s!\"", POSE_NAMES[mWant]);
    text(b, 200, 58, 0.45f, C(BLUSH), 1);
    if (mStreak > 1) { snprintf(b, sizeof b, "Streak x%d", mStreak); text(b, 392, 40, 0.5f, C(HOT), 2); }
    if (flashT > 0) rect(0, 0, TOP_W, TOP_H, C(255, 255, 255, (uint8_t)(flashT * 600 > 255 ? 255 : flashT * 600)));
}
static void modelDrawBottom(float t) {
    rect(0, 22, BOT_W, BOT_H - 22, C(40, 34, 44));
    // camera with the closing ring
    rrect(110, 34, 100, 72, 12, C(30, 28, 34)); rect(128, 28, 30, 10, C(30, 28, 34));
    circle(160, 70, 28, C(70, 70, 80)); circle(160, 70, 24, C(20, 20, 30)); circle(152, 62, 6, C(255, 255, 255, 90));
    if (!mJudged) {
        u32 rc = std::fabs(mRing - 24) < 7 ? C(120, 240, 140) : C(255, 140, 200);
        for (int i = 0; i < 24; i++) {
            float a0 = i * 0.2618f, a1 = (i + 1) * 0.2618f;
            line(160 + std::cos(a0) * mRing, 70 + std::sin(a0) * mRing, 160 + std::cos(a1) * mRing, 70 + std::sin(a1) * mRing, 3, rc);
        }
    }
    text("Hit the pose when the ring meets the lens!", 160, 112, 0.38f, C(CREAM), 1);
    for (int i = 0; i < 4; i++) {
        char b[32]; snprintf(b, sizeof b, "%s  %s", POSE_BTN[i], POSE_NAMES[i]);
        button(mPoseB(i), b, i == mWant && !mJudged && std::fmod(t, 0.6f) < 0.3f, i == mWant ? 2 : 0, 0.45f);
    }
}

// =========================================================== BARISTA
static const char* B_SIZE[3] = {"Small", "Medium", "Large"};
static const char* B_DRINK[4] = {"Latte", "Matcha", "Chai", "Mocha"};
static const char* B_MILK[3] = {"Whole", "Oat", "No Milk"};
static const char* B_TOP[4] = {"Heart Art", "Caramel", "Whip", "Cinnamon"};
static int bWant[4], bHave[4], bCust = 0;
static float bPat = 0, bPatMax = 1, bCountdown = 0;
static Btn bOpt(int row, int i, int n) { float w = (308.f - (n - 1) * 4) / n; return Btn{6 + i * (w + 4), 26 + row * 40.f, w, 34}; }
static const Btn bServe{170, 192, 144, 40}, bTrash{6, 192, 100, 40};
static void baristaNew() {
    bWant[0] = irand(3); bWant[1] = irand(4); bWant[2] = irand(3); bWant[3] = irand(4);
    for (int& h : bHave) h = -1;
    bCust = randomTownie();
    bPatMax = bPat = 17.f - lvl * 1.4f;
}
static void baristaStart() { baristaNew(); }
static void baristaUpdate(const Input& in) {
    bPat -= in.dt;
    if (bPat <= 0) { score -= 5; pop("They left without their drink!", false); baristaNew(); return; }
    if (!in.touch) return;
    static const int N[4] = {3, 4, 3, 4};
    for (int r = 0; r < 4; r++) for (int i = 0; i < N[r]; i++) if (bOpt(r, i, N[r]).in(in.tp)) { bHave[r] = i; sfxTap(); return; }
    if (bTrash.in(in.tp)) { for (int& h : bHave) h = -1; sfxTap(); return; }
    if (bServe.in(in.tp)) {
        int ok = 0;
        for (int r = 0; r < 4; r++) if (bHave[r] == bWant[r]) ok++;
        if (ok == 4) { int pts = 10 + (int)(bPat / bPatMax * 6); score += pts; char b[40]; snprintf(b, sizeof b, "Perfect drink! +%d", pts); pop(b, true); flashT = 0.3f; }
        else { score += ok - 3; pop("That's not what I ordered...", false); }
        baristaNew();
    }
}
static void baristaDrawTop(float t) {
    rrect(244, 30, 148, 120, 10, C(UI1)); rrect(246, 32, 144, 116, 9, C(255, 252, 240));
    text("ORDER", 318, 36, 0.45f, C(BERRY), 1);
    const char* rows[4] = {B_SIZE[bWant[0]], B_DRINK[bWant[1]], B_MILK[bWant[2]], B_TOP[bWant[3]]};
    for (int r = 0; r < 4; r++) text(rows[r], 318, 56 + r * 20.f, 0.48f, C(bHave[r] == bWant[r] ? RGB{60, 160, 90} : UI1), 1);
    float k = clampv(bPat / bPatMax, 0.f, 1.f);
    rect(252, 140, 132 * k, 4, C(k > 0.4f ? RGB{120, 220, 140} : RGB{240, 80, 90}));
    drawSimPortrait(G.sims[bCust], 318, 190, 34, t);
}
static void baristaDrawBottom(float t) {
    static const int N[4] = {3, 4, 3, 4};
    const char** L[4] = {B_SIZE, B_DRINK, B_MILK, B_TOP};
    for (int r = 0; r < 4; r++) for (int i = 0; i < N[r]; i++) button(bOpt(r, i, N[r]), L[r][i], bHave[r] == i, bHave[r] == i ? 2 : 0, 0.42f);
    button(bTrash, "Start Over", false, 1, 0.45f);
    button(bServe, "Serve!", false, 2, 0.6f);
    (void)t;
}

// =========================================================== CHEF
static const char* DISHES[] = {"Truffle Pasta", "Pink Sushi Roll", "Glam Burger", "Lobster Mac", "Strawberry Crepe", "Pad Thai", "Ramen Bowl", "Caesar Salad"};
static const char* STEPS[3] = {"Chop", "Cook", "Plate"};
static int cDish = 0, cStep = 0, cGood = 0;
static float cPos = 0, cDir = 1, cZone = 0.4f, cZoneW = 0.2f, cSpd = 0.9f, cHold = 0;
static const Btn cStop{60, 160, 200, 60};
static void chefZone() { cZoneW = std::max(0.1f, 0.22f - lvl * 0.02f); cZone = frand(0.1f, 0.9f - cZoneW); cPos = 0; cDir = 1; cSpd = 0.85f + lvl * 0.08f + cStep * 0.1f; }
static void chefNewDish() { cDish = irand(8); cStep = 0; cGood = 0; chefZone(); }
static void chefStart() { chefNewDish(); }
static void chefUpdate(const Input& in) {
    if (cHold > 0) { cHold -= in.dt; if (cHold <= 0) { if (cStep >= 3) chefNewDish(); else chefZone(); } return; }
    cPos += cDir * cSpd * in.dt;
    if (cPos > 1) { cPos = 1; cDir = -1; }
    if (cPos < 0) { cPos = 0; cDir = 1; }
    if ((in.down & KEY_A) || (in.touch && cStop.in(in.tp))) {
        float mid = cZone + cZoneW / 2;
        if (cPos >= cZone && cPos <= cZone + cZoneW) {
            bool perfect = std::fabs(cPos - mid) < cZoneW / 6;
            score += perfect ? 5 : 3; cGood++;
            pop(perfect ? "Perfect!" : "Good!", true);
        } else { score -= 2; pop(cStep == 1 ? "Burnt!" : cStep == 0 ? "Messy chop!" : "Sloppy plating!", false); }
        cStep++;
        if (cStep >= 3) { if (cGood == 3) { score += 5; pop("Chef's kiss! Dish served +5", true); flashT = 0.3f; } else pop("Dish served.", true); }
        cHold = 0.5f;
    }
}
static void chefDrawTop(float t) {
    rrect(250, 26, 142, 100, 8, C(255, 255, 255)); rect(250, 26, 142, 14, C(BERRY));
    text("TICKET", 321, 27, 0.4f, C(WHITE), 1);
    text(DISHES[cDish], 321, 46, 0.48f, C(UI1), 1);
    for (int s = 0; s < 3; s++) {
        bool dn = s < cStep;
        rrect(262 + s * 42.f, 72, 38, 22, 6, C(dn ? MINT : s == cStep ? GOLD : CREAM));
        text(STEPS[s], 281 + s * 42.f, 75, 0.38f, C(UI1), 1);
    }
    (void)t;
}
static void chefDrawBottom(float t) {
    rect(0, 22, BOT_W, BOT_H - 22, C(220, 220, 228));
    for (int i = 0; i < 8; i++) rect(i * 40.f, 22, 1, 218, C(200, 200, 210));
    char b[48]; snprintf(b, sizeof b, "%s: %s", DISHES[cDish], cStep < 3 ? STEPS[cStep] : "Served!");
    text(b, 160, 40, 0.55f, C(UI1), 1);
    const float x0 = 20, w = 280, y = 90;
    rrect(x0 - 3, y - 3, w + 6, 36, 8, C(UI1));
    rect(x0, y, w, 30, C(250, 180, 170));
    rect(x0 + w * cZone, y, w * cZoneW, 30, C(120, 220, 140));
    rect(x0 + w * (cZone + cZoneW / 3), y, w * cZoneW / 3, 30, C(70, 190, 100));
    float mx = x0 + w * cPos;
    tri(mx - 7, y - 10, mx + 7, y - 10, mx, y + 2, C(HOT)); rect(mx - 1.5f, y, 3, 30, C(BLACK));
    button(cStop, cStep == 0 ? "CHOP! (A)" : cStep == 1 ? "FLIP! (A)" : "PLATE! (A)", false, 2, 0.6f);
    (void)t;
}

// =========================================================== shared shell
static const char* howTo(int c) {
    switch (c) {
    case CR_HOSTESS: return "Tap a waiting party, then a free table big enough. An exact fit earns a bonus!";
    case CR_DELIVERY: return "Ride to FOOD to pick up orders, then to the houses with hearts. D-pad, Circle Pad or hold the stylus.";
    case CR_TEACHER: return "Tap raised hands to answer questions. Tap daydreamers to refocus them!";
    case CR_RIDESHARE: return "Dodge cones, potholes and cars (L/R). Reply to your passenger the way they'd like.";
    case CR_MODEL: return "Strike the called pose (A/B/X/Y) right when the ring hits the lens!";
    case CR_BARISTA: return "Tap the size, drink, milk and topping on the ticket, then Serve!";
    case CR_CHEF: return "Stop the marker in the green zone to chop, cook and plate each dish!";
    default: return "";
    }
}
static int perStar(int c) {
    switch (c) {
    case CR_HOSTESS: return 26;
    case CR_DELIVERY: return 20;
    case CR_TEACHER: return 22;
    case CR_RIDESHARE: return 22;
    case CR_MODEL: return 24;
    case CR_BARISTA: return 18;
    case CR_CHEF: return 14;
    default: return 20;
    }
}
static float introT = 0;

void shiftStart(int c) {
    car = c; lvl = MC().careerLevel; score = 0; stars = 0; done = false; timer = SHIFT_SECS; total = SHIFT_SECS;
    msgT = 0; flashT = 0; shakeT = 0; introT = 2.5f;
    g_shiftHours = CAREERS[c].hours;
    switch (c) {
    case CR_HOSTESS: hostStart(); break;
    case CR_DELIVERY: delStart(); break;
    case CR_TEACHER: teachStart(); break;
    case CR_RIDESHARE: rideStart(); break;
    case CR_MODEL: modelStart(); break;
    case CR_BARISTA: baristaStart(); break;
    case CR_CHEF: chefStart(); break;
    }
}

static const Btn bDone{110, 196, 100, 34};
bool shiftUpdate(const Input& in) {
    if (msgT > 0) msgT -= in.dt;
    if (flashT > 0) flashT -= in.dt;
    if (shakeT > 0) shakeT -= in.dt;
    if (done) {
        if ((in.down & KEY_A) || (in.touch && bDone.in(in.tp))) {
            Sim& me = MC();
            finishShift(me, CAREERS[car].hours, stars * 4.f);
            goalNotify(GE_SHIFT, stars);
            int tips = stars * (6 + lvl * 4);
            if (tips) { G.money += tips; toast("Tips: $" + std::to_string(tips) + "!"); }
            if (stars >= 4) addMoodlet(me, "Killed It at Work", EM_CONFIDENT, 2, 300);
            else if (stars <= 1) addMoodlet(me, "Rough Shift", EM_TENSE, 1, 180);
            me.needs[N_ENERGY] = std::fmax(0.f, me.needs[N_ENERGY] - 25);
            me.needs[N_FUN] = std::fmax(0.f, me.needs[N_FUN] - 8);
            return false;
        }
        return true;
    }
    if (introT > 0) { introT -= in.dt; if ((in.down & KEY_A) || in.touch) introT = 0; return true; }
    timer -= in.dt;
    switch (car) {
    case CR_HOSTESS: hostUpdate(in); break;
    case CR_DELIVERY: delUpdate(in); break;
    case CR_TEACHER: teachUpdate(in); break;
    case CR_RIDESHARE: rideUpdate(in); break;
    case CR_MODEL: modelUpdate(in); break;
    case CR_BARISTA: baristaUpdate(in); break;
    case CR_CHEF: chefUpdate(in); break;
    }
    if (timer <= 0) {
        done = true;
        stars = clampi(score / perStar(car), 0, 5);
        if (stars >= 3) sfxYay(); else sfxNo();
    }
    return true;
}

void shiftDrawTop(float t) {
    switch (car) {
    case CR_RIDESHARE: rideDrawTop(t); break;
    case CR_MODEL: modelDrawTop(t); break;
    default: {
        bool pix = artLayerBegin(0, g_artStyle);
        drawRoom(G, t, -1, -1, false);
        if (pix) artLayerEnd();
        if (car == CR_BARISTA) baristaDrawTop(t);
        if (car == CR_CHEF) chefDrawTop(t);
        break;
    }
    }
    // HUD
    rrect(4, 4, 150, 22, 8, C(UI1, 225));
    char b[64];
    snprintf(b, sizeof b, "%s  %ds", CAREERS[car].titles[lvl], (int)std::ceil(std::fmax(0.f, timer)));
    text(b, 79, 7, 0.4f, C(CREAM), 1);
    rrect(4, 28, 80, 20, 8, C(HOT, 225));
    snprintf(b, sizeof b, "Score %d", score); text(b, 44, 30, 0.42f, C(WHITE), 1);
    if (msgT > 0 && !done) { float w = textW(msg.c_str(), 0.5f) + 20; rrect(200 - w / 2, 200, w, 26, 10, C(255, 255, 255, 235)); text(msg.c_str(), 200, 204, 0.5f, C(BERRY), 1); }
    if (introT > 0 && !done) {
        rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, 120));
        rrect(40, 70, 320, 100, 14, C(255, 250, 252));
        text(CAREERS[car].name, 200, 78, 0.65f, C(HOT), 1);
        auto lines = wrapText(howTo(car), 0.45f, 290);
        for (size_t i = 0; i < lines.size() && i < 3; i++) text(lines[i].c_str(), 200, 108 + i * 18.f, 0.45f, C(UI1), 1);
    }
    if (done) {
        rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, 140));
        rrect(60, 40, 280, 160, 16, C(255, 250, 252));
        text("Shift Over!", 200, 50, 0.75f, C(HOT), 1);
        for (int i = 0; i < 5; i++) {
            float x = 120 + i * 40.f;
            if (i < stars) { sparkle(x, 110, 14 + std::sin(t * 4 + i) * 2, C(GOLD)); circle(x, 110, 4, C(255, 240, 180)); }
            else sparkle(x, 110, 8, C(220, 210, 220));
        }
        static const char* VERDICT[6] = {"Yikes... tomorrow's a new day.", "You survived!", "Solid shift.", "Great work!", "The boss is impressed!", "Employee of the Month!!"};
        text(VERDICT[stars], 200, 140, 0.5f, C(UI1), 1);
        snprintf(b, sizeof b, "Score %d   Tips $%d", score, stars * (6 + lvl * 4));
        text(b, 200, 164, 0.45f, C(BERRY), 1);
    }
}

void shiftDrawBottom(float t) {
    switch (car) {
    case CR_HOSTESS: bottomBackdrop(t); hostDrawBottom(t); break;
    case CR_DELIVERY: delDrawBottom(t); break;
    case CR_TEACHER: teachDrawBottom(t); break;
    case CR_RIDESHARE: rideDrawBottom(t); break;
    case CR_MODEL: modelDrawBottom(t); break;
    case CR_BARISTA: bottomBackdrop(t); baristaDrawBottom(t); break;
    case CR_CHEF: chefDrawBottom(t); break;
    }
    rect(0, 0, BOT_W, 22, C(UI1, 235));
    char b[48]; snprintf(b, sizeof b, "Shift: %ds left", (int)std::ceil(std::fmax(0.f, timer)));
    text(b, 6, 3, 0.42f, C(CREAM));
    snprintf(b, sizeof b, "Score %d", score); text(b, 314, 3, 0.42f, C(GOLD), 2);
    if (done) { rect(0, 22, BOT_W, BOT_H - 22, C(0, 0, 0, 150)); button(bDone, "Clock Out", false, 2, 0.55f); }
    else if (introT > 0) { rect(0, 22, BOT_W, BOT_H - 22, C(0, 0, 0, 120)); text("Tap or press A to start!", 160, 110, 0.55f, C(WHITE), 1); }
}
