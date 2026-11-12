#include "lifeext.hpp"
// Backyard garden, gift giving and the scrapbook of memories (see extras.hpp)
#include "extras.hpp"
#include "goals.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "social.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "cutscene.hpp"
#include "drama.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace col;

// ================================================================ garden
struct CropDef { const char* name; int seed, days, sell; RGB col, leaf; };
static const CropDef CROPS[NUM_CROPS] = {
    {"Strawberries", 8, 2, 30, {236, 60, 90}, {90, 170, 90}},
    {"Tomatoes", 6, 2, 22, {230, 70, 50}, {80, 160, 80}},
    {"Roses", 12, 3, 45, {250, 90, 140}, {70, 140, 80}},
    {"Sunflowers", 8, 3, 32, {255, 205, 60}, {100, 170, 70}},
    {"Lavender", 10, 3, 36, {170, 130, 230}, {110, 160, 120}},
    {"Pumpkins", 15, 4, 70, {250, 150, 50}, {90, 150, 70}},
};
const char* CROP_NAMES[NUM_CROPS] = {"Strawberries", "Tomatoes", "Roses", "Sunflowers", "Lavender", "Pumpkins"};
const char* GIFT_NAMES[NUM_GIFTS] = {"Strawberries", "Tomatoes", "Roses", "Sunflowers", "Lavender", "Pumpkin", "Chocolates"};

int cropSeedPrice(int c) { return CROPS[c].seed; }
int cropDays(int c) { return CROPS[c].days; }
static bool ready(const Plot& p) { return p.crop && p.stage >= CROPS[p.crop - 1].days; }
int gardenReady() { int n = 0; for (const Plot& p : G.plots) if (ready(p)) n++; return n; }
int gardenPlanted() { int n = 0; for (const Plot& p : G.plots) if (p.crop) n++; return n; }
bool gardenNeedsWater() { for (const Plot& p : G.plots) if (p.crop && !ready(p) && !p.watered) return true; return false; }

bool gardenPlant(int c) {
    if (c < 0 || c >= NUM_CROPS) return false;
    for (Plot& p : G.plots) {
        if (p.crop) continue;
        if (G.money < CROPS[c].seed) { toast("Not enough money for seeds!"); sfxNo(); return false; }
        G.money -= CROPS[c].seed;
        p = Plot{(uint8_t)(c + 1), 0, 0, 3};
        char b[96]; snprintf(b, sizeof b, "Planted %s! Water daily - ready in %d days.", CROPS[c].name, CROPS[c].days);
        toast(b); sfxTap();
        MC().needs[N_FUN] = std::fmin(100.f, MC().needs[N_FUN] + 4);
        return true;
    }
    toast("Every garden bed is full!"); sfxNo();
    return false;
}
void gardenWater() {
    int n = 0;
    for (Plot& p : G.plots) if (p.crop && !ready(p) && !p.watered) { p.watered = 1; n++; }
    if (!n) { toast("Nothing needs watering right now."); return; }
    Sim& me = MC();
    me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 6);
    me.needs[N_HYGIENE] = std::fmax(0.f, me.needs[N_HYGIENE] - 3);
    addMoodlet(me, "Green Thumb", EM_HAPPY, 1, 120);
    toast("The garden is watered. Grow, babies, grow!");
    sfx(500, 900, 150);
}
int gardenHarvest() {
    int n = 0;
    for (Plot& p : G.plots) {
        if (!ready(p)) continue;
        int k = p.crop - 1;
        int amt = p.health >= 3 ? 2 : 1;   // healthy plants give a bigger harvest
        G.basket[k] = (uint8_t)std::min(99, G.basket[k] + amt);
        n += amt;
        p = Plot();
    }
    if (!n) { toast("Nothing is ready to harvest yet."); return 0; }
    char b[64]; snprintf(b, sizeof b, "Harvested %d goodies into your basket!", n);
    toast(b); sfxYay();
    addMoodlet(MC(), "Harvest Time", EM_HAPPY, 1, 180);
    skillGain(MC(), SK_CREATIVITY, 1.5f);
    if (!hasMemory(MEM_HOME, -2, -2)) addMemory(MEM_HOME, -2, -2, "First harvest from my very own garden!");
    return n;
}
int basketCount(int k) { return (k >= 0 && k < NUM_GIFTS) ? G.basket[k] : 0; }
int basketTotal() { int n = 0; for (int k = 0; k < NUM_GIFTS; k++) n += G.basket[k]; return n; }
int basketSellValue() { int v = 0; for (int k = 0; k < NUM_CROPS; k++) v += G.basket[k] * CROPS[k].sell; return v; }
int sellHarvest() {
    int v = basketSellValue();
    if (!v) { toast("Your basket is empty - grow something first!"); sfxNo(); return 0; }
    for (int k = 0; k < NUM_CROPS; k++) G.basket[k] = 0;
    G.money += v;
    char b[64]; snprintf(b, sizeof b, "Sold your harvest at the farm stand for $%d!", v);
    toast(b); sfxYay();
    return v;
}
void gardenDaily() {
    const int season = seasonOf(G.day);
    const bool rain = G.weather == WX_RAIN || G.weather == WX_STORM;
    for (Plot& p : G.plots) {
        if (!p.crop) continue;
        if (ready(p)) { p.watered = 0; continue; }
        bool wet = p.watered || rain;
        if (wet && !(season == 3 && G.weather == WX_SNOW && !p.watered)) p.stage++;
        else if (!wet) {
            if (p.health) p.health--;
            if (!p.health) { char b[64]; snprintf(b, sizeof b, "Oh no, your %s wilted...", CROPS[p.crop - 1].name); toast(b); p = Plot(); continue; }
        }
        if (G.weather == WX_HEAT && !p.watered && p.health) p.health--;
        p.watered = 0;
    }
}

