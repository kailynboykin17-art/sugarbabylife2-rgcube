#include "farm.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "district.hpp"
#include "growup.hpp"
#include "tween.hpp"
#include "levels.hpp"
#include "extras.hpp"
#include "vfs.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
using namespace col;
void toast(const std::string& s);
void galleryFurnishStyle(int slot, int style);

#define fopen vfopen

namespace {
enum { CR_NONE, CR_TOMATO, CR_CARROT, CR_STRAWB, CR_POTATO, CR_CORN, CR_EGGPLANT, CR_PUMPKIN, CR_MELON, NCROP };
const char* CROP_N[NCROP] = {"-", "Tomato", "Carrot", "Strawberry", "Potato", "Corn", "Eggplant", "Pumpkin", "Watermelon"};
const int CROP_DAYS[NCROP] = {0, 3, 2, 3, 3, 4, 4, 5, 5};
const int CROP_SEED[NCROP] = {0, 4, 3, 5, 3, 5, 6, 8, 9};
const int CROP_SELL[NCROP] = {0, 9, 6, 10, 7, 11, 13, 22, 24};
const bool CROP_GIANT[NCROP] = {false, false, false, false, false, false, true, true, true};

struct FPlot { uint8_t crop, days, watered, giant, fert, pad[3]; };
struct Farm {
    uint32_t magic = 0x4D524146u;   // "FARM"
    uint8_t arrived = 0, townies = 0, coop = 0, barn = 0, silo = 0, alarm = 0, feeder = 0, pots = 0;
    uint16_t parts = 0, ribbons = 0, stitchXP = 0, hoops = 0;
    FPlot plot[8] = {}, pot[4] = {};
    uint8_t chickens = 0, golden = 0, cow = 0, llama = 0, goats = 0, sheep = 0, pigs = 0, ducks = 0;
    uint8_t mood[5] = {60, 60, 60, 60, 60};   // coop, stall (cow/llama), pen (goats/sheep), pigs & ducks, barn cleanliness
    uint8_t fed[4] = {}, petGoats = 0, petSheep = 0, cleaned = 0, pad0 = 0;
    uint16_t eggs = 0, goldEggs = 0, milk = 0, goatMilk = 0, wool = 0, truffles = 0, feathers = 0, jars = 0, pad1 = 0;
    uint16_t crop[NCROP] = {}, giants = 0;
    uint16_t nectar[12] = {};   // bottle ages in days (0 = empty slot, stored +1)
    uint8_t rabbit = 0, bird = 0, foxDays = 0, errand = 0, errGiver = 0, errNeed = 0, errCount = 0, fairWeek = 0;
    uint16_t lastFair = 0xFFFF, visits = 0;
    uint8_t farmhouse = 0, slot = 0, pad2[2] = {};   // second home: 0 none, 1 Apple Valley Farmhouse, 2 Red Barn Homestead; which home slot it lives in
} F;

int stage() { return MC().stage; }
bool pay(int c) { if (G.money < c) { toast("Not enough money ($" + std::to_string(c) + ")."); return false; } G.money -= c; return true; }
void need(int n, float d) { Sim& s = MC(); s.needs[n] = std::max(0.f, std::min(100.f, s.needs[n] + d)); }
std::string cash(int v) { return "$" + std::to_string(v); }

// ------------------------------------------------ Apple Valley locals
struct Local { const char* name; int stage; bool guy; const char* role; };
const Local LOC[8] = {{"Hattie Applewood", ST_ELDER, false, "the Mayor"}, {"Rusty Barrel", ST_MIDDLE, true, "the Cider Pub owner"}, {"Juniper Moss", ST_ADULT, false, "the Creature Keeper"},
                      {"Earl Bright", ST_ELDER, true, "an old farmer"}, {"Clementine Hayes", ST_TEEN, false, "a farm girl"}, {"Wade Colton", ST_YA, true, "a cowboy"},
                      {"Daisy Mae Porter", ST_YA, false, "the baker"}, {"Beau Whitaker", ST_ADULT, true, "the vet"}};
int findSim(const char* name) { for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].name == name) return i; return -1; }
void ensureLocals() {
    if (F.townies) return;
    for (const Local& l : LOC) if (findSim(l.name) < 0) { Sim s = randomSim((Stage)l.stage, l.guy ? 1 : 0); s.name = l.name; s.household = false; G.sims.push_back(s); }
    F.townies = 1;
}
int local(int k) { ensureLocals(); return findSim(LOC[k % 8].name); }
void chatLocal(int k, int fr) { const int i = local(k); if (i >= 0 && i != G.mc) changeRel(G.mc, i, fr, 0); }

// ------------------------------------------------ errands (villager favors)
struct Errand { int giver; const char* ask; int item; int count; int pay; int parts; };
enum { IT_EGGS, IT_MILK, IT_WOOL, IT_TOMATO, IT_JAR, IT_NECTAR, IT_PUMPKIN, IT_TRUFFLE, IT_FEATHER, IT_HOOP, IT_STRAWB, IT_GIANT };
const Errand ER[] = {
    {0, "The Mayor needs 4 eggs for the town breakfast.", IT_EGGS, 4, 60, 1}, {0, "The Mayor wants a giant crop for the town sign photo.", IT_GIANT, 1, 150, 2},
    {1, "Rusty needs 5 strawberries for cider.", IT_STRAWB, 5, 70, 1}, {1, "Rusty wants 2 jars of preserves for the pub.", IT_JAR, 2, 80, 1},
    {2, "Juniper needs 3 feathers for a nest project.", IT_FEATHER, 3, 55, 2}, {2, "Juniper wants 2 bottles of milk for the orphaned lambs.", IT_MILK, 2, 65, 2},
    {3, "Earl wants 6 tomatoes for his famous salsa.", IT_TOMATO, 6, 70, 1}, {3, "Earl needs 2 wool for his wife's knitting.", IT_WOOL, 2, 60, 1},
    {6, "Daisy Mae needs a pumpkin for pie.", IT_PUMPKIN, 1, 75, 1}, {7, "Beau the vet wants a truffle (don't ask).", IT_TRUFFLE, 1, 90, 2},
    {1, "Rusty wants an aged bottle of nectar.", IT_NECTAR, 1, 120, 2}, {4, "Clementine wants a cross-stitch hoop for her room.", IT_HOOP, 1, 70, 1},
};
const int NER = (int)(sizeof ER / sizeof ER[0]);
uint16_t& itemRef(int it) {
    static uint16_t dummy = 0;
    switch (it) { case IT_EGGS: return F.eggs; case IT_MILK: return F.milk; case IT_WOOL: return F.wool; case IT_TOMATO: return F.crop[CR_TOMATO]; case IT_JAR: return F.jars;
        case IT_PUMPKIN: return F.crop[CR_PUMPKIN]; case IT_TRUFFLE: return F.truffles; case IT_FEATHER: return F.feathers; case IT_HOOP: return F.hoops; case IT_STRAWB: return F.crop[CR_STRAWB]; case IT_GIANT: return F.giants; }
    return dummy;
}
int agedNectar() { int n = 0; for (uint16_t a : F.nectar) if (a > 7) n++; return n; }
int have(int it) { return it == IT_NECTAR ? agedNectar() : itemRef(it); }
void take(int it, int n) {
    if (it == IT_NECTAR) { for (auto& a : F.nectar) if (n && a > 7) { a = 0; n--; } return; }
    uint16_t& r = itemRef(it); r = (uint16_t)std::max(0, (int)r - n);
}
int offer(int k) { return (G.day * 3 + k * 5 + F.visits) % NER; }

