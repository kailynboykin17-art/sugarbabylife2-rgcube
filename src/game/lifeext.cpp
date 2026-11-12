#include "designer.hpp"
#include "lifeplus2.hpp"
#include "settings2.hpp"
#include "townpulse.hpp"
#include "choices.hpp"
#include "famdrama.hpp"
#include "dynasty.hpp"
#include "dailylife.hpp"
#include "growup.hpp"
#include "kidlife.hpp"
#include "tween.hpp"
#include "goals.hpp"
#include "people.hpp"
#include "worldsim.hpp"
#include "lifeext.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "lifestory.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cmath>
#include "draw.hpp"
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack
using namespace col;

static const char* CIRCLES[CI_COUNT] = {"Fashion Clique", "Artists", "Jocks", "Scholars", "Party Crowd", "Family Folks"};
static const char* PARTY_TYPES[6] = {"House Party", "Sleepover", "Dinner Party", "Pool Party", "Birthday Bash", "Housewarming"};
static const char* DRESS[5] = {"Casual", "Y2K Glam", "Formal", "Pajamas", "Swimwear"};
static const char* FOOD[4] = {"Chips & Soda ($20)", "Pizza Night ($50)", "Home-Cooked Feast ($80)", "Catered Fancy ($150)"};
static const int FOOD_COST[4] = {20, 50, 80, 150};
static const char* TRAD[5] = {"Sunday Dinner", "Game Night (Fri)", "Movie Night (Thu)", "Pancake Saturday", "Annual Reunion"};
static const char* JOBS[4] = {"", "Firefighter", "Ghost Hunter", "Architect"};
static const char* JOBTITLES[4][5] = {{}, {"Probie", "Firefighter", "Engineer", "Lieutenant", "Fire Chief"},
    {"Ghost Intern", "Paranormal Tech", "Ghost Hunter", "Spirit Expert", "Legendary Medium"},
    {"Drafter", "Junior Architect", "Architect", "Senior Architect", "Starchitect"}};

