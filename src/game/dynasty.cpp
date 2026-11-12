#include "settings2.hpp"
#include "townpulse.hpp"
#include "dynasty.hpp"
#include "deeplife.hpp"
#include "growup.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "tween.hpp"
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack

namespace {
const char* CAT[8] = {"Family", "Fortune", "Love", "Knowledge", "Creativity", "Popularity", "Athletic", "Career"};
struct DY {
    uint32_t magic = 0x44594E31;
    uint8_t on = 0, gen = 1, evFreq = 2, rotate = 0, rotDays = 7, rotLeft = 7, done = 0, pad = 0;
    int32_t total = 0; int16_t genScore[10] = {}; int16_t cat[8] = {};
    char log[8][48] = {}; uint8_t nlog = 0;
};
DY Y;
std::string first(int s) { const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
std::string last(int s) { const std::string& n = G.sims[s].name; const size_t p = n.rfind(' '); return p == std::string::npos ? "" : n.substr(p + 1); }
void logEv(const std::string& s) { if (Y.nlog >= 8) { for (int i = 0; i < 7; i++) memcpy(Y.log[i], Y.log[i + 1], 48); Y.nlog = 7; } snprintf(Y.log[Y.nlog++], 48, "Day %d: %s", (int)G.day, s.c_str()); }

int scoreCat(int c) {   // 0..10 for the current generation
    const Sim& me = MC(); int v = 0;
    switch (c) {
    case 0: { for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && G.sims[i].household && isFamily(G.mc, i)) v += 2; break; }
    case 1: v = (int)(G.money / 5000); break;
    case 2: for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.status == RS_MARRIED) v = 6 + std::max(0, r.ro / 25); break;
    case 3: v = (int)(me.skills[SK_LOGIC] + (me.gradePts >= 90 ? 2 : 0)); break;
    case 4: v = (int)((me.skills[SK_CREATIVITY] + me.skills[SK_STYLE]) / 2); break;
    case 5: for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.fr >= 50) v++; break;
    case 6: v = (int)me.skills[SK_FITNESS]; break;
    case 7: v = me.career != CR_NONE ? 1 + me.careerLevel : 0; break;
    }
    return std::max(0, std::min(10, v));
}
int liveScore() { int s = 0; for (int c = 0; c < 8; c++) s += scoreCat(c) * 10; return s; }
const char* rank(int t) { return t >= 6000 ? "LEGENDARY DYNASTY" : t >= 4000 ? "Iconic Family" : t >= 2500 ? "Well-Known Name" : t >= 1200 ? "Rising Family" : "Humble Beginnings"; }

void addRelative() {
    if (G.sims.size() >= 118) return;
    static const char* WHO[5] = {"cousin", "aunt", "uncle", "grandma", "long-lost sister"};
    static const Stage ST[5] = {ST_YA, ST_ADULT, ST_ADULT, ST_ELDER, ST_YA};
    const int k = irand(5);
    Sim r = randomSim(ST[k], k == 2 ? 1 : k == 0 ? -1 : 0); r.household = true;
    const std::string fn = r.name.substr(0, r.name.find(' ')); r.name = fn + " " + last(G.mc); r.bio = std::string("your ") + WHO[k] + ", who just showed up";
    G.sims.push_back(r); const int gi = (int)G.sims.size() - 1;
    Rel& rr = rel(G.mc, gi); rr.met = true; rr.fr = 20; rr.status = RS_FAMILY;
    Actor a; a.kind = 0; a.id = (uint16_t)gi; a.x = 120.f + irand(160); a.tx = a.x; a.y = 212; G.rooms[G.house][R_LIVING].actors.push_back(a);
    toast("SURPRISE! Your " + std::string(WHO[k]) + " " + fn + " showed up with three suitcases. \"Just for a little while...\"");
    logEv(std::string("Your ") + WHO[k] + " " + fn + " moved in");
}