const char* plotLine(const FPlot& p, int i, char* b, int n) {
    if (!p.crop) snprintf(b, n, "FPlot %d: empty (plant)", i + 1);
    else if (p.days >= CROP_DAYS[p.crop]) snprintf(b, n, "FPlot %d: %s%s READY!", i + 1, p.giant ? "GIANT " : "", CROP_N[p.crop]);
    else snprintf(b, n, "FPlot %d: %s %d/%d%s%s", i + 1, CROP_N[p.crop], p.days, CROP_DAYS[p.crop], p.watered ? "" : " - thirsty!", p.fert ? " (fert.)" : "");
    return b;
}
void growDay(FPlot& p, bool helpers) {
    if (!p.crop) return;
    if (p.watered || helpers && irand(3) == 0) { if (p.days < CROP_DAYS[p.crop]) p.days++; if (helpers && F.bird >= 50 && irand(4) == 0 && p.days < CROP_DAYS[p.crop]) p.days++; }
    p.watered = 0;
}
void harvest(FPlot& p) {
    if (!p.crop || p.days < CROP_DAYS[p.crop]) return;
    const int n = 1 + irand(2) + (F.rabbit >= 50 ? 1 : 0);
    if (p.giant) { F.giants++; toast(std::string("A GIANT ") + CROP_N[p.crop] + "! Perfect for the fair (or an errand)."); }
    else F.crop[p.crop] = (uint16_t)(F.crop[p.crop] + n);
    p = FPlot{};
}
int sellValue() {
    int v = 0; for (int c = 1; c < NCROP; c++) v += F.crop[c] * CROP_SELL[c];
    return v + F.eggs * 4 + F.goldEggs * 25 + F.milk * 6 + F.goatMilk * 8 + F.wool * 15 + F.truffles * 40 + F.feathers * 3 + F.jars * 18 + F.hoops * 45 + F.giants * 120;
}
const char* FH_N[3] = {"", "Apple Valley Farmhouse", "Red Barn Homestead"};
void goSlot(int to) {   // the whole household walks over to the other home
    const int from = G.house; if (from == to) return;
    for (int r = 0; r < R_COUNT; r++) {
        auto& v = G.rooms[from][r].actors;
        for (size_t i = 0; i < v.size();) {
            if (v[i].kind == 1 || (v[i].id < G.sims.size() && G.sims[v[i].id].household)) { Actor a = v[i]; a.x = 60 + irand(280); a.tx = a.x; a.y = 200 + irand(30);
                G.rooms[to][a.kind == 0 && G.sims[a.id].stage == ST_INFANT ? R_NURSERY : R_LIVING].actors.push_back(a); v.erase(v.begin() + i); }
            else i++;
        }
    }
    G.house = to; G.room = R_LIVING;
}
}  // namespace

void farmOnMainMove(int from, int to, bool furnished) {
    if (!F.farmhouse || F.slot != to) return;
    F.slot = (uint8_t)from;                       // the farmhouse takes the slot the main home just left
    if (furnished) { for (int r = 0; r < R_COUNT; r++) if (roomIsHome(r)) G.rooms[from][r].items.clear(); galleryFurnishStyle(from, 8); }   // (unfurnished moves swap furniture, so the farm keeps its own)
    G.houseType[from] = 1;
}
LxMenu farmHomeRows() {
    LxMenu m;
    if (!F.farmhouse) return m;
    if (G.house == F.slot && !farmRoom(G.room)) m.push_back({"Head Back to Your Main Home", 27721});
    else m.push_back({std::string("Go to Your ") + FH_N[F.farmhouse] + " (2nd home)", 27720});
    return m;
}

bool farmRoom(int r) { return r == R_AVSQUARE || r == R_AVFARM || r == R_AVBARN; }

void farmArrive(int room) {
    if (!farmRoom(room)) return;
    F.visits++;
    if (F.arrived) return;
    F.arrived = 1; ensureLocals();
    if (G.money >= 100) { G.money -= 100; toast("You bought a $100 Apple Valley rail pass - ride the train out any time now."); }
    else toast("The conductor waves you on: \"First ride's on Apple Valley, sugar.\"");
    toast("Mayor Hattie Applewood: \"Welcome to Apple Valley! Sunny Acres has been empty too long - the land's yours to work, darlin'.\"");
    toast("Tip: plant at Sunny Acres, build a coop & barn at the General Store, and enter the fair every Saturday.");
    chatLocal(0, 15);
    for (int k = 0; k < 4; k++) if (!F.plot[k].crop) { F.plot[k].crop = (uint8_t)(1 + k % 3); F.plot[k].days = 0; }   // a starter garden
}