struct Invite { int16_t who = -1, room = -1; uint16_t day = 0; uint8_t hour = 0, kind = 0; };
struct Gossip { char text[56] = {}; uint8_t circle = 0, spread = 0, truth = 1, age = 0; int8_t sign = -1; };
struct LX {
    uint32_t magic = 0x4C584531;
    uint8_t npcsAdded = 0, traditions = 0, tradStreak = 0, cycles = 1;
    int8_t rep[CI_COUNT] = {}; char why[CI_COUNT][28] = {}; int8_t trend[CI_COUNT] = {};
    Invite inv[4];
    Gossip gos[8];
    uint8_t job = 0, jobLvl = 0, jobPerf = 0, task = 0, taskScore = 0;
    uint8_t crisis[128] = {};   // bit0 mid-life crisis had, bit1 rebel phase had
    uint16_t promYear = 0xFFFF, cramps = 0;
    uint8_t clean = 80, bedOf[128] = {};   // bedOf: 0 unassigned, 1..4 = bedroom
    uint16_t xs[6] = {};
    // v2 fields (appended: older files keep defaults)
    uint8_t npcFix = 0, trendJudge = 1, rentPct = 100, pad2 = 0;
    char news[12][52] = {}; uint16_t newsDay[12] = {}; uint8_t newsN = 0, pad3 = 0;
    int16_t closedRoom = -1; uint16_t closedUntil = 0, contestDay = 0xFFFF;
    uint8_t wins = 0, entries = 0;
    // v3 fields
    uint16_t xs2[6] = {};
    int16_t group[3] = {-1, -1, -1};
    uint8_t clique = 0, teenMiles = 0, detentions = 0, pad4 = 0;   // teenMiles bits: crush, permit, homecoming, job, first party, graduated
    // v4: calendar
    struct Hol { uint8_t used = 0, kind = 0; uint16_t day = 0; } hol[8];   // custom holidays (repeat yearly on day % 28)
    struct Get { uint8_t used = 0, type = 0, len = 1, acts[3] = {0xFF, 0xFF, 0xFF}; uint16_t day = 0; } get[4];
    uint8_t tradDone = 0, tradDay = 0;
    uint8_t bodyFix = 0;
    // outfit categories: [sim][cat] = outfit+1 (0 = automatic default); base[sim] = everyday outfit + 1
    uint8_t wear[128][8] = {}, base[128] = {}, lastWorn[128] = {}, autoWear = 1;
    uint8_t bc[128] = {}, birthPlan = 0;
    uint8_t tat[128] = {};
    uint64_t own3 = 0, own4 = 0;   // outfit ownership 128..255
    uint8_t hidePurse = 0, crushBack = 0; uint16_t toys = 0; int16_t crush = -1;   // birth control per sim; 0 hospital, 1 home with a midwife, 2 water birth
};
static LX X;
static int partyType = 0, partyDress = 0, partyFood = 0; static bool wantParty = false, partyPending = false;
static int lastHour = -1;
static int pickWho = -1;
static const char* TRENDS[8] = {"Y2K Streetwear", "Coquette", "Preppy", "Vintage Thrift", "Goth", "Old Money", "Nightlife Glam", "Sporty Casual"};
int lxTrend() { return (int)((G.day / 7 * 2654435761u) >> 7) % 8; }
const char* lxTrendName() { return TRENDS[lxTrend()]; }
bool lxTrendy(int o) {
    if (o <= 0 || o >= NUM_OUTFITS) return false;
    const OutfitDef& d = OUTFITS[o]; const int lum = d.top.r + d.top.g + d.top.b;
    switch (lxTrend()) {
    case 0: return d.bt == BT_MINI || d.topStyle == TS_CROP || d.bt == BT_JEANS;
    case 1: return d.topPattern == 6 || (d.top.r > 220 && d.top.g < 200 && d.top.b > 150);
    case 2: return d.topStyle == TS_POLO || d.topStyle == TS_BUTTON || d.bt == BT_PLEATED || d.topStyle == TS_CARDIGAN;
    case 3: return d.bt == BT_JEANS || d.topStyle == TS_JACKET || d.topPattern == 4;
    case 4: return lum < 180;
    case 5: return d.topStyle == TS_BLAZER || d.topStyle == TS_SWEATER || d.topStyle == TS_CARDIGAN;
    case 6: return d.topStyle == TS_DRESS || d.topPattern == 7 || d.shoeStyle == SS_HEELS;
    case 7: return d.topStyle == TS_HOODIE || d.topStyle == TS_JERSEY || d.bt == BT_SWEATS || d.shoeStyle == SS_SNEAKERS;
    }
    return false;
}
void lxNews(const std::string& h) {
    for (int i = 0; i < X.newsN; i++) if (h == X.news[i] && G.day - X.newsDay[i] < 4) return;   // no repeat spam
    if (X.newsN == 12) { memmove(X.news[0], X.news[1], sizeof X.news[0] * 11); memmove(&X.newsDay[0], &X.newsDay[1], sizeof X.newsDay[0] * 11); X.newsN = 11; }
    snprintf(X.news[X.newsN], 52, "%s", h.c_str()); X.newsDay[X.newsN++] = (uint16_t)G.day;
}
bool lxBlocks(int room, std::string* why) {
    if (X.closedRoom < 0 || room != X.closedRoom || G.day >= X.closedUntil) return false;
    if (why) *why = std::string(ROOM_NAMES[room % R_COUNT]) + " is closed for renovation. Check the News app.";
    return true;
}
static bool shiftOn = false;
void lxDrawWork(float t) {
    if (!shiftOn || !X.job) return;
    rect(30, 20, 340, 150, C(20, 20, 30, 230));
    switch (X.job) {
    case 1:   // firefighter: burning house, truck, water arc
        rect(200, 70, 120, 90, C(150, 90, 70)); for (int i = 0; i < 7; i++) { const float h = 20 + std::sin(t * 9 + i) * 8; circle(212 + i * 16.f, 70 - h * 0.5f, 10 + std::sin(t * 7 + i * 2) * 3, C(255, 120 + i * 15, 30, 220)); }
        rect(50, 120, 110, 40, C(210, 30, 40)); circle(70, 160, 9, C(30, 30, 30)); circle(140, 160, 9, C(30, 30, 30)); rect(60, 110, 60, 10, C(230, 230, 230));
        for (int i = 0; i < 10; i++) circle(160 + i * 6.f, 112 - std::sin(i * 0.33f) * 26, 3, C(120, 190, 255, 200));
        break;
    case 2:   // ghost hunter: dark manor, floating ghost, EMF meter
        rect(150, 50, 120, 110, C(50, 40, 70)); for (int i = 0; i < 3; i++) rect(162 + i * 36.f, 70, 20, 26, C(255, 230, 120, 140));
        { const float gx = 120 + std::sin(t * 1.5f) * 60, gy = 60 + std::sin(t * 2.3f) * 10; circle(gx, gy, 14, C(240, 240, 255, 170)); rect(gx - 14, gy, 28, 16, C(240, 240, 255, 170)); circle(gx - 5, gy - 2, 2, C(20, 20, 30)); circle(gx + 5, gy - 2, 2, C(20, 20, 30)); }
        rect(300, 120, 40, 40, C(40, 40, 40)); for (int i = 0; i < 5; i++) rect(305 + i * 7.f, 150 - i * 5.f, 5, 5.f + i * 5, C(i < 1 + (int)(std::fabs(std::sin(t * 3)) * 5) ? RGB{90, 255, 120} : RGB{60, 60, 60}));
        break;
    case 3:   // architect: blueprint table with lines being drawn
        rect(60, 50, 280, 110, C(30, 70, 140)); for (int i = 0; i < 14; i++) rect(60 + i * 20.f, 50, 1, 110, C(80, 120, 190)); for (int i = 0; i < 6; i++) rect(60, 50 + i * 20.f, 280, 1, C(80, 120, 190));
        { const float p = std::fmod(t * 0.4f, 1.f); rect(110, 130, 180 * p, 2, C(255, 255, 255)); rect(110, 70, 2, 60 * std::min(1.f, p * 2), C(255, 255, 255)); rect(290, 70, 2, 60 * std::max(0.f, p * 2 - 1), C(255, 255, 255)); }
        break;
    }
    char b[64]; snprintf(b, sizeof b, "%s - task %d of 3", JOBTITLES[X.job][std::min(4, (int)X.jobLvl)], X.task + 1);
    textOutline(b, 200, 24, 0.5f, C(255, 255, 255), C(UI_INK), 1);
}
// ---------------------------------------------------------------- Seasons calendar
static const char* CHOL[4] = {"Self-Care Day", "Family Fun Day", "Party Day", "Love Day"};
static const char* GTYPE[4] = {"Summer Camp", "Weekend Retreat", "Beach Getaway", "Snowy Cabin"};
static const char* GACT[8] = {"Archery", "Kayaking", "Diving", "Volleyball", "Campfire Stories", "Crafts", "Hiking", "Spa Morning"};
static const bool GOUT[8] = {true, true, true, true, true, false, true, false};
static const char* TRADS[4][3] = {{"Open Presents", "Grand Meal", "Tell Stories"}, {"Wear a Costume", "Trick-or-Treat", "Carve Pumpkins"}, {"Spa Bath", "Journal", "Nap"}, {"Board Games", "Family Photo", "Grand Meal"}};
static int calView = 7, calSel = -1, calOff = 0;
int lxForecast(int day) {
    uint32_t h = (uint32_t)day * 2654435761u ^ 0x5EA50Du; h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
    const int r = (int)(h % 100);
    switch (seasonOf(day)) {
    case 0: return r < 45 ? WX_SUNNY : r < 70 ? WX_CLOUDY : r < 94 ? WX_RAIN : WX_STORM;
    case 1: return r < 50 ? WX_SUNNY : r < 75 ? WX_HEAT : r < 87 ? WX_CLOUDY : WX_STORM;
    case 2: return r < 30 ? WX_SUNNY : r < 65 ? WX_CLOUDY : r < 94 ? WX_RAIN : WX_STORM;
    default: return r < 45 ? WX_SNOW : r < 75 ? WX_CLOUDY : WX_SUNNY;
    }
}
static const char* tempWord(int day) { const int w = lxForecast(day), s = seasonOf(day); return w == WX_HEAT ? "Hot" : w == WX_SNOW ? "Freezing" : s == 3 ? "Cold" : s == 1 ? "Warm" : w == WX_RAIN || w == WX_STORM ? "Cool" : "Mild"; }
static int holidayOf(int day, const char** name) {   // -1 none; else tradition set
    const int dd = day % 28;
    if (dd == 25) { *name = "Winterfest"; return 0; }
    if (dd == 18) { *name = "Spooky Day"; return 1; }
    for (const auto& h : X.hol) if (h.used && h.day % 28 == dd) { *name = CHOL[h.kind % 4]; return h.kind % 4 == 0 ? 2 : 3; }
    return -1;
}
static void wxIcon(float x, float y, float s, int w) {
    switch (w) {
    case WX_SUNNY: circle(x, y, s, C(255, 200, 40)); break;
    case WX_HEAT: circle(x, y, s, C(255, 110, 40)); circle(x, y, s * 0.5f, C(255, 220, 80)); break;
    case WX_CLOUDY: circle(x - s * 0.4f, y, s * 0.7f, C(200, 205, 215)); circle(x + s * 0.4f, y, s * 0.8f, C(220, 225, 235)); break;
    case WX_RAIN: circle(x, y - s * 0.2f, s * 0.8f, C(150, 160, 180)); for (int i = -1; i <= 1; i++) rect(x + i * s * 0.5f, y + s * 0.5f, 1.5f, s * 0.6f, C(80, 150, 255)); break;
    case WX_STORM: circle(x, y - s * 0.2f, s * 0.8f, C(100, 100, 120)); rect(x - 1, y + s * 0.3f, 3, s * 0.8f, C(255, 230, 60)); break;
    case WX_SNOW: circle(x, y, s * 0.8f, C(240, 248, 255)); circle(x, y, s * 0.3f, C(170, 200, 255)); break;
    }
}
bool lxCalUpdate(const Input& in) {
    if (calSel < 0) calSel = G.day;
    const touchPosition& tp = in.tp;
    if (in.down & (KEY_LEFT)) calSel--; if (in.down & KEY_RIGHT) calSel++;
    if (in.down & KEY_UP) calSel -= 7; if (in.down & KEY_DOWN) calSel += 7;
    if (in.down & KEY_L) calOff -= calView; if (in.down & KEY_R) calOff += calView;
    if (in.down & KEY_Y) calView = calView == 7 ? 14 : calView == 14 ? 28 : 7;
    if (in.touch) {
        for (int k = 0; k < 3; k++) if (Btn{8 + k * 44.f, 28, 40, 20}.in(tp)) calView = k == 0 ? 7 : k == 1 ? 14 : 28;
        if (Btn{142, 28, 30, 20}.in(tp)) calOff -= calView;
        if (Btn{176, 28, 30, 20}.in(tp)) calOff += calView;
        if (Btn{210, 28, 104, 20}.in(tp) && calSel >= (int)G.day) { sfxTap(); return true; }
    }
    const int start = (int)(G.day / 7) * 7 + calOff;
    if (calSel < (int)G.day / 7 * 7 - 28) calSel = (int)G.day / 7 * 7 - 28;
    if (calSel < start) calOff -= calView; if (calSel >= start + calView) calOff += calView;
    if (start + calOff < 0) calOff = -(int)(G.day / 7) * 7;
    return (in.down & KEY_A) && calSel >= (int)G.day;
}
void lxCalDrawTop(float t) {
    (void)t; char b[48];
    rect(0, 0, TOP_W, TOP_H, C(255, 240, 246));
    const int start = (int)(G.day / 7) * 7 + calOff, rows = calView / 7;
    snprintf(b, sizeof b, "%s  -  %d-Day View", SEASON_NAMES[seasonOf(start)], calView); textOutline(b, 200, 4, 0.55f, C(255, 255, 255), C(UI_INK), 1);
    const float bh = (210.f / rows) - 4;
    for (int d = 0; d < calView; d++) {
        const int day = start + d; const float x = 6 + (d % 7) * 56.f, y = 24 + (d / 7) * (bh + 4);
        const char* hn = nullptr; const int hs = holidayOf(day, &hn);
        if (day == (int)G.day) rrect(x - 3, y - 3, 58, bh + 6, 8, C(255, 210, 60));   // glowing current day
        if (day == calSel) rrect(x - 2, y - 2, 56, bh + 4, 7, C(255, 46, 140));
        rrect(x, y, 52, bh, 6, C(hs >= 0 ? RGB{255, 214, 230} : RGB{255, 255, 255}));
        if (hs >= 0) { rrect(x, y + bh - 12, 52, 12, 4, C(hs == 0 ? RGB{90, 170, 110} : hs == 1 ? RGB{240, 140, 40} : RGB{236, 90, 170})); if (bh > 40) text(hn, x + 26, y + bh - 12, 0.28f, C(255, 255, 255), 1); }
        snprintf(b, sizeof b, "%d", day % 28 + 1); text(b, x + 5, y + 2, calView == 28 ? 0.34f : 0.45f, C(UI_INK));
        if (calView < 28) text(WEEKDAY_NAMES[day % 7], x + 34, y + 3, 0.3f, C(150, 110, 130), 1);
        wxIcon(x + 42, y + (calView == 28 ? 12 : 26), calView == 28 ? 5 : 8, lxForecast(day));
        bool evt = false; for (const Event& e : G.events) if (e.day == day) evt = true; for (const auto& g : X.get) if (g.used && day >= g.day && day < g.day + g.len) evt = true;
        if (evt) sparkle(x + 10, y + bh - 18, 4, C(255, 180, 0));
    }
}
void lxCalDrawBottom(float t) {
    (void)t; char b[64];
    rect(0, 24, BOT_W, BOT_H - 24, C(70, 30, 60));
    for (int k = 0; k < 3; k++) { snprintf(b, sizeof b, "%d", k == 0 ? 7 : k == 1 ? 14 : 28); button(Btn{8 + k * 44.f, 28, 40, 20}, b, calView == (k == 0 ? 7 : k == 1 ? 14 : 28), 0, 0.4f); }
    button(Btn{142, 28, 30, 20}, "<", false, 1, 0.45f); button(Btn{176, 28, 30, 20}, ">", false, 1, 0.45f);
    button(Btn{210, 28, 104, 20}, calSel >= (int)G.day ? "+ Add Event" : "(past day)", false, 2, 0.38f);
    const int day = calSel < 0 ? G.day : calSel, w = lxForecast(day);
    // weather & forecast
    wxIcon(28, 70, 14, w); snprintf(b, sizeof b, "%s %d - %s, %s", WEEKDAY_NAMES[day % 7], day % 28 + 1, WEATHER_NAMES[w], tempWord(day)); text(b, 50, 58, 0.42f, C(255, 255, 255));
    for (int k = 0; k < 7; k++) { wxIcon(56 + k * 24.f, 86, 6, lxForecast(day + k)); snprintf(b, sizeof b, "%s", WEEKDAY_NAMES[(day + k) % 7]); text(std::string(b).substr(0, 2).c_str(), 56 + k * 24.f, 94, 0.26f, C(230, 210, 230), 1); }
    // holiday & traditions
    const char* hn = nullptr; const int hs = holidayOf(day, &hn);
    float y = 110;
    if (hs >= 0) {
        rrect(6, y, 308, 16, 6, C(236, 90, 170)); text(hn, 160, y + 1, 0.42f, C(255, 255, 255), 1); y += 19;
        for (int k = 0; k < 3; k++) {
            const bool done = day == (int)G.day && X.tradDay == (uint8_t)(G.day & 255) && ((X.tradDone >> k) & 1);
            circle(16 + k * 104.f, y + 7, 6, C(done ? RGB{255, 200, 40} : RGB{120, 90, 120})); if (done) text("v", 16 + k * 104.f, y + 1, 0.3f, C(UI_INK), 1);
            text(TRADS[hs][k], 26 + k * 104.f, y + 2, 0.3f, C(255, 255, 255));
        }
        y += 18;
    }
    // schedule
    text("Schedule:", 8, y, 0.36f, C(255, 210, 60)); y += 13;
    auto line = [&](const std::string& s) { if (y < 232) { text(s.substr(0, 52).c_str(), 14, y, 0.32f, C(255, 255, 255)); y += 11; } };
    if (day % 7 < 5) line("8a-3p School / 9a-5p Work");
    if (day % 7 == 3) line("Dance-Off (Dance Studio)"); if (day % 7 == 5) line("Fashion Show (Museum)"); if (day % 7 == 6) { line("Cooking Challenge (Le Bistro)"); line("Bills due"); }
    for (const Event& e : G.events) if (e.day == day) { snprintf(b, sizeof b, "%d:00 %s", e.hour, e.text.c_str()); line(b); }
    for (const auto& g : X.get) if (g.used && day >= g.day && day < g.day + g.len) {
        std::string s = std::string("Getaway: ") + GTYPE[g.type % 4] + " -"; for (int k = 0; k < 3; k++) if (g.acts[k] < 8) s += std::string(" ") + GACT[g.acts[k]];
        line(s); if (GOUT[g.acts[0] % 8] && (lxForecast(day) == WX_RAIN || lxForecast(day) == WX_STORM || lxForecast(day) == WX_SNOW)) line("  ! Bad weather forecast for outdoor plans");
    }
    const TownEvent* ev[4]; const int ne = eventsOn(day, ev, 4); for (int k = 0; k < ne; k++) line(std::string("Festival: ") + ev[k]->name);
    for (size_t i = 0; i < G.sims.size(); i++) { const Sim& s = G.sims[i]; if (s.household && G.aging && STAGE_DAYS[s.stage] != 255 && G.day + (STAGE_DAYS[s.stage] - s.stageDays) == day) line(s.name + "'s birthday!"); }
}
static const char* XS_NAMES[XS_COUNT] = {"Dancing", "Singing", "Painting", "Gaming", "Photography", "Homemaking", "Writing", "Gardening", "Comedy", "Mixology", "Knitting", "Makeup"};
static uint16_t& XSV(int k) { return k < 6 ? X.xs[k] : X.xs2[k - 6]; }   // (used to call itself forever -> freeze)
static const int BEDROOMS[4] = {R_BEDROOM, R_BEDROOM2, R_TEENROOM, R_NURSERY};
void lxSkill(int k, int xp) {
    if (k < 0 || k >= XS_COUNT) return;
    const int before = XSV(k) / 100; XSV(k) = (uint16_t)std::max(0, std::min(1000, XSV(k) + xp));
    if (XSV(k) / 100 > before) { char b[64]; snprintf(b, sizeof b, "%s skill up! Level %d", XS_NAMES[k], XSV(k) / 100); toast(b); sfxYay(); }
}
void lxClean(int amt) { if (amt < 0) { X.clean = (uint8_t)std::max(0, X.clean + amt); return; }   // messes just make it dirtier
    X.clean = (uint8_t)std::min(100, X.clean + amt + XSV(XS_HOME) / 100 * 2); lxSkill(XS_HOME, amt / 2); }
void lxDrawPests(float t) {
#ifndef __3DS__
    if (getenv("PESTS")) X.clean = (uint8_t)(t > 0 ? 5 : X.clean);
#endif
    if (!roomIsHome(G.room) || X.clean >= 40) return;
    const int n = X.clean < 10 ? 10 : X.clean < 25 ? 6 : 0;
    for (int i = 0; i < n; i++) {   // scuttling roaches along the floor
        const float x = std::fmod(i * 61.f + t * (20 + i * 3), 420.f) - 10, y = 214 + (i % 4) * 6;
        rect(x, y, 5, 3, C(60, 36, 20)); rect(x + 4, y + 1, 2, 1, C(40, 24, 12));
    }
    if (X.clean < 10) for (int i = 0; i < 2; i++) { const float x = std::fmod(t * 45 + i * 200, 440.f) - 20; rect(x, 226, 12, 6, C(110, 100, 100)); rect(x + 11, 228, 3, 3, C(130, 120, 120)); rect(x - 6, 229, 6, 1, C(200, 150, 150)); }
    for (int i = 0; i < 4; i++) { const float x = 60 + i * 90.f, y = 150 - std::fmod(t * 12 + i * 9, 30.f); circle(x, y, 2, C(140, 170, 80, 120)); circle(x + 4, y - 6, 1.5f, C(140, 170, 80, 90)); }   // stink lines
}

