#include "levels.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "vfs.hpp"
#include "extras.hpp"
#include <algorithm>
#include <cstdio>
#include <string>
#define fopen vfopen
void toast(const std::string& s);

namespace {
struct Lv {
    uint32_t magic = 0x324C564Cu;   // "LVL2"
    uint8_t mode = 0, level = 1, task[3] = {0, 1, 2}, done = 0, ev = 0, pad = 0;
    uint16_t prestige = 0, lastDay = 0xFFFF;
    int32_t money0 = 0; uint8_t maxN[6] = {}, pad2[2] = {};
} L;
int needP(int lv) { const int k = lv - 1; return k <= 0 ? 0 : 8 * k + (k * k * 3) / 5; }   // Lv 10: 120, Lv 25: 537, Lv 50: 1832
struct Unl { int lv; const char* what; };
const Unl UNL[] = {{LF_CREDIT, "Financial Flexibility: loans & credit"}, {LF_PETS, "Furpals: adopt pets"}, {LF_ODDJOBS, "Hustle: the Odd Jobs app"},
                   {LF_POTS, "Homegrown: home garden pots"}, {LF_METRO, "Metro Central: subway & pixel arcade"}, {LF_RINK, "Roller Rink & Boardwalk Night Market"},
                   {LF_SALON, "Glam Up: the salon, nails & piercings"}, {LF_NIGHTLIFE, "Night Out: cinema & the Neon Strip"}, {LF_MODELING, "Catwalk: modeling & influencer gigs"},
                   {LF_DOWNTOWN, "Downtown: boutiques, bistro & date nights"}, {LF_FUNPARK, "Sparkle Land amusement park"}, {LF_ARTS, "Uptown Line: museum, aquarium, gardens & rooftop"},
                   {LF_COMMUNITY, "Welcome to the Club: community center, pool, block party & town outings"}, {LF_CLUB, "VIP: Club Glitz & the Velvet Room (18+)"},
                   {LF_ADULTGIGS, "After Dark: 18+ odd jobs & the tattoo apprentice"}, {LF_TRAVEL, "Passport: Coral Cove, Frostpeak & Belle Ville"},
                   {LF_FARM, "To Apple Valley!: the farm train"}, {LF_FARMBUILD, "Homesteader: coop, barn & silo"}, {LF_FARMHOUSE, "Country Living: buy a farmhouse 2nd home"}};
const char* unlockAt(int lv) { for (const Unl& u : UNL) if (u.lv == lv) return u.what; return nullptr; }
const char* TITLE[6] = {"Newbie", "Rising Star", "Local Celeb", "Town Icon", "Superstar", "LEGEND"};
const char* title() { return TITLE[std::min(5, L.level / 10)]; }
int perk() { return L.level >= 40 ? 100 : L.level >= 30 ? 50 : L.level >= 20 ? 20 : 0; }   // daily income perk
std::string rewardLine(int lv) {
    if (const char* u = unlockAt(lv)) return u;
    if (lv == 50) return "LEGEND: $10,000 and the Legend title";
    if (lv % 10 == 0) return std::string("Title: ") + TITLE[lv / 10] + " + $" + std::to_string(lv * 40) + (lv >= 20 ? " + daily income perk" : "");
    return "$" + std::to_string(lv * 25) + " Prestige bonus";
}
// daily tasks (ev bits come from lvlEvent; needs from the best value you reached today)
enum { EV_ODDJOB = 1, EV_OUTING = 2, EV_FARM = 4 };
struct Task { const char* name; int kind; int arg; };   // kind 0 need >= 70, 1 money up by arg, 2 event bit
const Task T[] = {{"Have a fun day (Fun 70+)", 0, N_FUN}, {"Be social (Social 70+)", 0, N_SOCIAL}, {"Stay fresh (Hygiene 70+)", 0, N_HYGIENE}, {"Eat well (Hunger 70+)", 0, N_HUNGER},
                  {"Get some rest (Energy 70+)", 0, N_ENERGY}, {"Make $50 today", 1, 50}, {"Make $200 today", 1, 200}, {"Work an odd job shift", 2, EV_ODDJOB},
                  {"Do something with the family", 2, EV_OUTING}, {"Put in farm work", 2, EV_FARM}};
const int NT = (int)(sizeof T / sizeof T[0]);
bool taskMet(int k) {
    const Task& t = T[k];
    if (t.kind == 0) return L.maxN[t.arg] >= 70;
    if (t.kind == 1) return G.money - L.money0 >= t.arg;
    return (L.ev & t.arg) != 0;
}
void rollTasks() {
    int a = irand(NT), b, c; do b = irand(NT); while (b == a); do c = irand(NT); while (c == a || c == b);
    if (L.level < LF_ODDJOBS) for (int* k : {&a, &b, &c}) if (T[*k].arg == EV_ODDJOB && T[*k].kind == 2) *k = 0;
    if (L.level < LF_FARM) for (int* k : {&a, &b, &c}) if (T[*k].arg == EV_FARM && T[*k].kind == 2) *k = 1;
    L.task[0] = (uint8_t)a; L.task[1] = (uint8_t)b; L.task[2] = (uint8_t)c; L.done = 0; L.ev = 0; L.money0 = G.money;
    for (auto& n : L.maxN) n = 0;
}
}