std::vector<std::pair<std::string, int>> farmSpotOptions(int k) {
    std::vector<std::pair<std::string, int>> v; char b[96];
    switch (k) {
    case SP_AVSTORE:
        snprintf(b, sizeof b, "Sell All Farm Goods (%s)", cash(sellValue()).c_str()); v.push_back({b, 27641});
        if (!F.coop) v.push_back({"Build a Chicken Coop ($600)", 27642});
        if (!F.barn) v.push_back({"Build the Big Red Barn ($1,500)", 27643});
        if (!F.silo) v.push_back({"Build a Silo ($900, feed costs half)", 27644});
        if (F.pots < 4) v.push_back({"Home Garden Pot Kit ($30)", 27645});
        if (!F.farmhouse) { v.push_back({"Buy the Apple Valley Farmhouse - 2nd home ($38,000)", 27647}); v.push_back({"Buy the Red Barn Homestead - 2nd home ($64,000)", 27648}); }
        else v.push_back({std::string("Go Inside Your ") + FH_N[F.farmhouse], 27720});
        v.push_back({"What's in my farm bag?", 27646}); break;
    case SP_AVERRAND:
        if (F.errand) { const Errand& e = ER[F.errand - 1]; snprintf(b, sizeof b, "Turn in: %s (%d/%d)", e.ask, std::min(have(e.item), e.count), e.count); v.push_back({b, 27664}); v.push_back({"Drop this errand", 27665}); }
        else for (int i = 0; i < 3; i++) { const Errand& e = ER[offer(i)]; snprintf(b, sizeof b, "%s (%s + %d part%s)", e.ask, cash(e.pay).c_str(), e.parts, e.parts > 1 ? "s" : ""); v.push_back({b, 27661 + i}); }
        snprintf(b, sizeof b, "Upgrade parts: %d", F.parts); v.push_back({b, 27999}); break;
    case SP_AVFAIR: {
        static const char* TH[4] = {"Livestock Show", "Giant Crop Contest", "Pie Bake-Off", "Wool & Stitch Show"};
        const int th = (G.day / 7) % 4;
        snprintf(b, sizeof b, "This week: %s (Saturdays)", TH[th]); v.push_back({b, 27999});
        if (G.day % 7 == 5) v.push_back({"Enter the Apple Valley Fair!", 27681}); else v.push_back({"(Come back Saturday!)", 27999});
        snprintf(b, sizeof b, "Ribbons won: %d", F.ribbons); v.push_back({b, 27999}); break; }
    case SP_AVPUB: v.push_back({"Hard Cider ($6)", 27701}); v.push_back({"Meet the Locals", 27702}); v.push_back({"Line Dancing Night", 27703}); v.push_back({"Ask About Town Gossip", 27704}); break;
    case SP_AVGARDEN:
        for (int i = 0; i < 8; i++) v.push_back({plotLine(F.plot[i], i, b, sizeof b), 27010 + i});
        v.push_back({"Water Everything", 27360}); v.push_back({"Harvest Everything", 27361}); break;
    case SP_AVMEADOW:
        snprintf(b, sizeof b, "Befriend the Wild Rabbits (%d/100 - they eat weeds)", F.rabbit); v.push_back({b, 27362});
        snprintf(b, sizeof b, "Feed the Songbirds (%d/100 - they eat bugs)", F.bird); v.push_back({b, 27363});
        if (F.foxDays) v.push_back({"!! Shoo the Fox Away", 27364});
        v.push_back({"Forage for Wild Berries", 27365}); break;
    case SP_AVCOOP:
        if (!F.coop) { v.push_back({"(Build a coop at the General Store first)", 27999}); break; }
        snprintf(b, sizeof b, "Chickens: %d/8 (%d golden)  Mood %d", F.chickens, F.golden, F.mood[0]); v.push_back({b, 27999});
        if (F.chickens < 8) v.push_back({"Buy a Chicken ($40)", 27401});
        v.push_back({"Feed the Chickens", 27402}); v.push_back({"Collect Eggs", 27403});
        if (F.chickens > F.golden) v.push_back({"Give a Golden Treat ($15)", 27404});
        if (!F.alarm) v.push_back({"Install Coop Alarm (2 parts) - stops foxes", 27405}); break;
    case SP_AVSTALL:
        if (!F.barn) { v.push_back({"(Build the barn at the General Store first)", 27999}); break; }
        if (!F.cow) v.push_back({"Buy a Cow ($300)", 27421}); if (!F.llama) v.push_back({"Buy a Llama ($350 - scares foxes)", 27422});
        if (F.cow || F.llama) { v.push_back({"Feed the Big Animals", 27423}); if (F.cow) v.push_back({"Milk the Cow", 27424}); if (F.llama) v.push_back({"Shear the Llama", 27425}); }
        v.push_back({"Muck Out the Barn", 27426});
        if (!F.feeder) v.push_back({"Install Auto-Feeder (3 parts)", 27427}); break;
    case SP_AVPEN:
        if (!F.barn) { v.push_back({"(Build the barn at the General Store first)", 27999}); break; }
        snprintf(b, sizeof b, "Mini Goats %d/3, Mini Sheep %d/3  Mood %d", F.goats, F.sheep, F.mood[2]); v.push_back({b, 27999});
        if (F.goats < 3) v.push_back({"Adopt a Mini Goat ($150)", 27441}); if (F.sheep < 3) v.push_back({"Adopt a Mini Sheep ($150)", 27442});
        if (F.goats) { v.push_back({"Cuddle the Mini Goats (de-stress)", 27443}); v.push_back({"Milk the Mini Goats", 27445}); }
        if (F.sheep) { v.push_back({"Cuddle the Mini Sheep (de-stress)", 27444}); v.push_back({"Shear the Mini Sheep", 27446}); } break;
    case SP_AVPIGS:
        if (!F.barn) { v.push_back({"(Build the barn at the General Store first)", 27999}); break; }
        if (F.pigs < 2) v.push_back({"Buy a Pig ($200)", 27461}); if (F.ducks < 4) v.push_back({"Buy a Duck ($60)", 27462});
        if (F.pigs || F.ducks) v.push_back({"Feed Pigs & Ducks", 27463});
        if (F.pigs) v.push_back({"Truffle Hunt with the Pigs", 27464});
        if (F.ducks) v.push_back({"Collect Duck Feathers", 27465}); break;
    case SP_AVLOOM:
        snprintf(b, sizeof b, "Cross-Stitch level %d  (wool: %d, hoops: %d)", 1 + std::min(4, F.stitchXP / 6), F.wool, F.hoops); v.push_back({b, 27999});
        v.push_back({"Stitch a Pattern Hoop (1 wool)", 27481}); v.push_back({"Stitch From a Reference Photo (2 wool)", 27482}); break;
    case SP_AVNECTAR: {
        int n = 0; for (uint16_t a : F.nectar) n += a > 0;
        snprintf(b, sizeof b, "Nectar rack: %d/12 bottles (%d aged 7+ days)", n, agedNectar()); v.push_back({b, 27999});
        v.push_back({"Stomp Fruit into Nectar (3 strawberries / 3 potatoes)", 27601});
        if (agedNectar()) { v.push_back({"Drink Vitality Nectar (aged)", 27604}); v.push_back({"Sell Aged Nectar", 27603}); }
        if (n) v.push_back({"Sell Fresh Nectar", 27602});
        v.push_back({"Can Strawberry Jam (3 strawberries)", 27621}); v.push_back({"Can Tomato Sauce (3 tomatoes)", 27622});
        v.push_back({"Can Custard (2 eggs + 1 milk)", 27623}); v.push_back({"Farm Breakfast (2 eggs + 1 milk)", 27624});
        v.push_back({"Bake a Fair Pie (pumpkin or 4 strawberries)", 27625}); break; }
    }
    return v;
}