static std::string first(int s) { const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
static bool okSim(int s) { return s >= 0 && s < (int)G.sims.size() && !simDead(s); }
static bool pay(int a) { if (G.money < a) { toast("Not enough money!"); sfxNo(); return false; } G.money -= a; return true; }

void lxRepAdd(int c, int d, const char* why) {
    if (c < 0 || c >= CI_COUNT) return;
    X.rep[c] = (int8_t)std::max(-100, std::min(100, X.rep[c] + d)); X.trend[c] = (int8_t)std::max(-9, std::min(9, X.trend[c] + (d > 0 ? 1 : -1)));
    snprintf(X.why[c], sizeof X.why[c], "%s", why);
}
int lxRep(int c) { return c >= 0 && c < CI_COUNT ? X.rep[c] : 0; }
int lxSquashRumors() { int n = 0; for (Gossip& g : X.gos) if (g.text[0] && !g.truth) { snprintf(g.text, sizeof g.text, "%s", "(debunked rumor)"); g.truth = 1; g.sign = 0; n++; } return n; }
void lxGossip(const std::string& text, bool truth, int circle, int sign) {
    int k = 0; for (int i = 1; i < 8; i++) if (X.gos[i].age > X.gos[k].age || !X.gos[i].text[0]) { k = i; if (!X.gos[i].text[0]) break; }
    if (!X.gos[0].text[0]) k = 0;
    Gossip& g = X.gos[k]; g = Gossip{}; snprintf(g.text, sizeof g.text, "%s", text.c_str());
    g.circle = (uint8_t)circle; g.truth = truth; g.sign = (int8_t)sign; g.spread = 1;
}

int lxRentShare(int price) {
    int adults = 0; for (const Sim& s : G.sims) if (s.household && s.stage >= ST_YA) adults++;
    price = price * X.rentPct / 100;
    return adults > 1 ? price / adults + (price % adults) : price;   // roommates & partners pay their share
}

// ---------------------------------------------------------------- menus
LxMenu lxApp(int which) {
    LxMenu v; char b[96];
    switch (which) {
    case 0:
        v.push_back({"Plan a gathering - pick a type:", 10999});
        for (int k = 0; k < 6; k++) v.push_back({PARTY_TYPES[k], 10010 + k});
        break;
    case 1: {
        bool any = false;
        for (int i = 0; i < 4; i++) { const Invite& I = X.inv[i]; if (I.who < 0 || !okSim(I.who)) continue; any = true;
            snprintf(b, sizeof b, "%s: %s @ %d:00 (day %d)", first(I.who).c_str(), ROOM_NAMES[I.room % R_COUNT], I.hour, I.day + 1); v.push_back({b, 10999});
            v.push_back({"  Accept & Go Now", 10100 + i}); v.push_back({"  Decline", 10110 + i}); v.push_back({"  Ask for Tomorrow", 10120 + i}); }
        if (!any) v.push_back({"No invitations right now.", 10999});
        break;
    }
    case 2:
        for (int c = 0; c < CI_COUNT; c++) { snprintf(b, sizeof b, "%s: %+d %s %s", CIRCLES[c], X.rep[c], X.trend[c] > 0 ? "(up)" : X.trend[c] < 0 ? "(down)" : "", X.why[c]); v.push_back({b, 10999}); }
        v.push_back({"-- The Word Around Town --", 10999});
        for (int i = 0; i < 8; i++) if (X.gos[i].text[0]) {
            snprintf(b, sizeof b, "[%s] %s", X.gos[i].truth ? "FACT" : "RUMOR", X.gos[i].text); v.push_back({b, 10999});
            if (!X.gos[i].truth) v.push_back({"  Set the Record Straight", 10200 + i});
        }
        break;
    case 3:
        for (int k = 0; k < 5; k++) { snprintf(b, sizeof b, "[%s] %s", (X.traditions >> k) & 1 ? "ON" : "off", TRAD[k]); v.push_back({b, 10300 + k}); }
        snprintf(b, sizeof b, "Tradition streak: %d", X.tradStreak); v.push_back({b, 10999});
        break;
    case 4:
        if (!X.job) { v.push_back({"Pick an active career:", 10999}); for (int k = 1; k < 4; k++) v.push_back({JOBS[k], 10400 + k}); break; }
        snprintf(b, sizeof b, "%s - %s (perf %d%%)", JOBS[X.job], JOBTITLES[X.job][std::min(4, (int)X.jobLvl)], X.jobPerf); v.push_back({b, 10999});
        v.push_back({"Go to Work (play your shift)", 10410});
        v.push_back({"Quit This Career", 10411});
        break;
    case 6: {
        v.push_back({"Tap someone to change their bedroom (max 4 rooms):", 10999});
        for (int i = 0; i < (int)G.sims.size() && i < 128; i++) if (G.sims[i].household && !simDead(i)) {
            snprintf(b, sizeof b, "%s: %s", first(i).c_str(), X.bedOf[i] ? ROOM_NAMES[BEDROOMS[X.bedOf[i] - 1]] : "(no room)"); v.push_back({b, 10600 + i}); }
        break;
    }
    case 7:
        for (int k = 0; k < XS_COUNT; k++) { snprintf(b, sizeof b, "%s: Lv %d (%d%%)", XS_NAMES[k], XSV(k) / 100, XSV(k) % 100); v.push_back({b, 10999}); }
        snprintf(b, sizeof b, "House cleanliness: %d%%%s", X.clean, X.clean < 10 ? " - RATS!" : X.clean < 25 ? " - roaches!" : X.clean < 40 ? " - smelly" : ""); v.push_back({b, 10999});
        v.push_back({"Practice Singing (shower)", 10700}); v.push_back({"Practice Dancing", 10701}); v.push_back({"Video Games", 10702});
        v.push_back({"Write a Story", 10703}); v.push_back({"Tend Houseplants", 10704}); v.push_back({"Practice Stand-Up", 10705}); v.push_back({"Mix Mocktails", 10706}); v.push_back({"Knit a Scarf", 10707}); v.push_back({"Makeup Tutorial", 10708});
        if (X.clean < 40) v.push_back({"Call the Exterminator ($120)", 10710});
        break;
    case 14: {
        const int day = calSel < 0 ? G.day : calSel; const char* hn = nullptr; const int hs = holidayOf(day, &hn);
        snprintf(b, sizeof b, "Plan for %s %d (%s):", WEEKDAY_NAMES[day % 7], day % 28 + 1, WEATHER_NAMES[lxForecast(day)]); v.push_back({b, 10999});
        if (hs >= 0 && day == (int)G.day) for (int k = 0; k < 3; k++) v.push_back({std::string("Tradition: ") + TRADS[hs][k], 13300 + k});
        for (int k = 0; k < 4; k++) v.push_back({std::string("Make it a holiday: ") + CHOL[k], 13310 + k});
        v.push_back({"Plan a Social Event (wedding, party, date...)", 14900});
        for (int k = 0; k < 4; k++) v.push_back({std::string("Getaway: ") + GTYPE[k] + (k == 0 ? " (3 days)" : k == 1 ? " (2 days)" : " (1 day)"), 13330 + k});
        break;
    }
    case 12: {
        const Sim& me = MC();
        if (me.stage != ST_TEEN) { v.push_back({"Teen Life is for teen characters.", 10999}); break; }
        static const char* CL[5] = {"no clique", "the Populars", "the Skaters", "the Theater Kids", "the Brainiacs"};
        snprintf(b, sizeof b, "You're with %s. Detentions: %d", CL[X.clique % 5], X.detentions); v.push_back({b, 10999});
        static const char* MS[6] = {"First Crush", "Learner's Permit", "Homecoming", "First Job", "First Party", "Graduated"};
        std::string m = "Milestones:"; for (int k = 0; k < 6; k++) if ((X.teenMiles >> k) & 1) m += std::string(" ") + MS[k] + ","; if (m.back() == ',') m.pop_back(); else m += " none yet"; v.push_back({m.substr(0, 60), 10999});
        v.push_back({"Sneak Out at Night", 13200}); v.push_back({"Skip Class", 13201}); v.push_back({"Prank a Teacher", 13202}); v.push_back({"Study Group", 13203});
        v.push_back({"Join a Clique...", 13204}); v.push_back({"Take the Driving Test", 13205}); v.push_back({"Go to Homecoming", 13206}); v.push_back({"Get a Part-Time Job", 13207}); v.push_back({"Confess a Crush", 13208});
        break;
    }
    case 13: {
        std::string g; for (int k = 0; k < 3; k++) if (okSim(X.group[k])) g += first(X.group[k]) + " ";
        v.push_back({g.empty() ? "No group. Add friends to travel together:" : ("Group: " + g).substr(0, 60), 10999});
        int n = 0; for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o < 0 || !okSim(o) || G.sims[o].household || r.fr < 20 || n >= 6) continue;
            bool in = false; for (int k = 0; k < 3; k++) in |= X.group[k] == o; if (in) continue; v.push_back({"Add " + first(o), 13000 + o}); n++; }
        if (!g.empty()) { v.push_back({"Sit Together", 13210}); v.push_back({"Group Chat", 13211}); v.push_back({"Group Selfie", 13212}); v.push_back({"Disband Group", 13213}); }
        break;
    }
    case 10: {
        static const char* CN[3] = {"Dance-Off (Wed, Dance Studio)", "Fashion Show (Fri, Museum)", "Cooking Challenge (Sat, Le Bistro)"};
        static const int CD[3] = {3, 5, 6};
        snprintf(b, sizeof b, "Contests entered: %d   Wins: %d", X.entries, X.wins); v.push_back({b, 10999});
        for (int k = 0; k < 3; k++) { const bool today = G.day % 7 == CD[k]; snprintf(b, sizeof b, "%s%s", CN[k], today ? " - TODAY!" : ""); v.push_back({b, today && X.contestDay != G.day ? 10900 + k : 10999}); }
        if (X.contestDay == G.day) v.push_back({"You already competed today.", 10999});
        break;
    }
    case 11:
        snprintf(b, sizeof b, "Trend of the week: %s", TRENDS[lxTrend()]); v.push_back({b, 10999});
        if (X.closedRoom >= 0 && G.day < X.closedUntil) { snprintf(b, sizeof b, "Closed: %s (until day %d)", ROOM_NAMES[X.closedRoom % R_COUNT], X.closedUntil + 1); v.push_back({b, 10999}); }
        snprintf(b, sizeof b, "Rent index: %d%%", X.rentPct); v.push_back({b, 10999});
        v.push_back({"-- Maple Valley Gazette --", 10999});
        for (int i = X.newsN - 1; i >= 0; i--) { snprintf(b, sizeof b, "Day %d: %s", X.newsDay[i] + 1, X.news[i]); v.push_back({b, 10999}); }
        if (!X.newsN) v.push_back({"Slow news week.", 10999});
        v.push_back({X.trendJudge ? "Trend judging: ON (turn off)" : "Trend judging: OFF (turn on)", 10950});
        break;
    case 8:
        snprintf(b, sizeof b, "Glam Live - %u followers", (unsigned)G.followers); v.push_back({b, 10999});
        v.push_back({"Go Live (chat & gifts)", 10800}); v.push_back({"Live Q&A", 10801}); v.push_back({"Live with a Friend", 10802}); v.push_back({"GRWM Live (makeup)", 10803});
        break;
    case 9:
        snprintf(b, sizeof b, "TikTak - %u followers", (unsigned)G.followers); v.push_back({b, 10999});
        v.push_back({"Post a Dance Trend", 10810}); v.push_back({"Lip-Sync Video", 10811}); v.push_back({"Get Ready With Me", 10812}); v.push_back({"Storytime (spill tea)", 10813}); v.push_back({"Comedy Skit", 10814});
        break;
    case 5: {
        const Sim& me = MC();
        if (me.look.guy || me.stage < ST_TEEN || me.stage > ST_MIDDLE) v.push_back({"No cycle to track for this character.", 10999});
        else if (me.pregnant) v.push_back({"Expecting - cycle paused.", 10999});
        else { const int d = (G.day + G.mc * 7) % 28; snprintf(b, sizeof b, "Cycle day %d of 28 %s", d + 1, d < 5 ? "(period)" : d >= 25 ? "(PMS)" : ""); v.push_back({b, 10999});
            v.push_back({"Take a Pain Reliever ($5)", 10500}); v.push_back({"Heating Pad & Rest", 10501}); v.push_back({"Chocolate Craving ($3)", 10502}); }
        v.push_back({X.cycles ? "Cycles: ON (turn off)" : "Cycles: OFF (turn on)", 10510});
        if (me.stage >= ST_YA) {
            static const char* TATN[8] = {"Arm Sleeve", "Love / Hate Thighs", "Thigh Heart", "Thigh Cross", "Shoulder Rose", "Face Stars", "Hip Butterfly", "Ankle Vine"};
            v.push_back({"-- Tattoo Parlor ($50 each) --", 10999});
            for (int k = 0; k < 8; k++) { snprintf(b, sizeof b, "%s %s", (lxTattoos(G.mc) >> k) & 1 ? "[x] Remove" : "[ ] Get", TATN[k]); v.push_back({b, 10530 + k}); }
            v.push_back({"-- Family Planning --", 10999});
            if (!me.look.guy || true) v.push_back({lxBirthControl(G.mc) ? "Birth control: ON (stop)" : "Birth control: OFF (start, $15/wk)", 10520});
            if (me.pregnant) {
                snprintf(b, sizeof b, "Pregnant - due in %d day%s", me.pregDays, me.pregDays == 1 ? "" : "s"); v.push_back({b, 10999});
                static const char* PLAN[3] = {"Hospital", "Home + Midwife", "Water Birth"};
                snprintf(b, sizeof b, "Birth plan: %s (change)", PLAN[X.birthPlan % 3]); v.push_back({b, 10521});
                v.push_back({"Prenatal Vitamins ($10)", 10522}); v.push_back({"Prenatal Check-up", 10523});
                v.push_back({"Clinic: End the Pregnancy...", 10524});
            }
        }
        break;
    }
    }
    return v;
}