void gardenDraw(float t) {
    // a row of six little raised beds along the back fence of the yard
    for (int i = 0; i < NUM_PLOTS; i++) {
        const Plot& p = G.plots[i];
        float x = 6 + i * 31.f, y = 161;
        rrect(x - 1, y - 1, 30, 17, 3, C(150, 100, 70));
        rrect(x + 1, y + 1, 26, 13, 2, C(p.watered ? RGB{92, 62, 44} : RGB{122, 84, 58}));
        if (!p.crop) { for (int k = 0; k < 2; k++) circle(x + 8 + k * 12, y + 9, 1.2f, C(80, 54, 38)); continue; }
        const CropDef& c = CROPS[p.crop - 1];
        float g = std::min(1.f, (float)p.stage / c.days);
        u32 L = C(p.health <= 1 ? RGB{170, 160, 90} : c.leaf);
        for (int k = 0; k < 2; k++) {
            float cx = x + 8 + k * 12, sway = std::sin(t * 2 + i + k) * 0.8f, base = y + 8;
            if (g <= 0.01f) { circle(cx, base, 1.6f, C(90, 140, 70)); continue; }   // seeds just sprouting
            float h = 4 + g * 14;
            line(cx, base, cx + sway, base - h, 1.6f, L);
            ellipse(cx - 3 + sway, base - h * 0.6f, 5, 3, L); ellipse(cx + 3 + sway, base - h * 0.45f, 5, 3, L);
            if (g >= 1.f) {   // ready: fruit / blooms
                float fy = base - h;
                switch (p.crop) {
                case 3: case 5: circle(cx + sway, fy, 3.2f, C(c.col)); circle(cx + sway, fy, 1.4f, C(shade(c.col, 0.8f))); break;
                case 4: for (int q = 0; q < 6; q++) { float a = q * 1.047f; circle(cx + sway + std::cos(a) * 2.6f, fy + std::sin(a) * 2.6f, 1.6f, C(c.col)); } circle(cx + sway, fy, 1.6f, C(120, 80, 40)); break;
                case 6: ellipse(cx, base - 1, 9, 7, C(c.col)); break;
                default: circle(cx + sway - 2, fy + 3, 2.2f, C(c.col)); circle(cx + sway + 2, fy + 5, 2.2f, C(c.col)); break;
                }
            }
        }
        if (g >= 1.f) sparkle(x + 26, y - 4, 2 + std::fabs(std::sin(t * 3 + i)) * 2, C(GOLD));
        else if (!p.watered) { circle(x + 25, y - 1, 2.2f, C(120, 190, 250, 220)); tri(x + 23, y - 1, x + 27, y - 1, x + 25, y - 5, C(120, 190, 250, 220)); }
    }
}