LxMenu farmHomePots() {
    LxMenu m; char b[96];
    if (!F.pots) { m.push_back({"No pots yet - buy a Home Garden Pot Kit at the Apple Valley General Store.", 27999}); return m; }
    m.push_back({"Home Garden Pots (water daily!)", 27999});
    for (int i = 0; i < F.pots; i++) { plotLine(F.pot[i], i, b, sizeof b); std::string s = b; s.replace(0, 4, "Pot"); m.push_back({s, 27510 + i}); }
    return m;
}

LxMenu farmOption(int id) {
    LxMenu m; char b[96]; Sim& me = MC();
    if (id == 27999) return {};
    if ((id == 27500 || id == 27645 || (id >= 27510 && id < 27580)) && !lvlAllows(LF_POTS)) { toast("Home garden pots unlock at Level 7."); return {}; }
    if ((id == 27642 || id == 27643 || id == 27644) && !lvlAllows(LF_FARMBUILD)) { toast("Farm buildings unlock at Level 35."); return {}; }
    if ((id == 27647 || id == 27648) && !lvlAllows(LF_FARMHOUSE)) { toast("The farmhouse unlocks at Level 37."); return {}; }
    if (id == 27500) return farmHomePots();
    if (id < 27500 || id >= 27600) lvlEvent(LEV_FARM);
    // ---- plots (27010+p) & pots (27510+p): per-slot menu
    if ((id >= 27010 && id < 27018) || (id >= 27510 && id < 27514)) {
        const bool pot = id >= 27510; const int p = id - (pot ? 27510 : 27010); FPlot& P = pot ? F.pot[p] : F.plot[p];
        m.push_back({plotLine(P, p, b, sizeof b), 27999});
        if (!P.crop) { for (int c = 1; c < NCROP; c++) { if (pot && CROP_GIANT[c]) continue; snprintf(b, sizeof b, "Plant %s (%s, %d days)", CROP_N[c], cash(CROP_SEED[c]).c_str(), CROP_DAYS[c]); m.push_back({b, (pot ? 27520 + p * 8 : 27100 + p * 16) + c}); } return m; }
        if (P.days >= CROP_DAYS[P.crop]) { m.push_back({"Harvest", pot ? 27570 + p : 27340 + p}); return m; }
        if (!P.watered) m.push_back({"Water", pot ? 27560 + p : 27300 + p});
        if (!pot && !P.fert && CROP_GIANT[P.crop]) m.push_back({"Giant-Grow Fertilizer ($12)", 27320 + p});
        if (!pot && !P.fert && !CROP_GIANT[P.crop]) m.push_back({"Fertilizer ($6, grows faster)", 27320 + p});
        return m;
    }
    if ((id >= 27100 && id < 27228) || (id >= 27520 && id < 27552)) {
        const bool pot = id >= 27520; const int p = pot ? (id - 27520) / 8 : (id - 27100) / 16, c = pot ? (id - 27520) % 8 : (id - 27100) % 16;
        if (c <= 0 || c >= NCROP) return {};
        if (!pay(CROP_SEED[c])) return {};
        FPlot& P = pot ? F.pot[p] : F.plot[p]; P = FPlot{}; P.crop = (uint8_t)c; P.watered = 1;
        need(N_FUN, 4); toast(std::string("Planted ") + CROP_N[c] + " and gave it a drink."); return {};
    }
    if (id >= 27300 && id < 27308) { F.plot[id - 27300].watered = 1; toast("Watered."); return {}; }
    if (id >= 27560 && id < 27564) { F.pot[id - 27560].watered = 1; toast("Watered your pot plant."); return {}; }
    if (id >= 27320 && id < 27328) { FPlot& P = F.plot[id - 27320]; const int c = CROP_GIANT[P.crop] ? 12 : 6; if (!pay(c)) return {};
        P.fert = 1; if (CROP_GIANT[P.crop]) { P.giant = irand(100) < 55 ? 1 : 0; toast(P.giant ? "It's getting... BIG. Giant crop incoming!" : "Fertilized. It's healthy, but not giant this time."); }
        else { if (P.days < CROP_DAYS[P.crop]) P.days++; toast("Fertilized - it shot up a day!"); } return {}; }
    if (id >= 27340 && id < 27348) { harvest(F.plot[id - 27340]); need(N_FUN, 6); toast("Harvested! Check your farm bag at the General Store."); return {}; }
    if (id >= 27570 && id < 27574) { harvest(F.pot[id - 27570]); toast("Harvested from your home pot!"); return {}; }
    switch (id) {
    case 27360: for (auto& p : F.plot) if (p.crop) p.watered = 1; need(N_ENERGY, -6); toast("Every plot watered by hand. Your arms are tired."); return {};
    case 27361: { int n = 0; for (auto& p : F.plot) if (p.crop && p.days >= CROP_DAYS[p.crop]) { harvest(p); n++; } toast(n ? "Harvested " + std::to_string(n) + " plots!" : "Nothing's ready yet."); return {}; }
    case 27362: F.rabbit = (uint8_t)std::min(100, F.rabbit + 12 + irand(8)); need(N_FUN, 6); toast(F.rabbit >= 50 ? "The bunnies trust you! They're munching your weeds (bigger harvests)." : "A bunny sniffed your hand. Progress!"); return {};
    case 27363: F.bird = (uint8_t)std::min(100, F.bird + 12 + irand(8)); toast(F.bird >= 50 ? "The songbirds patrol your garden for bugs now (crops grow faster)." : "A robin landed on the fence. Cute."); return {};
    case 27364: F.foxDays = 0; need(N_ENERGY, -5); toast("You clanged a pot lid - the fox bolted into the woods."); return {};
    case 27365: { const int n = 1 + irand(3); F.crop[CR_STRAWB] = (uint16_t)(F.crop[CR_STRAWB] + n); need(N_FUN, 5); toast("Found " + std::to_string(n) + " wild strawberries along the fence line."); return {}; }
    // ---- coop
    case 27401: if (pay(40)) { F.chickens++; toast("A new hen! She immediately judged you."); } return {};
    case 27402: if (pay(F.silo ? 2 : 4)) { F.fed[0] = 1; F.mood[0] = (uint8_t)std::min(100, F.mood[0] + 10); toast("Bawk bawk! The chickens are fed."); } return {};
    case 27403: { if (!F.chickens) { toast("No chickens yet."); return {}; } toast("Egg basket checked (eggs come every morning if they're fed)."); return {}; }
    case 27404: if (pay(15)) { F.golden++; toast("A hen ate the golden treat... her feathers are GOLD. Golden eggs ahead!"); } return {};
    case 27405: if (F.parts < 2) { toast("Need 2 upgrade parts (do errands)."); return {}; } F.parts -= 2; F.alarm = 1; toast("Coop alarm installed. Foxes beware."); return {};
    // ---- barn
    case 27421: if (pay(300)) { F.cow = 1; toast("Meet Buttercup the cow! (Name not negotiable.)"); } return {};
    case 27422: if (pay(350)) { F.llama = 1; toast("A llama! She already spat on the mail carrier. Foxes won't come near now."); } return {};
    case 27423: if (pay(F.silo ? 4 : 8)) { F.fed[1] = 1; F.mood[1] = (uint8_t)std::min(100, F.mood[1] + 10); toast("Hay for everyone."); } return {};
    case 27424: if (!F.fed[1]) { toast("Feed her first - happy cows give milk."); return {}; } F.milk = (uint16_t)(F.milk + 1 + (F.mood[1] > 70)); need(N_ENERGY, -4); toast("Fresh milk!"); F.fed[1] = 2; return {};
    case 27425: if (F.mood[1] < 40) { toast("The llama is not in the mood. (Feed her & clean the barn.)"); return {}; } F.wool = (uint16_t)(F.wool + 2); toast("Sheared! 2 bundles of fluffy llama wool."); F.mood[1] -= 10; return {};
    case 27426: F.cleaned = 1; F.mood[4] = 100; need(N_HYGIENE, -20); need(N_ENERGY, -8); toast("Barn mucked out. You smell... rustic."); return {};
    case 27427: if (F.parts < 3) { toast("Need 3 upgrade parts (do errands)."); return {}; } F.parts -= 3; F.feeder = 1; toast("Auto-feeder installed - the animals eat even when you're away."); return {};
    // ---- pen
    case 27441: if (pay(150)) { F.goats++; toast("A mini goat! It's already on the roof somehow."); } return {};
    case 27442: if (pay(150)) { F.sheep++; toast("A mini sheep! Like a cloud with legs."); } return {};
    case 27443: case 27444: { need(N_FUN, 20); addMoodlet(me, "Mini Animal Therapy", EM_HAPPY, 2, 300); F.mood[2] = (uint8_t)std::min(100, F.mood[2] + 8); tweenPlay(G.mc, TW_HOP);
        toast(id == 27443 ? "The mini goats headbutt you lovingly. Stress: gone." : "You hug a mini sheep. It's SO soft. Stress: gone."); return {}; }
    case 27445: { if (!F.goats) return {}; F.goatMilk = (uint16_t)(F.goatMilk + F.goats); toast("Mini goat milk collected (sells for more than cow milk!)."); return {}; }
    case 27446: { if (!F.sheep) return {}; if (F.mood[2] < 40) { toast("They're grumpy. Cuddle them first."); return {}; } F.wool = (uint16_t)(F.wool + F.sheep); F.mood[2] -= 10; toast("Mini sheep sheared - fluffy wool!"); return {}; }
    // ---- pigs & ducks
    case 27461: if (pay(200)) { F.pigs++; toast("Oink! A truffle pig joins the farm."); } return {};
    case 27462: if (pay(60)) { F.ducks++; toast("A duck waddles in and claims the pond."); } return {};
    case 27463: if (pay(F.silo ? 3 : 6)) { F.fed[3] = 1; F.mood[3] = (uint8_t)std::min(100, F.mood[3] + 10); toast("Slop for the pigs, corn for the ducks."); } return {};
    case 27464: { if (!F.fed[3]) { toast("Hungry pigs don't hunt - feed them first."); return {}; } need(N_FUN, 10); need(N_ENERGY, -8);
        if (irand(100) < 35 + F.pigs * 15) { F.truffles++; toast("The pig went WILD - a black truffle! ($40)"); } else toast("The pig found... a stick. And mud. Mostly mud."); F.fed[3] = 2; return {}; }
    case 27465: F.feathers = (uint16_t)(F.feathers + F.ducks); toast("Collected " + std::to_string(F.ducks) + " soft duck feathers."); return {};
    // ---- cross-stitch
    case 27481: case 27482: { const int w = id == 27481 ? 1 : 2; if (F.wool < w) { toast("Need wool - shear your sheep or llama."); return {}; }
        F.wool = (uint16_t)(F.wool - w); F.hoops++; F.stitchXP = (uint16_t)(F.stitchXP + w * 2); need(N_FUN, 12);
        static const char* PAT[5] = {"a strawberry", "'Home Sweet Farm'", "a llama in a sweater", "the Big Red Barn", "your family portrait"};
        toast(std::string("You stitched ") + (id == 27482 ? PAT[4] : PAT[irand(4)]) + ". Gorgeous!"); return {}; }
    // ---- nectar
    case 27601: { int src = F.crop[CR_STRAWB] >= 3 ? CR_STRAWB : F.crop[CR_POTATO] >= 3 ? CR_POTATO : 0; if (!src) { toast("Need 3 strawberries or 3 potatoes."); return {}; }
        int slot = -1; for (int i = 0; i < 12; i++) if (!F.nectar[i]) { slot = i; break; } if (slot < 0) { toast("The rack is full - sell some nectar."); return {}; }
        F.crop[src] = (uint16_t)(F.crop[src] - 3); F.nectar[slot] = 1; need(N_FUN, 10); tweenPlay(G.mc, TW_STOMP);
        toast(src == CR_STRAWB ? "Stomp stomp stomp! Strawberry nectar on the rack. Let it age!" : "Potato nectar? Bold. It's on the rack aging."); return {}; }
    case 27602: { int n = 0, v = 0; for (auto& a : F.nectar) if (a && a <= 7) { v += 10 + a * 3; a = 0; n++; } G.money += v; toast("Sold " + std::to_string(n) + " fresh bottles for " + cash(v) + "."); return {}; }
    case 27603: { int n = 0, v = 0; for (auto& a : F.nectar) if (a > 7) { v += 30 + std::min(60, (int)a) * 4; a = 0; n++; } G.money += v; toast("Sold " + std::to_string(n) + " aged bottles for " + cash(v) + "! The older the better."); return {}; }
    case 27604: { for (auto& a : F.nectar) if (a > 7) { a = 0; break; } me.needs[N_ENERGY] = 100; need(N_FUN, 10); addMoodlet(me, "Vitality Nectar", EM_ENERGIZED, 2, 600); toast("Vitality Nectar! You feel like you could plow a field."); return {}; }
    // ---- stove
    case 27621: if (F.crop[CR_STRAWB] < 3) { toast("Need 3 strawberries."); return {}; } F.crop[CR_STRAWB] -= 3; F.jars++; toast("Strawberry jam jarred!"); return {};
    case 27622: if (F.crop[CR_TOMATO] < 3) { toast("Need 3 tomatoes."); return {}; } F.crop[CR_TOMATO] -= 3; F.jars++; toast("Tomato sauce canned!"); return {};
    case 27623: if (F.eggs < 2 || F.milk + F.goatMilk < 1) { toast("Need 2 eggs + 1 milk."); return {}; } F.eggs -= 2; if (F.milk) F.milk--; else F.goatMilk--; F.jars++; toast("Vanilla custard jarred. Fancy."); return {};
    case 27624: if (F.eggs < 2 || F.milk + F.goatMilk < 1) { toast("Need 2 eggs + 1 milk."); return {}; } F.eggs -= 2; if (F.milk) F.milk--; else F.goatMilk--; me.needs[N_HUNGER] = 100; addMoodlet(me, "Farm-Fresh Breakfast", EM_HAPPY, 2, 360); toast("Fresh eggs, fresh milk. Best breakfast of your LIFE."); return {};
    case 27625: if (F.crop[CR_PUMPKIN]) F.crop[CR_PUMPKIN]--; else if (F.crop[CR_STRAWB] >= 4) F.crop[CR_STRAWB] -= 4; else { toast("Need a pumpkin or 4 strawberries."); return {}; }
        F.jars++; toast("A blue-ribbon-worthy pie! (Counts as a jar for errands & the Pie Bake-Off.)"); return {};
    // ---- store
    case 27641: { const int v = sellValue(); if (!v) { toast("Your farm bag is empty."); return {}; } G.money += v;
        for (auto& c : F.crop) c = 0; F.eggs = F.goldEggs = F.milk = F.goatMilk = F.wool = F.truffles = F.feathers = F.jars = F.hoops = F.giants = 0;
        toast("The General Store paid " + cash(v) + " for your farm goods!"); return {}; }
    case 27642: if (pay(600)) { F.coop = 1; toast("A cute red coop goes up at Sunny Acres!"); } return {};
    case 27643: if (pay(1500)) { F.barn = 1; toast("The Big Red Barn is built! Animal stalls & pens are open."); guPhoto("Raising the Big Red Barn"); } return {};
    case 27644: if (pay(900)) { F.silo = 1; toast("Silo built - animal feed costs half now."); } return {};
    case 27645: if (pay(30)) { F.pots++; toast("Pot kit bought! Plant & water it from Daily Life > Home Garden Pots."); } return {};
    case 27646: { snprintf(b, sizeof b, "Eggs %d (gold %d), Milk %d, Goat milk %d, Wool %d", F.eggs, F.goldEggs, F.milk, F.goatMilk, F.wool); m.push_back({b, 27999});
        snprintf(b, sizeof b, "Truffles %d, Feathers %d, Jars %d, Hoops %d, Giant crops %d", F.truffles, F.feathers, F.jars, F.hoops, F.giants); m.push_back({b, 27999});
        std::string s = "Crops:"; for (int c = 1; c < NCROP; c++) if (F.crop[c]) s += " " + std::string(CROP_N[c]) + " " + std::to_string(F.crop[c]);
        m.push_back({s == "Crops:" ? "Crops: none" : s, 27999}); return m; }
    case 27647: case 27648: {
        if (F.farmhouse) return {};
        if (stage() < ST_YA) { toast("You need to be a young adult to buy a home."); return {}; }
        if (!pay(id == 27647 ? 38000 : 64000)) return {};
        F.farmhouse = (uint8_t)(id - 27646); F.slot = (uint8_t)(1 - G.house);
        for (int r = 0; r < R_COUNT; r++) if (roomIsHome(r)) { G.rooms[F.slot][r].items.clear(); G.rooms[F.slot][r].actors.clear(); }
        galleryFurnishStyle(F.slot, 8); G.houseType[F.slot] = id == 27648 ? 2 : 1;
        addMemory(MEM_HOME, G.mc, -1, std::string("Bought the ") + FH_N[F.farmhouse] + " in Apple Valley - a second home for the family!");
        guPhoto(std::string("Keys to the ") + FH_N[F.farmhouse]); addMoodlet(me, "Farmhouse Dreams", EM_HAPPY, 3, 1440);
        toast(std::string("The ") + FH_N[F.farmhouse] + " is yours - a second home! Your main house stays exactly as it is."); return {}; }
    case 27720: if (!F.farmhouse) return {}; goSlot(F.slot); toast(std::string("Welcome to the ") + FH_N[F.farmhouse] + "! The whole family came along."); return {};
    case 27721: goSlot(1 - F.slot); toast("Back at your main home."); return {};
    // ---- errands
    case 27661: case 27662: case 27663: { const int e = offer(id - 27661); F.errand = (uint8_t)(e + 1); toast(std::string("Errand taken: ") + ER[e].ask); return {}; }
    case 27664: { if (!F.errand) return {}; const Errand& e = ER[F.errand - 1];
        if (have(e.item) < e.count) { toast("Not enough yet - " + std::to_string(have(e.item)) + "/" + std::to_string(e.count) + "."); return {}; }
        take(e.item, e.count); G.money += e.pay; lvlPrestige(3, "farm errand"); F.parts = (uint16_t)(F.parts + e.parts); chatLocal(e.giver, 10); F.errand = 0;
        toast(std::string(LOC[e.giver].name) + ": \"Bless your heart!\" +" + cash(e.pay) + " and " + std::to_string(e.parts) + " upgrade part" + (e.parts > 1 ? "s." : ".")); return {}; }
    case 27665: F.errand = 0; toast("Errand dropped. The town will survive."); return {};
    // ---- fair
    case 27681: {
        if (F.lastFair == G.day) { toast("You already entered today's fair."); return {}; }
        const int th = (G.day / 7) % 4; int q = -1;
        if (th == 0) { const int best = std::max({(int)F.mood[0] * (F.chickens > 0), (int)F.mood[1] * (F.cow || F.llama), (int)F.mood[2] * (F.goats || F.sheep), (int)F.mood[3] * (F.pigs || F.ducks)}); if (best) q = best; }
        if (th == 1 && F.giants) { F.giants--; q = 70 + irand(31); }
        if (th == 2 && F.jars) { F.jars--; q = 55 + irand(40); }
        if (th == 3 && (F.hoops || F.wool)) { if (F.hoops) F.hoops--; else F.wool--; q = 50 + F.stitchXP * 2 + irand(30); }
        if (q < 0) { static const char* NEED[4] = {"an animal", "a giant crop", "a pie or jar", "wool or a stitched hoop"}; toast(std::string("You need ") + NEED[th] + " to enter this week."); return {}; }
        F.lastFair = G.day; const int score = q + irand(25) - 10;
        if (score >= 90) { F.ribbons++; G.money += 250; toast("BLUE RIBBON! The Mayor pins it on you herself. +$250"); guPhoto("Blue ribbon at the Apple Valley Fair"); addMoodlet(me, "Blue Ribbon!", EM_CONFIDENT, 3, 1440); }
        else if (score >= 70) { F.ribbons++; G.money += 120; toast("Red ribbon - second place! +$120"); }
        else if (score >= 50) { F.ribbons++; G.money += 50; toast("White ribbon. Third place still counts! +$50"); }
        if (score >= 50) lvlPrestige(3, "fair ribbon");
        else toast("No ribbon this time. Earl Bright won again. He ALWAYS wins.");
        need(N_FUN, 20); chatLocal(0, 4); return {}; }
    // ---- pub
    case 27701: if (pay(6)) { need(N_FUN, 10); need(N_SOCIAL, 10); toast(me.stage >= ST_YA ? "Rusty's hard cider hits different." : "Rusty slides you a sparkling apple juice instead. Nice try."); } return {};
    case 27702: { const int k = irand(8); const int i = local(k); chatLocal(k, 8); need(N_SOCIAL, 20); if (i >= 0) toast("You met " + G.sims[i].name + ", " + LOC[k].role + "."); return {}; }
    case 27703: need(N_FUN, 25); need(N_ENERGY, -10); for (int k = 0; k < 3; k++) chatLocal(irand(8), 3); tweenPlay(G.mc, TW_STOMP); toast("Boot-scootin' boogie! The whole pub line dances."); return {};
    case 27704: { static const char* GO[5] = {"Earl's been secretly feeding his prize pig candy before the fair.", "Daisy Mae and Wade were seen sharing a milkshake...", "The Mayor's llama has a fan club now.", "Someone keeps stealing pies off windowsills.", "Juniper swears a fox can talk. Nobody believes her."};
        need(N_SOCIAL, 8); toast(std::string("Pub gossip: ") + GO[irand(5)]); return {}; }
    }
    return {};
}