void randomEvent() {
    Sim& me = MC();
    const int roll = irand(100);
    if (roll < 4) { const int w = 5000 + irand(45000); G.money += w; toast("YOU WON THE LOTTERY! +$" + std::to_string(w) + "!!!"); guPhoto("Won the lottery!"); logEv("Won $" + std::to_string(w) + " in the lottery"); sfxYay(); return; }
    if (roll < 16) { const int w = 20 + irand(180); G.money += w; toast("Scratch-off winner! +$" + std::to_string(w)); logEv("Scratch-off: $" + std::to_string(w)); return; }
    if (roll < 23 && gsOn(GS_FIRES)) {   // house fire
        int lost = 0;
        for (int r = 0; r < R_COUNT && lost < 3; r++) { if (!roomIsHome(r)) continue; auto& it = G.rooms[G.house][r].items; if (it.size() > 2 && irand(3) == 0) { it.erase(it.begin() + irand((int)it.size())); lost++; } }
        const int cost = 300 + irand(700); G.money = G.money > cost ? G.money - cost : 0;
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) addMoodlet(G.sims[i], "House Fire!", EM_SAD, 2, 1440);
        toast("KITCHEN FIRE! Everyone got out safe, but " + std::to_string(lost) + " pieces of furniture are gone and repairs cost $" + std::to_string(cost) + ".");
        logEv("House fire - lost " + std::to_string(lost) + " items"); return;
    }
    if (roll < 33) { addRelative(); return; }
    if (roll < 43) { const int c = 150 + irand(400); G.money = G.money > c ? G.money - c : 0; toast("Your car broke down on the highway. Tow + repairs: $" + std::to_string(c) + "."); logEv("Car trouble: -$" + std::to_string(c)); return; }
    if (roll < 52) { const int c = 500 + irand(4500); G.money += c; toast("A distant relative left you $" + std::to_string(c) + " in their will. Who even WAS that?"); logEv("Surprise inheritance $" + std::to_string(c)); return; }
    if (roll < 60) { toast("A stray kitten followed you home. Everyone is in love."); for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) addMoodlet(G.sims[i], "Stray Kitten", EM_HAPPY, 1, 1440); logEv("A stray kitten showed up"); return; }
    if (roll < 68) { toast("Pipe burst in the bathroom! Plumber: $250."); G.money = G.money > 250 ? G.money - 250 : 0; logEv("Burst pipe: -$250"); return; }
    if (roll < 76) { toast("You went viral for a dance in the grocery store! +2,000 followers."); G.followers += 2000; logEv("Went viral"); return; }
    if (roll < 84) { toast("Power outage! Candles, board games and ghost stories tonight."); for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) G.sims[i].needs[N_FUN] = std::min(100.f, G.sims[i].needs[N_FUN] + 15); logEv("Power outage night"); return; }
    if (roll < 92) { toast("Jury summons in the mail. Ugh."); logEv("Jury summons"); return; }
    { toast("You found $50 in an old jacket pocket!"); G.money += 50; addMoodlet(me, "Found Money", EM_HAPPY, 1, 360); logEv("Found $50"); }
}

void rotateNow() {
    DlMenu m = dlApp(4); int pickId = -1;
    for (auto& e : m) if (e.second >= 11610 && e.second < 11614) { pickId = e.second; break; }   // longest-waiting parked household
    if (pickId < 0) { std::vector<int> ids; for (auto& e : m) if (e.second >= 12200 && e.second < 12400) ids.push_back(e.second); if (!ids.empty()) pickId = ids[irand((int)ids.size())]; }
    if (pickId < 0) { toast("Rotation: no other household to switch to yet."); return; }
    dlOption(pickId);
    toast("Rotation! Now playing the " + last(G.mc) + " household.");
    logEv("Rotated to the " + last(G.mc) + " family");
}
}

LxMenu dyApp() {
    LxMenu m; char b[80];
    if (!Y.on) { m.push_back({"Start a 10-Generation Dynasty", 20001}); }
    else {
        snprintf(b, sizeof b, "Generation %d / 10  -  Total %d pts", Y.gen, (int)Y.total); m.push_back({b, 20999});
        snprintf(b, sizeof b, "Rank: %s", rank(Y.total + liveScore())); m.push_back({b, 20999});
        m.push_back({"This Generation's Score (" + std::to_string(liveScore()) + ")", 20010});
        m.push_back({"Past Generations", 20011});
        if (!Y.done) m.push_back({"Pass the Torch to an Heir", 20020});
        m.push_back({"End Dynasty", 20002});
    }
    static const char* F[4] = {"Off", "Rare", "Normal", "Chaotic"};
    m.push_back({std::string("Random Life Events: ") + F[Y.evFreq & 3] + " (change)", 20030});
    m.push_back({"Recent Life Events", 20031});
    m.push_back({"Town Pulse: What Everyone Else Is Up To", 20032});
    m.push_back({Y.rotate ? "Rotational Play: ON every " + std::to_string(Y.rotDays) + " days (" + std::to_string(Y.rotLeft) + " left)" : std::string("Rotational Play: OFF (turn on)"), 20040});
    if (Y.rotate) { m.push_back({"Change Rotation Length", 20041}); m.push_back({"Rotate Now", 20042}); }
    return m;
}