static const char* TASKS[4][3][4] = {{},
    {{"Kitchen fire on Maple Lane!", "Rush in with the hose", "Shut off the gas first", "Evacuate the neighbors"},
     {"Cat stuck in a tree.", "Climb the ladder", "Coax it with treats", "Call the owner over"},
     {"Station inspection today.", "Polish the truck", "Run safety drills", "Charm the inspector"}},
    {{"EMF spike in the attic!", "Sweep with the meter", "Set a spirit trap", "Interview the owner"},
     {"Cold spot in the cellar.", "Thermal camera scan", "Burn sage & talk to it", "Research the house history"},
     {"A client swears it's haunted.", "Debunk with science", "Hold a seance", "Calm the client down"}},
    {{"Client brief: a dream cottage.", "Sketch bold concepts", "Crunch the budget", "Pitch it with flair"},
     {"Model day at the studio.", "Build the scale model", "Fix the load math", "Present to the board"},
     {"Site visit - something's off.", "Redesign on the spot", "Check the blueprints", "Talk to the crew"}}};
static const int TASK_SKILL[3] = {SK_FITNESS, SK_LOGIC, SK_CHARISMA};
static const int ARCH_SKILL[3] = {SK_CREATIVITY, SK_LOGIC, SK_CHARISMA};

static LxMenu taskMenu() {
    LxMenu v; const int t = (X.task + X.job) % 3;
    v.push_back({TASKS[X.job][t][0], 10999});
    for (int k = 0; k < 3; k++) v.push_back({TASKS[X.job][t][k + 1], 10420 + k});
    return v;
}