void farmDaily() {
    if (!F.arrived && !F.pots) return;
    const bool helpers = F.rabbit >= 50 || F.bird >= 50;
    for (auto& p : F.plot) growDay(p, helpers);
    for (int i = 0; i < F.pots; i++) growDay(F.pot[i], false);
    // animals: auto-feeder covers anyone you forgot (for a fee)
    if (F.feeder) for (int g = 0; g < 4; g++) if (!F.fed[g]) { const int c = F.silo ? 3 : 6; if (G.money >= c) { G.money -= c; F.fed[g] = 1; } }
    if (F.chickens) { if (F.fed[0]) { F.eggs = (uint16_t)(F.eggs + F.chickens - F.golden); F.goldEggs = (uint16_t)(F.goldEggs + F.golden); } F.mood[0] = (uint8_t)std::max(0, F.mood[0] + (F.fed[0] ? 3 : -12)); }
    if (F.cow || F.llama) F.mood[1] = (uint8_t)std::max(0, std::min(100, F.mood[1] + (F.fed[1] ? 3 : -12) + (F.mood[4] > 50 ? 2 : -4)));
    if (F.goats || F.sheep) F.mood[2] = (uint8_t)std::max(0, std::min(100, F.mood[2] + (F.fed[2] || F.fed[1] ? 2 : -6)));
    if (F.pigs || F.ducks) { F.mood[3] = (uint8_t)std::max(0, std::min(100, F.mood[3] + (F.fed[3] ? 3 : -10))); if (F.fed[3] && F.ducks) F.feathers = (uint16_t)(F.feathers + (irand(2) ? 1 : 0)); }
    if (F.barn) F.mood[4] = (uint8_t)std::max(0, F.mood[4] - 15);
    for (auto& f : F.fed) f = 0;
    for (auto& a : F.nectar) if (a && a < 365) a++;
    // wild foxes (llamas & coop alarms keep them away)
    if (F.foxDays) {
        if (F.chickens && !F.alarm && !F.llama) { if (F.eggs) { F.eggs = (uint16_t)(F.eggs / 2); toast("A fox raided the coop overnight and stole eggs! (Shoo it from the meadow, or get a llama / coop alarm.)"); }
                                                   else if (irand(3) == 0 && F.chickens > 1) { F.chickens--; if (F.golden > F.chickens) F.golden = F.chickens; toast("A fox got one of your hens. RIP, Henrietta."); } }
        if (++F.foxDays > 3) F.foxDays = 0;
    } else if (F.chickens && irand(100) < 12) { F.foxDays = 1; if (!F.alarm && !F.llama) toast("A fox was spotted near Sunny Acres..."); }
    if (G.day % 7 == 5 && F.arrived) toast("It's Saturday - the Apple Valley Fair is on in the square!");
}