void giveGift(int k, int t) {
    if (k < 0 || k >= NUM_GIFTS || G.basket[k] == 0 || t < 0 || t >= (int)G.sims.size()) return;
    G.basket[k]--;
    Sim& me = MC();
    dramaGiftReaction(t, k);
    const Sim& s = G.sims[t];
    bool romance = canRomance(G.mc, t);
    int fr = 6, ro = 0;
    const char* reply = "Aww, for me? Thank you!";
    switch (k) {
    case 0: fr = 6; if (s.hasTrait(TR_FOODIE)) fr += 4; reply = "Fresh strawberries?! Yum!"; break;
    case 1: fr = 2; reply = "...a tomato? Um, thanks? Haha."; if (s.hasTrait(TR_GOOFBALL)) { fr = 6; reply = "A TOMATO! Iconic. I love you."; } break;
    case 2: if (romance) { fr = 4; ro = 12; reply = "Roses?! You're making me blush!"; } else { fr = 8; reply = "Beautiful roses, thank you!"; } if (s.hasTrait(TR_ROMANTIC)) ro += 4; break;
    case 3: fr = 10; reply = "Sunflowers are my favorite! Best friend ever."; break;
    case 4: fr = 7; reply = "Lavender smells so calming. Thank you!"; break;
    case 5: fr = 8; reply = "A whole pumpkin? Pumpkin pie time!"; if (s.hasTrait(TR_FOODIE)) fr += 4; break;
    case 6: fr = 6; ro = romance ? 6 : 0; reply = "Chocolates! You know me so well."; if (s.hasTrait(TR_FOODIE)) fr += 3; break;
    }
    changeRel(G.mc, t, fr, ro);
    say(G.mc, std::string("I got you ") + GIFT_NAMES[k] + "!");
    say(t, reply);
    addMoodlet(me, "Gift Giver", EM_HAPPY, 1, 180);
    G.rep = (int16_t)std::min(100, G.rep + 1);
    if (G.date.active && G.date.partner == t) G.date.score += 15;
    fxBurst(200, 90, 10); sfxYay();
    goalNotify(GE_SOCIAL, SO_GIFT);
}

// ================================================================ scrapbook
static const RGB MEM_COLS[8][2] = {{{255, 190, 214}, {255, 228, 238}}, {{190, 220, 255}, {236, 246, 255}}, {{255, 220, 140}, {255, 244, 214}},
                                   {{214, 190, 255}, {244, 236, 255}}, {{170, 230, 210}, {230, 250, 244}}, {{255, 200, 150}, {255, 236, 220}},
                                   {{255, 170, 200}, {200, 230, 255}}, {{200, 230, 170}, {244, 252, 236}}};

void addMemory(int type, int a, int b, const std::string& text) {
    Memory m; m.day = G.day; m.type = (uint8_t)type; m.a = (int16_t)a; m.b = (int16_t)b; m.text = text;
    G.memories.push_back(m);
    {   // the Gazette covers real milestones in Maple Valley
        static const char* KEY[] = {"arried", "edding", "ngaged", "born", "Born", "baby", "Baby", "romot", "raduat", "Moved into", "passed", "Divorc", "broke up", "Broke up", "irthday", "Won", "won", "Crowned", nullptr};
        if (type != MEM_FUN || text.find("Won") != std::string::npos) for (int k = 0; KEY[k]; k++) if (text.find(KEY[k]) != std::string::npos) {
            std::string who = (a >= 0 && a < (int)G.sims.size()) ? G.sims[a].name.substr(0, G.sims[a].name.find(' ')) + ": " : "";
            lxNews((who + text).substr(0, 51)); break;
        }
    }
    if (G.memories.size() > 80) G.memories.erase(G.memories.begin());
    toast("New scrapbook memory saved!");
}
void addMemoryOnce(int type, int a, int b, const std::string& text) {
    for (const Memory& m : G.memories) if (m.text == text) return;
    addMemory(type, a, b, text);
}
bool hasMemory(int type, int a, int b) {
    for (const Memory& m : G.memories) if (m.type == type && m.a == a && m.b == b) return true;
    return false;
}

static int sbSel = -1, sbPage = 0;
static Btn sbRow(int i) { return Btn{6, 28 + i * 34.f, 308, 30}; }
static const Btn sbPrev{150, 208, 76, 28}, sbNext{236, 208, 80, 28}, sbMovies{66, 208, 78, 28};
static bool sbShowMovies = false;
static Btn movieB(int i) { return Btn{8 + (i % 2) * 156.f, 30 + (i / 2) * 44.f, 148, 38}; }