// ---------------------------------------------------------------- outfit categories (auto-changing)
enum { OC_EVERYDAY, OC_FORMAL, OC_ATHLETIC, OC_SLEEP, OC_PARTY, OC_SWIM, OC_HOT, OC_COLD, OC_N };
static const char* OCN[OC_N] = {"Everyday", "Formal", "Athletic", "Sleepwear", "Party", "Swimwear", "Hot Weather", "Cold Weather"};
static int ocPick = 0;
static int defaultFor(int cat, bool guy) {
    static const char* KW[OC_N][6] = {{nullptr}, {"Gown", "Formal", "Gala", "Suit", "Cocktail", "Prom"}, {"Sport", "Gym", "Track", "Yoga", "Jersey", "Athle"},
        {"Pajama", "Sleep", "PJ", "Lounge", "Night", nullptr}, {"Rhinestone", "Glam", "Club", "Disco", "Sparkle", "Night Out"}, {"Swim", "Bikini", "Beach", "Pool", "Surf", nullptr},
        {"Summer", "Shorts", "Tank", "Sun", "Beach", nullptr}, {"Winter", "Coat", "Puffer", "Sweater", "Cozy", "Snow"}};
    for (int k = 0; k < 6; k++) { if (!KW[cat][k]) break; for (int o = 1; o < NUM_OUTFITS; o++) if (std::string(OUTFITS[o].name).find(KW[cat][k]) != std::string::npos) return o; }
    (void)guy; return -1;
}
static int catNow(int s, const Actor* a) {
    const int r = G.room;
    if (a && a->sleeping) return OC_SLEEP;
    if (r == R_BEACH || r == R_HOTELPOOL) return OC_SWIM;
    if (r == R_GYM) return OC_ATHLETIC;
    if (r == R_CLUB || r == R_VELVET || r == R_KARAOKE || r == R_GLOWBOWL) return OC_PARTY;
    if (r == R_CHAPEL || r == R_RESTAURANT || r == R_DTBISTRO || r == R_ROOFTOP || r == R_MUSEUM) return OC_FORMAL;
    if (!roomIsHome(r)) { if (G.weather == WX_HEAT) return OC_HOT; if (G.weather == WX_SNOW || seasonOf(G.day) == 3) return OC_COLD; }
    if (roomIsHome(r) && (lastHour >= 22 || (lastHour >= 0 && lastHour < 6)) && s >= 0) return OC_SLEEP;
    return OC_EVERYDAY;
}
static void outfitTick() {
    if (!X.autoWear) return;
    for (const Actor& a : G.rooms[G.house][G.room].actors) {
        if (a.kind != 0 || a.id >= 128 || a.id >= G.sims.size()) continue;
        Sim& s = G.sims[a.id]; if (!s.household || s.stage < ST_CHILD) continue;
        const int i = a.id;
        if (!X.base[i]) X.base[i] = (uint8_t)(s.outfit + 1);
        if (X.lastWorn[i] && s.outfit + 1 != X.lastWorn[i]) X.base[i] = (uint8_t)(s.outfit + 1);   // changed by hand (closet / Create-a-Sim): that's the new everyday look
        const int c = catNow(i, &a);
        int o = c == OC_EVERYDAY ? X.base[i] - 1 : X.wear[i][c] ? X.wear[i][c] - 1 : defaultFor(c, s.look.guy);
        if (o < 0) o = X.base[i] - 1;
        if (o >= 0 && o < NUM_OUTFITS && s.outfit != o) s.outfit = (uint8_t)o;
        X.lastWorn[i] = (uint8_t)(s.outfit + 1);
    }
}
LxMenu lxWardrobe() {
    LxMenu v; char b[96]; const int i = G.mc < 128 ? G.mc : 0;
    v.push_back({X.autoWear ? "Auto-change outfits: ON (turn off)" : "Auto-change outfits: OFF (turn on)", 15900});
    for (int c = 0; c < OC_N; c++) {
        const int o = c == OC_EVERYDAY ? (X.base[i] ? X.base[i] - 1 : MC().outfit) : X.wear[i][c] ? X.wear[i][c] - 1 : defaultFor(c, MC().look.guy);
        snprintf(b, sizeof b, "%s: %s%s", OCN[c], o >= 0 && o < NUM_OUTFITS ? OUTFITS[o].name : "(same as everyday)", c && !X.wear[i][c] ? " (auto)" : ""); v.push_back({b, 15000 + c});
    }
    return v;
}
int lxTattoos(int s) { return s >= 0 && s < 128 ? X.tat[s] : 0; }
bool lxBirthControl(int s) { return s >= 0 && s < 128 && X.bc[s]; }
void lxBirth(int mom, int baby) {
    if (mom < 0 || mom >= (int)G.sims.size() || baby < 0 || baby >= (int)G.sims.size()) return;
    static const char* WHERE[3] = {"at Maple Valley Hospital", "at home with a midwife", "in a calm water birth"};
    const int p = X.birthPlan % 3;
    if (p == 0 && G.sims[mom].household) G.money = G.money > 200 ? G.money - 200 : 0;
    Sim& m = G.sims[mom];
    addMoodlet(m, "Exhausted New Mom", EM_UNCOMFORTABLE, 2, 1440); m.needs[N_ENERGY] = std::min(m.needs[N_ENERGY], 20.f);
    for (int i = 0; i < (int)G.sims.size(); i++) if (i != mom && G.sims[i].household && !simDead(i)) { addMoodlet(G.sims[i], "New Baby in the House!", EM_HAPPY, 2, 1440); tweenPlay(i, TW_CHEER, 1.2f); }
    toast(first(mom) + " went into labor and gave birth " + WHERE[p] + "! Say hi to " + first(baby) + "!" + (p == 0 && G.sims[mom].household ? " (-$200)" : ""));
    addMemoryOnce(MEM_FAMILY, mom, baby, "Gave birth to " + G.sims[baby].name + " " + WHERE[p] + ".");
    lxNews("Welcome to the world, baby " + first(baby) + "!");
}
extern int g_seCalDay;
LxMenu lxOption(int id) {
    Sim& me = MC(); Activity a; char b[128];
    if (id == 15900) { X.autoWear ^= 1; return lxWardrobe(); }
    if (id >= 15000 && id < 15000 + OC_N) {
        ocPick = id - 15000; LxMenu v; snprintf(b, sizeof b, "Pick your %s outfit:", OCN[ocPick]); v.push_back({b, 10999});
        if (ocPick) v.push_back({"Automatic", 15100});
        for (int o = 1; o < NUM_OUTFITS; o++) if ((ownsOutfit(o) || o == me.outfit || (o >= FIRST_KID_OUTFIT && o < FIRST_MALE_OUTFIT && me.stage <= ST_PRETEEN) || (o >= FIRST_MALE_OUTFIT && me.stage >= ST_TEEN)) && outfitFits(o, me.stage, me.look.guy)) v.push_back({OUTFITS[o].name, 15101 + o});
        return v;
    }
    if (id >= 15100 && id < 15101 + NUM_OUTFITS) {
        const int i = G.mc < 128 ? G.mc : 0, o = id - 15101;
        if (ocPick == OC_EVERYDAY) { if (o >= 0) { X.base[i] = (uint8_t)(o + 1); me.outfit = (uint8_t)o; X.lastWorn[i] = (uint8_t)(o + 1); } }
        else X.wear[i][ocPick] = (uint8_t)(o < 0 ? 0 : o + 1);
        toast(std::string(OCN[ocPick]) + " outfit saved!"); outfitTick(); return lxWardrobe();
    }
    if (id >= 13000 && id < 13200 && okSim(id - 13000)) {
        int k = 0; while (k < 3 && okSim(X.group[k])) k++;
        if (k >= 3) { toast("Groups max out at you + 3 friends."); return lxApp(13); }
        X.group[k] = (int16_t)(id - 13000); toast(first(id - 13000) + " joined your group! They'll travel with you."); return lxApp(13);
    }
    if (id >= 10010 && id < 10016) { partyType = id - 10010; LxMenu v; v.push_back({"Dress code:", 10999}); for (int k = 0; k < 5; k++) v.push_back({DRESS[k], 10020 + k}); return v; }
    if (id >= 10020 && id < 10025) { partyDress = id - 10020; LxMenu v; v.push_back({"Food:", 10999}); for (int k = 0; k < 4; k++) v.push_back({FOOD[k], 10030 + k}); return v; }
    if (id >= 10030 && id < 10034) { partyFood = id - 10030; LxMenu v;
        snprintf(b, sizeof b, "%s | %s dress | %s", PARTY_TYPES[partyType], DRESS[partyDress], FOOD[partyFood]); v.push_back({b, 10999});
        v.push_back({"Send Invites & Start!", 10040}); v.push_back({"Cancel", 10999}); return v; }
    if (id == 10040) {
        if (!roomIsHome(G.room)) { toast("Host it at home!"); return {}; }
        if (!pay(FOOD_COST[partyFood])) return {};
        wantParty = true; partyPending = true; return {};
    }
    if (id >= 10100 && id < 10130) {
        const int i = id % 10; Invite& I = X.inv[i]; if (I.who < 0 || !okSim(I.who)) return {};
        const std::string n = first(I.who);
        if (id < 10110) { changeRel(G.mc, I.who, 6, I.kind == 2 ? 6 : 0); addMoodlet(me, "Out with Friends", EM_HAPPY, 1, 240); addMemoryOnce(MEM_FRIEND, G.mc, I.who, "Hung out at " + std::string(ROOM_NAMES[I.room % R_COUNT]) + ".");
            const int rm = I.room; I.who = -1; toast(n + " is so happy you came!"); travelTo(rm); return {}; }
        if (id < 10120) { changeRel(G.mc, I.who, -1, 0); toast(n + ": \"No worries, next time!\""); I.who = -1; return {}; }
        I.day++; toast(n + ": \"Tomorrow works!\""); return {};
    }
    if (id >= 10200 && id < 10208) {
        Gossip& g = X.gos[id - 10200];
        if (irand(10) < 4 + (int)me.skills[SK_CHARISMA]) { lxRepAdd(g.circle, 6, "cleared a rumor"); toast("You set the record straight. People believe you."); g = Gossip{}; }
        else { lxRepAdd(g.circle, -2, "denied too hard"); toast("Nobody's buying it... yet."); }
        return lxApp(2);
    }
    if (id >= 10600 && id < 10728 && id - 10600 < (int)G.sims.size()) {
        const int s = id - 10600; X.bedOf[s] = (uint8_t)((X.bedOf[s] + 1) % 5);
        if (X.bedOf[s]) { int n = 0; for (int i = 0; i < (int)G.sims.size() && i < 128; i++) if (G.sims[i].household && X.bedOf[i] == X.bedOf[s]) n++; if (n > 2) toast("That room is getting crowded (2 max feels best)."); }
        return lxApp(6);
    }
    if (id >= 10300 && id < 10305) { X.traditions ^= (uint8_t)(1 << (id - 10300)); return lxApp(3); }
    if (id >= 10401 && id <= 10403) { X.job = (uint8_t)(id - 10400); X.jobLvl = 0; X.jobPerf = 50; toast(std::string("You're now a ") + JOBTITLES[X.job][0] + "!"); return lxApp(4); }
    if (id == 10411) { X.job = 0; toast("You quit. Freedom!"); return {}; }
    if (id == 10410) {
        if (me.stage < ST_YA) { toast("You need to be an adult for this career."); return {}; }
        if (me.needs[N_ENERGY] < 15) { toast("Too tired to work. Sleep first!"); return {}; }
        X.task = 0; X.taskScore = 0; shiftOn = true;
        { static const int WR[4] = {R_LIVING, R_STREET, R_HAUNTED, R_CORNER}; if (G.room != WR[X.job]) travelTo(WR[X.job]); }
        return taskMenu();
    }
    if (id >= 10420 && id < 10423) {
        const int k = id - 10420, sk = X.job == 3 ? ARCH_SKILL[k] : TASK_SKILL[k];
        const int roll = irand(10) + (int)me.skills[sk] * 2 + (me.needs[N_ENERGY] > 50 ? 2 : 0);
        a = makeAct(JOBS[X.job], 120, 8); a.rate[N_ENERGY] = -6; a.skill = sk; a.skillRate = 8; startActivity(a);
        const bool good = roll >= 8; X.taskScore += good ? 2 : roll >= 5 ? 1 : 0;
        toast(good ? "Nailed it!" : roll >= 5 ? "It went okay." : "Rough one...");
        if (++X.task < 3) return taskMenu();
        shiftOn = false;
        const int payout = 90 + X.jobLvl * 60 + X.taskScore * 20;
        G.money += payout; X.jobPerf = (uint8_t)std::max(0, std::min(100, X.jobPerf + (X.taskScore - 3) * 8));
        if (X.jobPerf >= 90 && X.jobLvl < 4) { X.jobLvl++; X.jobPerf = 50; snprintf(b, sizeof b, "PROMOTED to %s! (+$%d)", JOBTITLES[X.job][X.jobLvl], payout); addMoodlet(me, "Promoted!", EM_CONFIDENT, 3, 1440); addMemoryOnce(MEM_CAREER, G.mc, -1, b); }
        else snprintf(b, sizeof b, "Shift over: earned $%d. Performance %d%%.", payout, X.jobPerf);
        toast(b); lxRepAdd(X.job == 1 ? CI_JOCKS : X.job == 2 ? CI_SCHOLARS : CI_ARTISTS, 2, "great work");
        return {};
    }
    switch (id) {
    case 10500: if (!pay(5)) return {}; X.cramps = 0; toast("Cramps easing up."); addMoodlet(me, "Feeling Better", EM_FINE, 1, 240); return {};
    case 10501: a = makeAct("Resting with a Heating Pad", 60, 8); a.rate[N_ENERGY] = 6; startActivity(a); addMoodlet(me, "Cozy & Warm", EM_HAPPY, 1, 240); return {};
    case 10502: if (!pay(3)) return {}; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 8); addMoodlet(me, "Chocolate Fix", EM_HAPPY, 1, 120); return {};
    case 10510: X.cycles ^= 1; return lxApp(5);
    case 10520: if (G.mc < 128) { X.bc[G.mc] ^= 1; toast(X.bc[G.mc] ? "You started birth control. Surprise pregnancies are very unlikely now." : "You stopped birth control."); } return lxApp(5);
    case 10530: case 10531: case 10532: case 10533: case 10534: case 10535: case 10536: case 10537:
        if (G.mc < 128) { const int k = id - 10530; const bool has = (X.tat[G.mc] >> k) & 1;
            if (!has && !pay(50)) return {};
            X.tat[G.mc] ^= (uint8_t)(1 << k); if (!has) { a = makeAct("Getting Tattooed", 60, 8); startActivity(a); addMoodlet(me, "Fresh Ink!", EM_CONFIDENT, 2, 1440); lxRepAdd(CI_PARTY, 1, "fresh ink"); } else toast("Tattoo removed with laser."); }
        return lxApp(5);
    case 10521: X.birthPlan = (uint8_t)((X.birthPlan + 1) % 3); return lxApp(5);
    case 10522: if (!pay(10)) return {}; addMoodlet(me, "Taking Care of Baby & Me", EM_HAPPY, 1, 1440); return {};
    case 10523: a = makeAct("Prenatal Check-up", 60, 8); startActivity(a); addMoodlet(me, "Heard the Heartbeat", EM_HAPPY, 2, 720); toast("Everything looks healthy! The doctor printed an ultrasound."); return {};
    case 10524: { LxMenu v; v.push_back({"This is a private choice. Are you sure?", 10999}); v.push_back({"Yes, end the pregnancy", 10525}); v.push_back({"No, go back", 10999}); return v; }
    case 10525:
        if (!me.pregnant || me.stage < ST_YA) return {};
        me.pregnant = false; me.pregDays = 0;
        a = makeAct("Clinic Visit", 120, 8); startActivity(a);
        addMoodlet(me, "Complicated Feelings", EM_SAD, 1, 1440); addMoodlet(me, "Resting Up", EM_UNCOMFORTABLE, 1, 720);
        toast("You had the procedure at the clinic. It's private - nobody else needs to know. Take it easy today.");
        return {};
    case 10800: case 10801: case 10802: case 10803: {
        if (me.stage < ST_TEEN) { toast("Going live is for teens and up."); return {}; }
        a = makeAct("Live Streaming", 45, 8); a.rate[N_SOCIAL] = 15; a.rate[N_FUN] = 8; a.skill = id == 10803 ? SK_STYLE : SK_CHARISMA; a.skillRate = 6; startActivity(a);
        const int viewers = 5 + (int)(G.followers / 20) + irand(30) + (int)me.skills[SK_CHARISMA] * 6;
        const int gifts = viewers / 12 + irand(8), gain = viewers / 6 + irand(10);
        G.money += gifts; G.followers += gain;
        if (id == 10802) { int f = -1; for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o >= 0 && r.fr >= 30 && okSim(o)) { f = o; break; } }
            if (f >= 0) { changeRel(G.mc, f, 5, 0); toast(first(f) + " joined your live!"); } }
        if (id == 10803) lxRepAdd(CI_FASHION, 2, "GRWM lives");
        snprintf(b, sizeof b, "LIVE: %d viewers, $%d in gifts, +%d followers!", viewers, gifts, gain); toast(b);
        if (irand(100) < 8) { addMoodlet(me, "Trolls in the Chat", EM_ANGRY, 1, 180); toast("Ugh, trolls in the chat."); }
        return {};
    }
    case 10810: case 10811: case 10812: case 10813: case 10814: {
        if (me.stage < ST_TEEN) { toast("TikTak is for teens and up."); return {}; }
        static const char* NM[5] = {"Filming a Dance Trend", "Filming a Lip-Sync", "Filming a GRWM", "Filming Storytime", "Filming a Skit"};
        a = makeAct(NM[id - 10810], 30, 8); a.rate[N_FUN] = 10; startActivity(a);
        int q = irand(10) + (int)me.skills[SK_CHARISMA];
        if (id == 10810) { q += XSV(XS_DANCE) / 100; lxSkill(XS_DANCE, 8); }
        if (id == 10811) { q += XSV(XS_SING) / 100; lxSkill(XS_SING, 8); }
        if (id == 10812) { q += (int)me.skills[SK_STYLE]; lxRepAdd(CI_FASHION, 1, "GRWM videos"); }
        if (id == 10813) { q += 2; if (irand(100) < 25) lxGossip(first(G.mc) + " spilled tea on TikTak", irand(2), CI_PARTY, -1); }
        if (id == 10814) q += (int)me.skills[SK_CREATIVITY];
        lxSkill(XS_PHOTO, 5);
        const bool viral = irand(100) < 4 + q;
        const int views = (viral ? 20000 : 200) + irand(800) + (int)G.followers * (viral ? 3 : 1) / 2, gain = views / (viral ? 40 : 120);
        G.followers += gain;
        snprintf(b, sizeof b, "%s %d views, +%d followers!", viral ? "IT WENT VIRAL!" : "Posted:", views, gain); toast(b);
        if (viral) { lxNews(first(G.mc) + "'s TikTak goes viral"); addMoodlet(me, "Went Viral!", EM_CONFIDENT, 3, 720); sfxYay(); addMemoryOnce(MEM_FUN, G.mc, -1, "Went viral on TikTak!"); }
        return {};
    }
    case 10950: X.trendJudge ^= 1; return lxApp(11);
    case 10900: case 10901: case 10902: {
        if (X.contestDay == G.day) return {};
        const int k = id - 10900; static const int ROOM[3] = {R_DANCESTUDIO, R_MUSEUM, R_DTBISTRO}; static const char* NM[3] = {"Dance-Off", "Fashion Show", "Cooking Challenge"};
        X.contestDay = (uint16_t)G.day; X.entries++;
        if (G.room != ROOM[k]) travelTo(ROOM[k]);
        const float skill = k == 0 ? me.skills[SK_FITNESS] + XSV(XS_DANCE) / 100.f : k == 1 ? me.skills[SK_STYLE] + (lxTrendy(me.outfit) ? 3 : 0) : me.skills[SK_COOKING];
        const int mood = me.needs[N_ENERGY] > 50 ? 6 : 0, luck = irand(13);
        const int mine = (int)(skill * 7) + mood + luck + 10;
        std::vector<int> rivals; for (int i = 0; i < (int)G.sims.size() && rivals.size() < 3; i++) if (okSim(i) && !G.sims[i].household && G.sims[i].stage >= ST_TEEN && irand(6) == 0) rivals.push_back(i);
        int best = -1, bestScore = -1;
        for (int r : rivals) { const int sc = 25 + irand(50); if (sc > bestScore) { bestScore = sc; best = r; } }
        const bool win = mine >= bestScore;
        snprintf(b, sizeof b, "%s: you scored %d (skill %d, %s, luck %d) vs %s's %d. %s", NM[k], mine, (int)skill, mood ? "well rested" : "tired", luck, best >= 0 ? first(best).c_str() : "the field", bestScore, win ? "YOU WIN! +$300" : "So close!");
        toast(b);
        if (win) { tweenPlay(G.mc, TW_CHEER, 1.5f, 2);
            X.wins++; G.money += 300; addMoodlet(me, "Contest Champion", EM_CONFIDENT, 3, 1440); addMemoryOnce(MEM_GOAL, G.mc, -1, std::string("Won the ") + NM[k] + "!"); lxRepAdd(k == 0 ? CI_JOCKS : k == 1 ? CI_FASHION : CI_FAMILY, 6, "contest winner"); lxNews(first(G.mc) + " wins the " + NM[k]); sfxYay(); }
        else { addMoodlet(me, "Runner-Up", EM_SAD, 1, 480); if (best >= 0) { lxNews(first(best) + " wins the " + NM[k]); changeRel(G.mc, best, 2, 0); } }
        a = makeAct(NM[k], 90, 8); a.rate[N_FUN] = 10; startActivity(a);
        return {};
    }
    case 10700: a = makeAct("Singing in the Shower", 30, 8); a.rate[N_HYGIENE] = 30; a.rate[N_FUN] = 10; startActivity(a); lxSkill(XS_SING, 12); return {};
    case 10701: a = makeAct("Practicing Dance Moves", 45, 8); a.rate[N_FUN] = 12; a.rate[N_ENERGY] = -6; startActivity(a); lxSkill(XS_DANCE, 12); return {};
    case 10702: a = makeAct("Playing Video Games", 60, 8); a.rate[N_FUN] = 25; startActivity(a); lxSkill(XS_GAMING, 12); return {};
    case 10703: a = makeAct("Writing a Story", 60, 8); a.skill = SK_CREATIVITY; a.skillRate = 6; startActivity(a); lxSkill(XS_WRITE, 14); if (XSV(XS_WRITE) >= 500 && irand(4) == 0) { G.money += 80; toast("A magazine bought your short story! +$80"); } return {};
    case 10704: a = makeAct("Tending Houseplants", 30, 8); startActivity(a); lxSkill(XS_GARDEN, 14); lxClean(3); return {};
    case 10705: a = makeAct("Practicing Stand-Up", 45, 8); a.rate[N_FUN] = 10; a.skill = SK_CHARISMA; a.skillRate = 6; startActivity(a); lxSkill(XS_COMEDY, 14); return {};
    case 10706: a = makeAct("Mixing Mocktails", 30, 8); a.rate[N_FUN] = 8; startActivity(a); lxSkill(XS_MIXOLOGY, 14); return {};
    case 10707: a = makeAct("Knitting", 60, 8); a.rate[N_FUN] = 6; startActivity(a); lxSkill(XS_KNIT, 14); if (XSV(XS_KNIT) >= 300 && irand(3) == 0) { addMoodlet(me, "Cozy Handmade Scarf", EM_HAPPY, 1, 1440); toast("You knit a cozy scarf!"); } return {};
    case 10708: a = makeAct("Makeup Tutorial", 30, 8); a.skill = SK_STYLE; a.skillRate = 6; startActivity(a); lxSkill(XS_MAKEUP, 14); addMoodlet(me, "Beat Face", EM_CONFIDENT, 1, 360); return {};
    case 13200: {   // sneak out
        if (me.stage != ST_TEEN) return {};
        a = makeAct("Sneaking Out", 120, 8); a.rate[N_FUN] = 20; a.rate[N_ENERGY] = -10; startActivity(a);
        if (irand(100) < 35) { G.mcGrounded = 3; X.detentions++; toast("Busted climbing back in the window! Grounded for 3 days."); lxRepAdd(CI_FAMILY, -4, "sneaking out"); }
        else { addMoodlet(me, "Night Out Rush", EM_PLAYFUL, 2, 360); lxRepAdd(CI_PARTY, 3, "sneaks out"); toast("You snuck out and back. Nobody knows... yet."); if (irand(100) < 20) lxGossip(first(G.mc) + " sneaks out at night", true, CI_FAMILY, -1); }
        return {};
    }
    case 13201: if (me.stage != ST_TEEN) return {}; a = makeAct("Skipping Class", 120, 8); a.rate[N_FUN] = 15; startActivity(a);
        if (irand(100) < 40) { X.detentions++; toast("The vice principal caught you. Detention!"); lxRepAdd(CI_SCHOLARS, -4, "skips class"); addMoodlet(me, "Detention", EM_BORED, 1, 600); } else { lxRepAdd(CI_PARTY, 2, "skips class"); toast("Freedom! (Your grades won't love this.)"); } return {};
    case 13202: if (me.stage != ST_TEEN) return {};
        if (irand(10) < 4 + (int)me.skills[SK_CREATIVITY] / 2) { lxRepAdd(CI_PARTY, 5, "legendary prank"); lxGossip(first(G.mc) + " pulled an epic prank on a teacher", true, CI_PARTY, 1); addMoodlet(me, "Prank Legend", EM_PLAYFUL, 2, 720); toast("The whole school is talking about your prank!"); }
        else { X.detentions++; G.mcGrounded = 2; toast("Caught red-handed. Detention AND grounded."); lxRepAdd(CI_SCHOLARS, -3, "pranks"); } return {};
    case 13203: a = makeAct("Study Group", 90, 8); a.rate[N_SOCIAL] = 12; a.skill = SK_LOGIC; a.skillRate = 12; startActivity(a); lxRepAdd(CI_SCHOLARS, 3, "study group"); return {};
    case 13204: { LxMenu v; v.push_back({"Pick a clique:", 10999}); static const char* CL[4] = {"The Populars", "The Skaters", "The Theater Kids", "The Brainiacs"}; for (int k = 0; k < 4; k++) v.push_back({CL[k], 13220 + k}); return v; }
    case 13220: case 13221: case 13222: case 13223: { X.clique = (uint8_t)(id - 13219); static const int C2[4] = {CI_FASHION, CI_JOCKS, CI_ARTISTS, CI_SCHOLARS}; lxRepAdd(C2[id - 13220], 8, "joined the clique"); toast("You're in! Lunch table secured."); return lxApp(12); }
    case 13205: if (me.stage != ST_TEEN) return {};
        if (irand(10) < 3 + (int)me.skills[SK_LOGIC]) { X.teenMiles |= 2; addMoodlet(me, "Got My Permit!", EM_CONFIDENT, 3, 1440); addMemoryOnce(MEM_LIFE, G.mc, -1, "Passed the driving test!"); toast("You PASSED! Learner's permit unlocked."); }
        else toast("You hit a cone. Try again after more studying (Logic)."); return {};
    case 13206: X.teenMiles |= 4; a = makeAct("Homecoming Dance", 180, 8); a.rate[N_FUN] = 20; a.rate[N_SOCIAL] = 15; startActivity(a); addMoodlet(me, "Homecoming!", EM_HAPPY, 2, 720); addMemoryOnce(MEM_LIFE, G.mc, -1, "Went to homecoming."); lxRepAdd(CI_FASHION, 3, "homecoming"); return {};
    case 13207: X.teenMiles |= 8; a = makeAct("Shift at the Smoothie Shop", 240, 8); a.rate[N_ENERGY] = -8; startActivity(a); G.money += 45; toast("Part-time shift done: +$45"); addMemoryOnce(MEM_CAREER, G.mc, -1, "Got my first job!"); return {};
    case 13208: { int best = -1, bf = -1; for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o >= 0 && okSim(o) && G.sims[o].stage == ST_TEEN && r.fr > bf) { bf = r.fr; best = o; } }
        if (best < 0) { toast("You don't have a crush yet... meet more teens!"); return {}; }
        X.teenMiles |= 1;
        if (irand(100) < 30 + bf / 2) { changeRel(G.mc, best, 5, 10); addMoodlet(me, "They Like Me Back!", EM_HAPPY, 3, 1440); toast(first(best) + " likes you back!!"); addMemoryOnce(MEM_LOVE, G.mc, best, "First crush - they liked me back!"); }
        else { addMoodlet(me, "Crushed", EM_SAD, 2, 720); toast(first(best) + ": \"Oh... I think of you as a friend.\""); }
        return {}; }
    case 13210: for (Actor& x : G.rooms[G.house][G.room].actors) if (x.kind == 0) for (int k = 0; k < 3; k++) if (x.id == X.group[k] || x.id == G.mc) { x.pose = PO_SIT; x.poseT = 0; }
        toast("Everyone grabs a seat together."); return {};
    case 13211: { a = makeAct("Group Chat", 45, 8); a.rate[N_SOCIAL] = 20; a.rate[N_FUN] = 8; startActivity(a);
        for (int k = 0; k < 3; k++) if (okSim(X.group[k])) { changeRel(G.mc, X.group[k], 4, 0); for (int j = k + 1; j < 3; j++) if (okSim(X.group[j])) changeRel(X.group[k], X.group[j], 3, 0); }
        return {}; }
    case 13212: lxSkill(XS_PHOTO, 6); G.followers += 10 + irand(30); addMoodlet(me, "Squad Goals", EM_HAPPY, 1, 360); toast("Group selfie posted! The squad looks iconic."); return {};
    case 13213: for (auto& g : X.group) g = -1; toast("The group heads their separate ways."); return {};
    case 13300: case 13301: case 13302: {
        const char* hn = nullptr; const int hs = holidayOf(G.day, &hn); if (hs < 0) return {};
        if (X.tradDay != (uint8_t)(G.day & 255)) { X.tradDay = (uint8_t)(G.day & 255); X.tradDone = 0; }
        const int k = id - 13300; X.tradDone |= (uint8_t)(1 << k);
        a = makeAct(TRADS[hs][k], 60, 8); a.rate[N_FUN] = 15; a.rate[N_SOCIAL] = 10; startActivity(a);
        for (Sim& s : G.sims) if (s.household) addMoodlet(s, TRADS[hs][k], EM_HAPPY, 1, 480);
        if (X.tradDone == 7) { for (Sim& s : G.sims) if (s.household) addMoodlet(s, (std::string(hn) + " Magic").c_str(), EM_HAPPY, 3, 1440); addMemoryOnce(MEM_FAMILY, G.mc, -1, std::string("A perfect ") + hn + "!"); toast(std::string("All ") + hn + " traditions done! Holiday magic!"); }
        return lxApp(14);
    }
    case 13310: case 13311: case 13312: case 13313: {
        const int day = calSel < 0 ? G.day : calSel; int s = -1; for (int i = 0; i < 8; i++) if (!X.hol[i].used) { s = i; break; }
        if (s < 0) { toast("You have 8 custom holidays already."); return {}; }
        X.hol[s].used = 1; X.hol[s].kind = (uint8_t)(id - 13310); X.hol[s].day = (uint16_t)day;
        toast(std::string(CHOL[id - 13310]) + " added! It repeats every year."); return {};
    }
    case 14900: g_seCalDay = calSel < 0 ? G.day : calSel; return {{"__SE__", 14900}};
    case 13320: case 13321: case 13322: {
        static const char* NM[3] = {"Birthday Party", "Wedding", "Dinner Party"}; static const int HR[3] = {18, 16, 19};
        Event e; e.day = (uint16_t)(calSel < 0 ? G.day : calSel); e.hour = (uint8_t)HR[id - 13320]; e.type = EV_PLAN; e.text = NM[id - 13320]; G.events.push_back(e);
        toast(std::string(NM[id - 13320]) + " is on the calendar!"); return {};
    }
    case 13330: case 13331: case 13332: case 13333: {
        int s = -1; for (int i = 0; i < 4; i++) if (!X.get[i].used) { s = i; break; } if (s < 0) { toast("You already have 4 getaways planned."); return {}; }
        auto& g = X.get[s]; g = {}; g.used = 1; g.type = (uint8_t)(id - 13330); g.len = (uint8_t)(g.type == 0 ? 3 : g.type == 1 ? 2 : 1); g.day = (uint16_t)std::max<int>(G.day + 1, calSel < 0 ? G.day + 1 : calSel);
        pickWho = s; LxMenu v; v.push_back({"Add up to 3 activities:", 10999}); for (int k = 0; k < 8; k++) v.push_back({std::string(GACT[k]) + (GOUT[k] ? " (outdoor)" : ""), 13340 + k}); v.push_back({"Done", 10999}); return v;
    }
    case 13340: case 13341: case 13342: case 13343: case 13344: case 13345: case 13346: case 13347: {
        if (pickWho < 0 || pickWho > 3) return {}; auto& g = X.get[pickWho];
        for (int k = 0; k < 3; k++) if (g.acts[k] == 0xFF) { g.acts[k] = (uint8_t)(id - 13340); toast(std::string("Added ") + GACT[id - 13340] + "."); break; }
        if (g.acts[2] != 0xFF) { toast(std::string(GTYPE[g.type]) + " is booked! Check the calendar forecast."); return {}; }
        LxMenu v; v.push_back({"Add another activity:", 10999}); for (int k = 0; k < 8; k++) v.push_back({std::string(GACT[k]) + (GOUT[k] ? " (outdoor)" : ""), 13340 + k}); v.push_back({"Done", 10999}); return v;
    }
    case 10710: if (!pay(120)) return {}; X.clean = 60; toast("The exterminator cleared out the roaches and rats. Now clean up!"); return {};
    }
    return {};
}