void farmDrawRoom(int room, float t, bool night) {
    grad(0, 0, TOP_W, 120, C(150, 210, 250), C(220, 240, 250));
    circle(340, 34, 16, C(255, 236, 140));
    for (int i = 0; i < 4; i++) circle(30 + i * 110.f + std::fmod(t * 4, 110.f), 30 + (i % 2) * 10, 14, C(255, 255, 255, 200));
    for (int i = 0; i < 5; i++) circle(i * 100.f, 128, 36, C(i % 2 ? RGB{120, 190, 100} : RGB{100, 175, 90}));   // rolling hills
    rect(0, 120, TOP_W, 38, C(110, 180, 95));
    if (room == R_AVBARN) { rect(0, 158, TOP_W, 82, C(214, 182, 110)); for (int i = 0; i < 40; i++) rect((i * 37) % 400, 166 + (i * 13) % 70, 10, 2, C(236, 206, 130)); }   // hay floor
    else { rect(0, 158, TOP_W, 82, C(118, 188, 98)); rect(0, 190, TOP_W, 26, C(196, 160, 112)); for (int i = 0; i < 30; i++) rect((i * 53) % 400, 160 + (i * 17) % 28, 2, 4, C(90, 160, 80)); }   // grass & dirt path
    switch (room) {
    case R_AVSQUARE:
        rect(20, 50, 110, 70, C(240, 220, 180)); rect(14, 42, 122, 12, C(160, 60, 50)); text("GENERAL STORE", 75, 60, 0.36f, C(120, 50, 40), 1);
        rect(270, 50, 110, 70, C(150, 100, 70)); rect(264, 42, 122, 12, C(90, 60, 40)); text("CIDER PUB", 325, 60, 0.38f, C(255, 230, 160), 1);
        rect(160, 70, 80, 50, C(250, 250, 240)); for (int i = 0; i < 5; i++) rect(162 + i * 16.f, 64, 12, 8, C(i % 2 ? RGB{230, 70, 70} : RGB{255, 255, 255}));
        text(G.day % 7 == 5 ? "FAIR TODAY!" : "ERRANDS", 200, 86, 0.36f, C(120, 60, 60), 1);
        for (int i = 0; i < 10; i++) circle(14 + i * 42.f, 24 + std::sin(i * 1.1f) * 4, 3, C(i % 2 ? RGB{255, 120, 120} : RGB{255, 230, 120}));
        break;
    case R_AVFARM:
        rect(250, 54, 120, 66, C(250, 240, 225)); for (int i = 0; i < 6; i++) rect(244 + i * 22.f, 40 - i % 2 * 0, 22, 14, C(200, 80, 70)); text("SUNNY ACRES", 310, 70, 0.36f, C(150, 70, 60), 1);
        for (int r = 0; r < 2; r++) for (int i = 0; i < 4; i++) { const FPlot& p = F.plot[r * 4 + i]; const float x = 20 + i * 52.f, y = 98 + r * 22.f;
            rect(x, y, 46, 16, C(p.watered ? RGB{110, 70, 45} : RGB{150, 105, 70}));
            if (p.crop) { const float g = std::min(1.f, p.days / (float)CROP_DAYS[p.crop]); circle(x + 23, y + 6 - g * 4, 3 + g * (p.giant ? 9 : 5), C(p.days >= CROP_DAYS[p.crop] ? (p.crop == CR_PUMPKIN ? RGB{240, 140, 40} : p.crop == CR_MELON ? RGB{70, 160, 70} : RGB{230, 60, 60}) : RGB{90, 170, 70})); } }
        if (F.coop) { rect(222, 92, 26, 22, C(200, 70, 60)); rect(219, 86, 32, 8, C(120, 50, 40)); for (int i = 0; i < std::min<int>(F.chickens, 5); i++) circle(226 + i * 6.f, 118 + std::sin(t * 6 + i) * 1, 2.5f, C(i < F.golden ? RGB{240, 200, 60} : RGB{250, 250, 250})); }
        if (F.foxDays) { circle(380, 130, 6, C(230, 120, 40)); circle(386, 126, 3, C(230, 120, 40)); }
        if (F.rabbit >= 30) circle(10 + std::fmod(t * 20, 380.f), 140, 4, C(240, 240, 240));
        break;
    case R_AVBARN:
        rect(80, 36, 240, 100, C(190, 50, 45)); for (int i = 0; i < 12; i++) rect(80 + i * 20.f, 36, 2, 100, C(160, 40, 38));
        rect(70, 26, 260, 14, C(240, 240, 235)); rect(175, 70, 50, 66, C(150, 35, 30)); rect(178, 73, 44, 60, C(240, 240, 235, 60));
        if (!F.barn) text("(BUILD ME AT THE STORE)", 200, 52, 0.4f, C(255, 255, 255), 1);
        if (F.cow) { rect(20, 112, 30, 16, C(250, 250, 250)); circle(30, 118, 4, C(30, 30, 30)); circle(52, 114, 6, C(250, 250, 250)); }
        if (F.llama) { rect(340, 110, 22, 16, C(240, 230, 210)); rect(358, 90, 6, 24, C(240, 230, 210)); circle(362, 88, 5, C(240, 230, 210)); }
        for (int i = 0; i < F.goats; i++) circle(110 + i * 14.f, 132 + std::sin(t * 5 + i) * 2, 4, C(220, 200, 170));
        for (int i = 0; i < F.sheep; i++) circle(250 + i * 14.f, 132, 5, C(250, 250, 250));
        for (int i = 0; i < F.pigs; i++) circle(160 + i * 16.f, 140, 5, C(250, 180, 190));
        break;
    }
    if (night) rect(0, 0, TOP_W, TOP_H, C(10, 10, 40, 80));
}

void farmSaveTo(const std::string& path) { FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fwrite(&F, sizeof F, 1, f); fclose(f); }
void farmLoadFrom(const std::string& path) {
    F = Farm{}; FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    Farm t; const size_t n = fread(&t, 1, sizeof t, f); fclose(f);
    if (n == sizeof t && t.magic == 0x4D524146u) F = t;
    if (F.pots > 4) F.pots = 4; if (F.errand > NER) F.errand = 0; for (auto& p : F.plot) if (p.crop >= NCROP) p = FPlot{}; for (auto& p : F.pot) if (p.crop >= NCROP) p = FPlot{};
    if (F.chickens > 8) F.chickens = 8; if (F.golden > F.chickens) F.golden = F.chickens;
    if (F.farmhouse > 2 || F.slot > 1) F.farmhouse = 0;
}