LxMenu dyOption(int id) {
    LxMenu m;
    switch (id) {
    case 20001: Y = DY{}; Y.on = 1; Y.gen = 1; toast("Your dynasty begins! Score points in 8 categories, then pass the torch. 10 generations to go."); return dyApp();
    case 20002: Y.on = 0; toast("Dynasty ended. Final total: " + std::to_string(Y.total) + " - " + rank(Y.total)); return {};
    case 20010: for (int c = 0; c < 8; c++) m.push_back({std::string(CAT[c]) + ": " + std::to_string(scoreCat(c)) + "/10", 20999});
                m.push_back({"(Banked when you pass the torch)", 20999}); return m;
    case 20011: for (int g = 0; g < Y.gen - 1 && g < 10; g++) m.push_back({"Gen " + std::to_string(g + 1) + ": " + std::to_string(Y.genScore[g]) + " pts", 20999});
                if (m.empty()) m.push_back({"(Still on generation 1)", 20999}); return m;
    case 20020: { for (int i = 0; i < (int)G.sims.size() && i < 250; i++) { const Sim& s = G.sims[i]; if (i != G.mc && s.household && s.stage >= ST_TEEN && s.stage < MC().stage && isFamily(G.mc, i)) m.push_back({"Heir: " + s.name, 20100 + i}); }
                  if (m.empty()) m.push_back({"(You need a teen or older kid in the house)", 20999}); return m; }
    case 20030: Y.evFreq = (uint8_t)((Y.evFreq + 1) & 3); return dyApp();
    case 20031: for (int i = Y.nlog - 1; i >= 0; i--) m.push_back({Y.log[i], 20999}); if (m.empty()) m.push_back({"(Nothing wild yet)", 20999}); return m;
    case 20032: return tpLog();
    case 20040: Y.rotate ^= 1; Y.rotLeft = Y.rotDays; toast(Y.rotate ? "Rotational play ON: the game switches households for you." : "Rotational play OFF."); return dyApp();
    case 20041: Y.rotDays = Y.rotDays == 7 ? 3 : Y.rotDays == 3 ? 14 : 7; Y.rotLeft = Y.rotDays; return dyApp();
    case 20042: rotateNow(); Y.rotLeft = Y.rotDays; return {};
    }
    if (id >= 20100 && id < 20350 && id - 20100 < (int)G.sims.size()) {
        const int h = id - 20100; const int pts = liveScore() + 50;
        if (Y.gen <= 10) Y.genScore[Y.gen - 1] = (int16_t)pts;
        Y.total += pts; G.money += 1000;
        { const int w = guWillHeir();
          if (w == h) { G.money += 2000; toast("The will is honored: " + first(h) + " inherits the family estate (+$2,000)."); }
          else if (w >= 0 && w < (int)G.sims.size()) { changeRel(h, w, -15, 0); toast("Drama! The will named " + first(w) + ", but " + first(h) + " is taking over. They are NOT speaking."); } }
        toast("Generation " + std::to_string(Y.gen) + " banked " + std::to_string(pts) + " points! " + first(h) + " carries on the " + last(G.mc) + " name.");
        guPhoto("Gen " + std::to_string(Y.gen) + " passes the torch to " + first(h));
        G.mc = h; for (int r = 0; r < R_COUNT; r++) for (const Actor& a : G.rooms[G.house][r].actors) if (a.kind == 0 && a.id == h) G.room = r;
        if (++Y.gen > 10) { Y.done = 1; toast("10 GENERATIONS COMPLETE! Final score " + std::to_string(Y.total) + " - " + rank(Y.total) + "!"); }
        sfxYay(); return {};
    }
    return {};
}

void dyDaily() {
    static const int CH[4] = {0, 3, 8, 18};
    if (gsOn(GS_EVENTS) && irand(100) < CH[Y.evFreq & 3]) randomEvent();
    if (Y.rotate && Y.rotLeft > 0 && --Y.rotLeft == 0) { rotateNow(); Y.rotLeft = Y.rotDays; }
}

void dySaveTo(const std::string& p) { FILE* f = fopen(p.c_str(), "wb"); if (!f) return; fwrite(&Y, sizeof Y, 1, f); fclose(f); }
void dyLoadFrom(const std::string& p) { Y = DY{}; FILE* f = fopen(p.c_str(), "rb"); if (!f) return; DY d; if (fread(&d, 1, sizeof d, f) >= 8 && d.magic == Y.magic) Y = d; fclose(f); }
int dyGeneration() { return Y.on ? Y.gen : 0; }