bool lxWantsParty() { bool w = wantParty; wantParty = false; return w; }
void lxAfterParty() {
    if (!partyPending) return; partyPending = false;
    int guests = 0, score = 40 + partyFood * 12 + (int)MC().skills[SK_CHARISMA] * 3 + irand(20);
    for (const Actor& x : G.rooms[G.house][G.room].actors) if (x.kind == 0 && x.id >= 0 && x.id < (int)G.sims.size() && !G.sims[x.id].household) {
        guests++; const bool likesDress = irand(5) != 0 || partyDress == 0;
        changeRel(G.mc, x.id, likesDress ? 5 : 1, 0); score += likesDress ? 2 : -4;
        if (partyType == 1) addMoodlet(G.sims[x.id], "Sleepover!", EM_PLAYFUL, 2, 600);
    }
    score = std::max(0, std::min(100, score));
    char b[120]; snprintf(b, sizeof b, "%s (%s): %d guests, satisfaction %d%%%s", PARTY_TYPES[partyType], DRESS[partyDress], guests, score, score >= 80 ? " - legendary!" : score < 40 ? " - kinda flopped." : "");
    toast(b);
    addMoodlet(MC(), score >= 60 ? "Hosted a Great Party" : "Party Was Meh", score >= 60 ? EM_HAPPY : EM_BORED, score >= 80 ? 3 : 1, 720);
    lxRepAdd(partyType == 1 || partyType == 5 ? CI_FAMILY : CI_PARTY, score >= 60 ? 5 : -2, PARTY_TYPES[partyType]);
    if (score >= 80) { addMemoryOnce(MEM_FUN, G.mc, -1, std::string("Threw a legendary ") + PARTY_TYPES[partyType] + "!"); lxGossip(first(G.mc) + "'s " + PARTY_TYPES[partyType] + " was legendary", true, CI_PARTY, 1); lxNews(std::string("Legendary ") + PARTY_TYPES[partyType] + " at " + first(G.mc) + "'s place"); }
}