bool lvlLevels() { return L.mode == 1; }
bool lvlAllows(int f) { return L.mode == 0 || L.level >= f; }
bool lvlRoomOk(int r, std::string* why) {
    int f = 0;
    if (r == R_AVSQUARE || r == R_AVFARM || r == R_AVBARN) f = LF_FARM;
    else if (r == R_CLUB || r == R_VELVET) f = LF_CLUB;
    else if (r == R_COMMUNITY || r == R_BLOCKPARTY || r == R_POOL) f = LF_COMMUNITY;
    else if (r == R_MUSEUM || r == R_GARDENS || r == R_AQUARIUM || r == R_ROOFTOP || r == R_PAINTSIP) f = LF_ARTS;
    else if (r == R_FUNPARK || r == R_FUNRIDES) f = LF_FUNPARK;
    else if (r >= R_DOWNTOWN && r <= R_DTBISTRO) f = LF_DOWNTOWN;
    else if (r == R_CINEMA || (r >= R_GLOWBOWL && r <= R_SERENITY)) f = LF_NIGHTLIFE;
    else if (r == R_SALON) f = LF_SALON;
    else if (r == R_ROLLER || r == R_NIGHTMARKET) f = LF_RINK;
    else if (r == R_SUBWAY || r == R_PIXELARCADE) f = LF_METRO;
    else if ((r >= R_ISLAND && r <= R_TOWER) || (r >= R_COVESUITE && r <= R_BELLESIGHTS)) f = LF_TRAVEL;
    if (!f || lvlAllows(f)) return true;
    if (why) *why = "Locked until Level " + std::to_string(f) + " (Game Settings > Game Mode & Levels)";
    return false;
}
void lvlPrestige(int pts, const char* why) {
    if (L.mode != 1 || pts <= 0) return;
    L.prestige = (uint16_t)std::min(9999, L.prestige + pts);
    toast("+" + std::to_string(pts) + " Prestige (" + why + ")");
    while (L.level < LF_MAX && L.prestige >= needP(L.level + 1)) {
        L.level++;
        const int lv = L.level;
        if (const char* u = unlockAt(lv)) toast("LEVEL " + std::to_string(lv) + "! Unlocked - " + u);
        else if (lv == 50) { G.money += 10000; toast("LEVEL 50!! You're a LEGEND. +$10,000"); addMemory(MEM_GOAL, G.mc, -1, "Reached Level 50 - a true Legend!"); }
        else if (lv % 10 == 0) { G.money += lv * 40; toast("LEVEL " + std::to_string(lv) + "! New title: " + TITLE[lv / 10] + " (+$" + std::to_string(lv * 40) + ")"); }
        else { G.money += lv * 25; toast("LEVEL " + std::to_string(lv) + "! +$" + std::to_string(lv * 25)); }
        addMoodlet(MC(), "Leveled Up!", EM_CONFIDENT, 2, 720);
    }
}
void lvlEvent(int ev) { L.ev |= (uint8_t)ev; }
void lvlHourly(int hour) {
    if (L.mode != 1 || hour < 7 || hour > 23) return;
    const Sim& s = MC(); for (int n = 0; n < N_COUNT && n < 6; n++) L.maxN[n] = (uint8_t)std::max<int>(L.maxN[n], (int)s.needs[n]);
    for (int k = 0; k < 3; k++) if (!(L.done >> k & 1) && taskMet(L.task[k])) { L.done |= (uint8_t)(1 << k); lvlPrestige(2, T[L.task[k]].name); if (L.done == 7) lvlPrestige(2, "all 3 daily tasks!"); }
}
void lvlDaily() {
    if (L.mode != 1) return;
    if (L.lastDay == G.day) return;
    L.lastDay = (uint16_t)G.day; rollTasks();
    if (perk()) { G.money += perk(); toast(std::string(title()) + " perk: +$" + std::to_string(perk()) + " today."); }
}