void scrapbookUpdate(const Input& in) {
    int n = (int)G.memories.size();
    if (sbSel >= n) sbSel = -1;
    int pages = std::max(1, (n + 4) / 5);
    if (sbPage >= pages) sbPage = pages - 1;
    if (in.down & KEY_LEFT) { sbPage = std::max(0, sbPage - 1); return; }
    if (in.down & KEY_RIGHT) { sbPage = std::min(pages - 1, sbPage + 1); return; }
    if (!in.touch) return;
    if (sbMovies.in(in.tp)) { sbShowMovies = !sbShowMovies; sfxTap(); return; }
    if (sbShowMovies) {
        for (int i = 0; i < CS_COUNT; i++) if (movieB(i).in(in.tp)) { if (G.csSeen & (1u << i)) cutscenePlay(i, true); else sfxNo(); return; }
        return;
    }
    if (sbPrev.in(in.tp)) { sbPage = std::max(0, sbPage - 1); sfxTap(); return; }
    if (sbNext.in(in.tp)) { sbPage = std::min(pages - 1, sbPage + 1); sfxTap(); return; }
    for (int i = 0; i < 5; i++) {
        int k = n - 1 - (sbPage * 5 + i);   // newest first
        if (k >= 0 && sbRow(i).in(in.tp)) { sbSel = k; sfxTap(); return; }
    }
}

static void sticker(int type, float x, float y, float t) {
    switch (type) {
    case MEM_LOVE: heart(x, y, 9, C(HOT)); heart(x + 16, y + 10, 5, C(BLUSH)); break;
    case MEM_FAMILY: circle(x, y, 8, C(255, 240, 200)); circle(x - 2, y - 1, 1.2f, C(UI1)); circle(x + 3, y - 1, 1.2f, C(UI1)); heart(x + 14, y - 8, 4, C(HOT)); break;
    case MEM_CAREER: sparkle(x, y, 11, C(GOLD)); circle(x, y, 3, C(255, 240, 180)); break;
    case MEM_GOAL: rrect(x - 7, y - 8, 14, 10, 3, C(GOLD)); rect(x - 2, y + 2, 4, 5, C(GOLD)); rect(x - 6, y + 7, 12, 3, C(shade(GOLD, 0.8f))); break;
    case MEM_FRIEND: for (int i = 0; i < 3; i++) sparkle(x + i * 9, y + (i % 2) * 6, 4 + std::sin(t * 3 + i), C(255, 200, 240)); break;
    case MEM_FUN: for (int i = 0; i < 8; i++) rect(x - 10 + (i * 7) % 22, y - 8 + (i * 5) % 16, 3, 3, C(i % 2 ? HOT : RGB{90, 200, 230})); break;
    case MEM_LIFE: ellipse(x, y, 12, 15, C(HOT)); line(x, y + 7, x + 2, y + 16, 1, C(UI1)); ellipse(x + 12, y + 3, 10, 13, C(LILAC)); line(x + 12, y + 9, x + 11, y + 18, 1, C(UI1)); break;
    default: tri(x - 9, y, x + 9, y, x, y - 9, C(HOT)); rect(x - 7, y, 14, 10, C(CREAM)); rect(x - 2, y + 4, 4, 6, C(UIM)); break;
    }
}

void scrapbookDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(255, 240, 246), C(236, 228, 250));
    for (int i = 0; i < 12; i++) heart(20 + (i * 67) % 380, 18 + (i * 41) % 210, 4, C(255, 200, 220, 120));
    rect(0, 0, TOP_W, 20, C(UI1));
    text("My Scrapbook", 6, 2, 0.48f, C(WHITE));
    char b[64]; snprintf(b, sizeof b, "%d memories", (int)G.memories.size()); text(b, 394, 2, 0.45f, C(GOLD), 2);
    int n = (int)G.memories.size();
    if (!n) {
        text("No memories yet!", 200, 90, 0.7f, C(BERRY), 1);
        text("Fall in love, get promoted, win contests, grow up...", 200, 124, 0.45f, C(UI2), 1);
        text("your biggest moments land here automatically.", 200, 142, 0.45f, C(UI2), 1);
        return;
    }
    if (sbSel >= n) sbSel = -1;
    int k = sbSel >= 0 ? sbSel : n - 1;
    const Memory& m = G.memories[k];
    // the polaroid
    float px = 112, py = 26, pw = 176, ph = 208;
    rrect(px + 4, py + 4, pw, ph, 4, C(0, 0, 0, 40));
    rrect(px, py, pw, ph, 4, C(255, 255, 252));
    const RGB* bg = MEM_COLS[m.type % 8];
    grad(px + 10, py + 10, pw - 20, 140, C(bg[0]), C(bg[1]));
    bool two = m.a >= 0 && m.b >= 0 && m.a < (int)G.sims.size() && m.b < (int)G.sims.size();
    if (two) { drawSimPortrait(G.sims[m.a], px + 58, py + 82, 34, t); drawSimPortrait(G.sims[m.b], px + 118, py + 82, 34, t); }
    else if (m.a >= 0 && m.a < (int)G.sims.size()) drawSimPortrait(G.sims[m.a], px + 88, py + 82, 46, t);
    else if (m.b >= 0 && m.b < (int)G.sims.size()) drawSimPortrait(G.sims[m.b], px + 88, py + 82, 46, t);
    else drawSimPortrait(MC(), px + 88, py + 82, 46, t);
    sticker(m.type, px + pw - 22, py + 24, t);
    // tape
    quad(px + 60, py - 8, px + 116, py - 6, px + 114, py + 8, px + 58, py + 6, C(255, 240, 200, 200));
    auto lines = wrapText(m.text, 0.42f, pw - 16);
    for (size_t i = 0; i < lines.size() && i < 2; i++) text(lines[i].c_str(), px + pw / 2, py + 156 + i * 15.f, 0.42f, C(80, 50, 70), 1);
    text(dateStr(m.day).c_str(), px + pw / 2, py + 188, 0.36f, C(160, 130, 150), 1);
    // side decorations
    sparkle(60, 60, 6 + std::sin(t * 2) * 2, C(GOLD)); sparkle(340, 190, 5 + std::cos(t * 2) * 2, C(GOLD));
    snprintf(b, sizeof b, "%d / %d", n - k, n); text(b, 340, 30, 0.42f, C(UI2), 1);
}

void scrapbookDrawBottom(float t) {
    (void)t;
    button(sbMovies, sbShowMovies ? "Photos" : "Movies", sbShowMovies, 2, 0.42f);
    if (sbShowMovies) {
        for (int i = 0; i < CS_COUNT; i++) {
            bool seen = G.csSeen & (1u << i);
            button(movieB(i), seen ? CS_NAMES[i] : "??? (locked)", false, seen ? i % 4 : 1, 0.38f);
        }
        return;
    }
    int n = (int)G.memories.size();
    if (sbSel >= n) sbSel = -1;
    if (sbPage > n / 5) sbPage = 0;
    static const char* TYPE_N[8] = {"Love", "Family", "Career", "Goal", "Friends", "Fun", "Milestone", "Home"};
    for (int i = 0; i < 5; i++) {
        int k = n - 1 - (sbPage * 5 + i);
        if (k < 0) break;
        const Memory& m = G.memories[k];
        Btn r = sbRow(i);
        bool sel = k == (sbSel >= 0 ? sbSel : n - 1);
        rrect(r.x, r.y, r.w, r.h, 8, C(sel ? RGB{255, 255, 255} : RGB{255, 236, 244}, 235));
        const RGB* bg = MEM_COLS[m.type % 8];
        rrect(r.x + 4, r.y + 4, 56, r.h - 8, 6, C(bg[0]));
        text(TYPE_N[m.type % 8], r.x + 32, r.y + 7, 0.36f, C(UI1), 1);
        std::string s = m.text;
        if (textW(s.c_str(), 0.4f) > 236) { while (s.size() > 4 && textW((s + "...").c_str(), 0.4f) > 236) s.pop_back(); s += "..."; }
        text(s.c_str(), r.x + 66, r.y + 7, 0.4f, C(UI1));
    }
    int pages = std::max(1, (n + 4) / 5);
    char b[24]; snprintf(b, sizeof b, "Page %d/%d", sbPage + 1, pages);
    text(b, 110, 214, 0.4f, C(WHITE), 1);
    button(sbPrev, "Newer", false, 1, 0.42f);
    button(sbNext, "Older", false, 1, 0.42f);
}