// ---------------------------------------------------------------- simulation
static void maybeInvite(int hour) {
    if (hour < 9 || hour > 19 || irand(100) >= 7) return;
    int slot = -1; for (int i = 0; i < 4; i++) if (X.inv[i].who < 0) { slot = i; break; }
    if (slot < 0) return;
    const Sim& me = MC(); std::vector<int> c;
    for (const Rel& r : G.rels) { const int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1;
        if (o < 0 || !okSim(o) || G.sims[o].household || r.fr < 30) continue;
        const Stage s = G.sims[o].stage; if ((me.stage >= ST_YA) != (s >= ST_YA) || s < ST_CHILD) continue;
        c.push_back(o); }
    if (c.empty()) return;
    const int who = c[irand((int)c.size())];
    static const int ROOMS[5] = {R_CAFE, R_CLUB, R_RESTAURANT, R_ARCADE, R_DINER};
    static const char* WHAT[5] = {"brunch", "a concert", "a date", "game night", "family dinner"};
    int kind = irand(5);
    if (kind == 2 && !canRomance(G.mc, who)) kind = 0;
    if (kind == 4 && !isFamily(G.mc, who)) kind = 3;
    if (kind == 1 && me.stage < ST_YA) kind = 3;
    Invite& I = X.inv[slot]; I.who = (int16_t)who; I.room = (int16_t)ROOMS[kind]; I.day = G.day; I.hour = (uint8_t)std::min(21, hour + 2); I.kind = (uint8_t)kind;
    toast(first(who) + " invited you to " + WHAT[kind] + "! (Phone > More > Invites)"); sfxTap();
}

static void traditionsHour(int hour) {
    if (!X.traditions || !roomIsHome(G.room)) return;
    const int dow = G.day % 7; int k = -1;
    if ((X.traditions & 1) && dow == 0 && hour == 18) k = 0;
    if ((X.traditions & 2) && dow == 5 && hour == 19) k = 1;
    if ((X.traditions & 4) && dow == 4 && hour == 20) k = 2;
    if ((X.traditions & 8) && dow == 6 && hour == 9) k = 3;
    if ((X.traditions & 16) && G.day % 84 == 40 && hour == 13) k = 4;
    if (k < 0) return;
    std::vector<int> hh; for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && !simDead(i)) hh.push_back(i);
    for (size_t i = 0; i < hh.size(); i++) { addMoodlet(G.sims[hh[i]], TRAD[k], EM_HAPPY, 2, 360); for (size_t j = i + 1; j < hh.size(); j++) changeRel(hh[i], hh[j], 3, 0); }
    if (k == 4) for (int i = 0; i < (int)G.sims.size(); i++) if (!G.sims[i].household && isFamily(G.mc, i) && okSim(i)) changeRel(G.mc, i, 6, 0);
    X.tradStreak = (uint8_t)std::min(250, X.tradStreak + 1);
    lxRepAdd(CI_FAMILY, 2, TRAD[k]);
    toast(std::string("Family tradition: ") + TRAD[k] + "! Everyone's together."); sfxYay();
    if (X.tradStreak % 10 == 0) addMemoryOnce(MEM_FAMILY, G.mc, -1, std::string(TRAD[k]) + " - a family tradition.");
}

void lxGroupFrame() {
    { static int tick = 0; if (++tick >= 30) { tick = 0; outfitTick(); } }   // grouped friends come along wherever you go and stay near you
    if (G.sims.empty() || !G.rooms[G.house][G.room].actors.size()) return;
    auto& acts = G.rooms[G.house][G.room].actors; float mx = 200;
    for (const Actor& x : acts) if (x.kind == 0 && x.id == G.mc) mx = x.x;
    for (int k = 0; k < 3; k++) {
        const int s = X.group[k]; if (!okSim(s) || G.sims[s].household) continue;
        Actor* a = nullptr; for (Actor& x : acts) if (x.kind == 0 && x.id == s) { a = &x; break; }
        if (!a) {   // pull them out of wherever they were
            for (int h = 0; h < 2; h++) for (int r = 0; r < R_COUNT; r++) { auto& v = G.rooms[h][r].actors; v.erase(std::remove_if(v.begin(), v.end(), [&](const Actor& x) { return x.kind == 0 && x.id == s; }), v.end()); }
            Actor n; n.kind = 0; n.id = (uint16_t)s; n.x = mx + 30 + k * 26; n.tx = n.x; n.y = 214 + k * 4; acts.push_back(n); continue;
        }
        a->stay = 120;
        if (std::fabs(a->x - mx) > 90 && a->pose != PO_SIT) a->tx = mx + (k % 2 ? -1 : 1) * (30 + k * 14);
    }
}
void lxHourly(int hour) {
    if (G.sims.empty() || hour == lastHour) return; lastHour = hour;
    maybeInvite(hour);
    if (hour == 9) for (auto& g : X.get) if (g.used && G.day >= g.day && G.day < g.day + g.len) {
        const int w = lxForecast(G.day); const bool bad = w == WX_RAIN || w == WX_STORM || w == WX_SNOW;
        std::string log = std::string(GTYPE[g.type % 4]) + ":";
        for (int k = 0; k < 3; k++) if (g.acts[k] < 8) {
            if (GOUT[g.acts[k]] && bad) { for (Sim& s : G.sims) if (s.household) addMoodlet(s, "Rained Out", EM_SAD, 1, 360); log += std::string(" ") + GACT[g.acts[k]] + " rained out;"; }
            else { for (Sim& s : G.sims) if (s.household) { addMoodlet(s, GACT[g.acts[k]], EM_HAPPY, 1, 480); s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 15); } log += std::string(" ") + GACT[g.acts[k]] + "!"; if (g.acts[k] < 4) MC().skills[SK_FITNESS] = std::min(10.f, MC().skills[SK_FITNESS] + 0.2f); }
        }
        toast(log.substr(0, 120));
        if (G.day == g.day + g.len - 1) { addMemoryOnce(MEM_FUN, G.mc, -1, std::string("Went on a ") + GTYPE[g.type % 4] + "!"); lxNews(first(G.mc) + "'s household returns from " + GTYPE[g.type % 4]); g.used = 0; }
    }
    traditionsHour(hour);
    for (Invite& I : X.inv) if (I.who >= 0 && I.day == G.day && hour == I.hour && G.room != I.room) {
        if (okSim(I.who) && irand(100) < 15) { toast(first(I.who) + " canceled - something came up."); I.who = -1; continue; }
    }
    // prom night for household teens
    if (hour == 20 && G.day % 112 == 100 && X.promYear != G.day / 112) {
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && G.sims[i].stage == ST_TEEN) {
            X.promYear = (uint16_t)(G.day / 112);
            Sim& t = G.sims[i]; addMoodlet(t, "Prom Night!", EM_HAPPY, 3, 720);
            const bool crown = irand(10) < 2 + (int)t.skills[SK_CHARISMA];
            addMemoryOnce(MEM_LIFE, i, -1, crown ? "Crowned at prom!" : "Went to prom.");
            toast(first(i) + (crown ? " was crowned at PROM!" : " had an amazing prom night!")); sfxYay(); if (crown) lxNews(first(i) + " crowned prom royalty at Glitter High");
            if (crown) lxRepAdd(CI_FASHION, 8, "prom royalty");
        }
    }
}