LxMenu lvlMenu() {
    LxMenu m; char b[96];
    m.push_back({std::string("Game Mode: ") + (L.mode ? "LEVELS" : "ULTIMATE SANDBOX") + " (tap to switch)", 28001});
    if (!L.mode) { m.push_back({"Everything is unlocked. Switch to Levels to earn your way up.", 28999}); return m; }
    snprintf(b, sizeof b, "Level %d/50 (%s)   Prestige %d%s", L.level, title(), L.prestige, L.level < LF_MAX ? (" / " + std::to_string(needP(L.level + 1))).c_str() : " (MAX)"); m.push_back({b, 28999});
    m.push_back({"-- Today's tasks (+2 each, +2 for all 3) --", 28999});
    for (int k = 0; k < 3; k++) m.push_back({std::string(L.done >> k & 1 ? "[X] " : "[ ] ") + T[L.task[k]].name, 28999});
    if (perk()) { snprintf(b, sizeof b, "Perk: +$%d every day", perk()); m.push_back({b, 28999}); }
    m.push_back({"-- Level rewards --", 28999});
    for (int l = 2; l <= LF_MAX; l++) { snprintf(b, sizeof b, "%s Lv %d: %s", L.level >= l ? "[X]" : "[ ]", l, rewardLine(l).c_str()); m.push_back({b, 28999}); }
    m.push_back({"More Prestige: odd jobs, farm errands, fair ribbons, family outings", 28999});
    return m;
}
LxMenu lvlOption(int id) {
    if (id == 28000) return lvlMenu();
    if (id == 28001) {
        L.mode = (uint8_t)!L.mode;
        if (L.mode) { if (L.level < 1) L.level = 1; L.lastDay = 0xFFFF; lvlDaily(); toast("LEVELS mode: earn Prestige to unlock the world, one level at a time. (Your progress is kept if you switch back.)"); }
        else toast("ULTIMATE SANDBOX: everything is open. Have fun!");
        return lvlMenu();
    }
    return {};
}
void lvlSaveTo(const std::string& path) { FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fwrite(&L, sizeof L, 1, f); fclose(f); }
void lvlLoadFrom(const std::string& path) {
    L = Lv{}; FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    Lv t; const size_t n = fread(&t, 1, sizeof t, f); fclose(f);
    if (n == sizeof t && t.magic == 0x324C564Cu) L = t;
    if (L.level < 1 || L.level > LF_MAX) L.level = 1; if (L.mode > 1) L.mode = 0; for (auto& k : L.task) if (k >= NT) k = 0;
}