static const char* NPC52[52] = {
    "Aaliyah Brooks", "Bianca Torres", "Camila Reyes", "Dani Park", "Elena Vasquez", "Fiona Hart", "Gia Moretti", "Harper Lane",
    "Imani Cole", "Jade Kim", "Kiara Banks", "Lena Novak", "Maya Patel", "Nia Freeman", "Olivia Shaw", "Paris Dupont",
    "Quinn Ellis", "Riley Monroe", "Sienna Gold", "Talia Ross", "Uma Singh", "Vera Lin", "Willow Hayes", "Ximena Cruz",
    "Yara Haddad", "Zoe Carter", "Brielle Wade", "Coco Laurent", "Destiny Hall", "Esme Rivera",
    "Andre Wells", "Bryce Cooper", "Carlos Mendez", "Dante Rivers", "Eli Harper", "Felix Wright", "Grant Chen", "Hugo Bauer",
    "Isaiah Grant", "Jaden Ford", "Kai Nakamura", "Leo Santos", "Marcus Reed", "Nate Hill", "Omar Aziz", "Preston Vale",
    "Rafael Ortiz", "Sean Murphy", "Theo Blake", "Victor Hale", "Wes Jordan", "Zane Mercer"};
static const char* BIOS[8] = {"lives for brunch & drama", "barista by day, DJ by night", "always at the gym", "aspiring designer",
    "grad student, mostly caffeine", "local gossip queen", "runs a small bakery", "skater, painter, menace"};

void lxDaily() {
    if (G.sims.empty()) return;
    if (!X.npcFix) {   // the 52 new residents are the SBLCast pixel dolls; earlier builds added random lookalikes - they move away
        X.npcFix = 1; X.npcsAdded = 1;
        if (G.movedAway.size() < G.sims.size()) G.movedAway.resize(G.sims.size(), 0);
        for (int i = 0; i < (int)G.sims.size(); i++) for (int k = 0; k < 52; k++) if (!G.sims[i].household && G.sims[i].name == NPC52[k]) G.movedAway[i] = 1;
    }
    if (!X.bodyFix) {   // existing towns: give neighbors a mix of slim, mid-size and plus-size bodies (once)
        X.bodyFix = 1;
        for (Sim& s : G.sims) if (!s.household && s.stage >= ST_TEEN && s.look.f.pad == 0) { const int b = irand(10); s.look.f.pad = (uint8_t)(b < 5 ? 0 : b < 8 ? 1 : 2); }
    }
    if (!X.npcsAdded) {
        X.npcsAdded = 1; int added = 0;
        static const Stage ST[8] = {ST_YA, ST_YA, ST_ADULT, ST_ADULT, ST_TEEN, ST_MIDDLE, ST_ELDER, ST_YA};
        for (int k = 0; k < 52 && G.sims.size() < 118; k++) {
            Sim s = randomSim(ST[k % 8], k >= 30 ? 1 : 0);
            s.name = NPC52[k]; s.bio = BIOS[k % 8]; s.household = false;
            G.sims.push_back(s); added++;
        }
        if (added) { char b[80]; snprintf(b, sizeof b, "%d new neighbors moved into Maple Valley!", added); toast(b); }
    }
    for (Invite& I : X.inv) if (I.who >= 0 && I.day < G.day) I.who = -1;   // missed invitations expire
    if (G.day % 7 == 0) for (int i = 0; i < (int)G.sims.size() && i < 128; i++) if (X.bc[i] && G.sims[i].household) G.money = G.money > 15 ? G.money - 15 : 0;
    // town changes
    if (G.day % 7 == 0) lxNews(std::string("This week's trend: ") + TRENDS[lxTrend()]);
    if (G.day % 28 == 14 && X.rentPct < 160) { X.rentPct = (uint8_t)(X.rentPct + 5); lxNews("Landlords raise rents across Maple Valley"); }
    if (X.closedRoom >= 0 && G.day == X.closedUntil) { lxNews(std::string(ROOM_NAMES[X.closedRoom % R_COUNT]) + " reopens with a grand opening!"); X.closedRoom = -1; }
    if (X.closedRoom < 0 && G.day % 21 == 10) { static const int V[8] = {R_CAFE, R_DINER, R_ARCADE, R_GLOWBOWL, R_PAINTSIP, R_SUSHI, R_BAKERY, R_THRIFT}; X.closedRoom = (int16_t)V[irand(8)]; X.closedUntil = (uint16_t)(G.day + 5); lxNews(std::string(ROOM_NAMES[X.closedRoom % R_COUNT]) + " closes for renovation"); }
    if (X.trendJudge && MC().stage >= ST_TEEN) {
        if (lxTrendy(MC().outfit)) { addMoodlet(MC(), "On Trend", EM_CONFIDENT, 1, 720); lxRepAdd(CI_FASHION, 1, "on trend"); }
        if (irand(100) < 10) { int o = irand((int)G.sims.size()); if (okSim(o) && !G.sims[o].household && G.sims[o].stage >= ST_TEEN) toast(first(o) + std::string(": \"") + TRENDS[lxTrend()] + "? Everyone's wearing it. Not for me.\""); }
    }
    { int hh = 0; for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && !simDead(i)) hh++;
      X.clean = (uint8_t)std::max(0, X.clean - (6 + hh * 3));
      for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && !simDead(i)) {
        Sim& s = G.sims[i];
        if (X.clean < 10) { addMoodlet(s, "RATS in the House!", EM_UNCOMFORTABLE, 3, 1440); s.needs[N_HYGIENE] = std::max(0.f, s.needs[N_HYGIENE] - 25); }
        else if (X.clean < 25) addMoodlet(s, "Roaches Everywhere", EM_UNCOMFORTABLE, 2, 1440);
        else if (X.clean < 40) addMoodlet(s, "Smelly House", EM_UNCOMFORTABLE, 1, 1440);
        if (i < 128 && X.bedOf[i]) { int n = 0; for (int j = 0; j < (int)G.sims.size() && j < 128; j++) if (G.sims[j].household && X.bedOf[j] == X.bedOf[i]) n++; addMoodlet(s, n <= 1 ? "My Own Room" : n == 2 ? "Shared Room" : "Crowded Room", n <= 2 ? EM_HAPPY : EM_TENSE, 1, 1440); }
      }
      if (X.clean < 10) toast("There are RATS in the house! Clean up or call the exterminator (Phone > Skills+).");
      else if (X.clean < 25) toast("Ew... roaches in the kitchen. Time to do some chores!"); }
    // gossip spreads & fades
    for (Gossip& g : X.gos) if (g.text[0]) {
        g.age++; if (g.spread < 20) { g.spread++; lxRepAdd(g.circle, g.sign * (g.truth ? 2 : 1), g.truth ? "word got around" : "a rumor"); }
        if (!g.truth && irand(100) < 10) { g.truth = 2; }   // rumor debunked by someone else
        if (g.truth == 2) { snprintf(g.text, sizeof g.text, "%s", "(debunked rumor)"); g.truth = 1; g.sign = 0; }
        if (g.age > 12) g = Gossip{};
    }
    for (int c = 0; c < CI_COUNT; c++) if (X.trend[c]) X.trend[c] += X.trend[c] > 0 ? -1 : 1;
    // generations: mid-life crises & teen rebellion
    for (int i = 0; i < (int)G.sims.size() && i < 128; i++) {
        Sim& s = G.sims[i]; if (simDead(i)) continue;
        if (s.stage == ST_MIDDLE && !(X.crisis[i] & 1) && irand(100) < 3) {
            X.crisis[i] |= 1; static const char* W[4] = {"wants a flashy sports car", "booked a dramatic makeover", "is learning electric guitar", "joined a skydiving club"};
            addMoodlet(s, "Mid-Life Crisis", EM_TENSE, 2, 4320);
            if (s.household) toast(first(i) + " is having a mid-life crisis and " + W[irand(4)] + "!");
            lxGossip(first(i) + " is having a mid-life crisis", true, CI_FAMILY, 0);
        }
        if (s.stage == ST_TEEN && !(X.crisis[i] & 2) && irand(100) < 3) {
            X.crisis[i] |= 2; static const char* R[4] = {"dyed their hair without asking", "is ignoring curfew", "says nobody understands them", "got a secret piercing"};
            addMoodlet(s, "Rebellious Phase", EM_ANGRY, 2, 4320);
            if (s.household) { toast(first(i) + " " + R[irand(4)] + ". Teen rebellion!"); for (int p = 0; p < (int)G.sims.size(); p++) if (G.sims[p].household && G.sims[p].stage >= ST_ADULT && isFamily(i, p)) changeRel(i, p, -4, 0); }
        }
        // after-school scrapes for household teens
        if (s.household && s.stage == ST_TEEN && irand(100) < 6) {
            static const char* SC[5] = {"got detention for talking back", "got into a lunchroom argument", "missed curfew by an hour", "aced a pop quiz", "got invited to the cool kids' table"};
            const int e = irand(5); toast(first(i) + " " + SC[e] + "!");
            if (e < 3) { addMoodlet(s, "Teen Drama", EM_TENSE, 1, 720); if (i == G.mc) X.detentions += e == 0; } else addMoodlet(s, "School Win", EM_HAPPY, 1, 720);
        }
        if (s.household && s.stage == ST_YA && i == G.mc && !(X.teenMiles & 32) && (X.teenMiles & 7)) { X.teenMiles |= 32; addMemoryOnce(MEM_LIFE, i, -1, "Graduated high school!"); }
        // cycles
        if (X.cycles && !s.look.guy && s.stage >= ST_TEEN && s.stage <= ST_MIDDLE && !s.pregnant && (s.household || i == G.mc)) {
            const int d = (G.day + i * 7) % 28;
            if (d == 0) addMoodlet(s, "Period Cramps", EM_UNCOMFORTABLE, 1, 1440 * 2);
            else if (d == 3) addMoodlet(s, "Period", EM_FINE, 1, 1440 * 2);
            else if (d == 25) addMoodlet(s, "PMS", irand(2) ? EM_TENSE : EM_SAD, 1, 1440 * 2);
        }
    }
}

void lxSaveTo(const std::string& path) { X.own3 = G.ownedOutfits3; X.own4 = G.ownedOutfits4; X.hidePurse = G.hidePurse; X.crushBack = G.crushBack; X.toys = G.toys; X.crush = G.crush; guSaveTo(path + "g"); dlySaveTo(path + "d"); dySaveTo(path + "y"); famSaveTo(path + "f"); gsSaveTo(path + "s"); tpSaveTo(path + "t"); choiceSaveTo(path + "c"); lpSaveTo(path + "p"); dsSaveTo(path + "x"); FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fwrite(&X, sizeof X, 1, f); fclose(f); }
void lxLoadFrom(const std::string& path) {
    guLoadFrom(path + "g"); dlyLoadFrom(path + "d"); dyLoadFrom(path + "y"); famLoadFrom(path + "f"); gsLoadFrom(path + "s"); tpLoadFrom(path + "t"); choiceLoadFrom(path + "c"); lpLoadFrom(path + "p"); dsLoadFrom(path + "x");
    X = LX{}; X.npcsAdded = 1; G.ownedOutfits3 = G.ownedOutfits4 = 0; G.hidePurse = G.crushBack = 0; G.toys = 0; G.crush = -1;   // existing saves without the file get the residents once below
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) { X.npcsAdded = 0; return; }
    LX x; size_t n = fread(&x, 1, sizeof x, f); fclose(f);   // older (shorter) files keep the new fields' defaults
    if (n < 16 || x.magic != 0x4C584531) return;
    x.npcsAdded = 1; if (x.clean > 100) x.clean = 80;
    if (x.job > 3) x.job = 0; x.jobLvl = std::min<uint8_t>(x.jobLvl, 4); x.task = 0;
    for (Invite& I : x.inv) if (I.who >= (int)G.sims.size() || I.room < 0 || I.room >= R_COUNT) I.who = -1;
    for (Gossip& g : x.gos) { g.text[55] = 0; g.circle %= CI_COUNT; }
    for (auto& w : x.why) w[27] = 0;
    X = x;
    G.ownedOutfits3 = X.own3; G.ownedOutfits4 = X.own4; G.hidePurse = X.hidePurse; G.crushBack = X.crushBack; G.toys = X.toys; G.crush = X.crush;
}
int lxHolidayToday(int day, const char** name) { return holidayOf(day, name); }
