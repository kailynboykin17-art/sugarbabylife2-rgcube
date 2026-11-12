// World Settings, story progression extras, rival romances, and the Grand Starlight hotel business.
#include "worldsim.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "lifestory.hpp"
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
using namespace col;
void toast(const std::string& m);
int lifeAddTownie(int stage, int gender);
void lifeMarry(int a, int b);

namespace {
bool ok(int s) { return s >= 0 && s < (int)G.sims.size(); }
const float SPEEDS[5] = {0.5f, 1.f, 1.5f, 2.f, 3.f};
const char* SPEED_N[5] = {"Slow (0.5x)", "Normal", "Fast (1.5x)", "Faster (2x)", "Speedy (3x)"};
const int POPS[5] = {25, 35, 45, 60, 80};
const char* LIFE_N[3] = {"Short", "Normal", "Long"};
const int LIFE_DAYS[3] = {10, 22, 40};
bool alive(int s) { return ok(s) && !simDead(s) && !simGone(s); }
int townCount() { int n = 0; for (size_t i = 0; i < G.sims.size(); i++) if (!G.sims[i].household && (int)i != G.mc && alive((int)i)) n++; return n; }
}

float worldSpeedMul() { return SPEEDS[G.speedIdx % 5]; }
bool simGone(int s) { return s >= 0 && s < (int)G.movedAway.size() && G.movedAway[s]; }
int lifespanDays() { return LIFE_DAYS[G.lifespan % 3]; }

// ---------------------------------------------------------------- rival romances
static void rivalDaily() {
    Sim& me = MC();
    if (!me.isAdult() && me.stage != ST_TEEN) return;
    if (ok(G.rival) && (!alive(G.rival) || !alive(G.rivalFor))) { G.rival = -1; }
    if (!ok(G.rival)) {
        // who is MC sweet on?
        int t = -1, best = 24;
        for (const Rel& r : G.rels) { if (r.a != G.mc && r.b != G.mc) continue; int o = r.a == G.mc ? r.b : r.a;
            if (!alive(o) || G.sims[o].household || r.status >= RS_ENGAGED) continue; if ((G.sims[o].stage >= ST_YA) != me.isAdult()) continue;
            if (r.ro > best) { best = r.ro; t = o; } }
        if (t < 0 || irand(100) >= 18) return;
        std::vector<int> pool;
        for (size_t i = 0; i < G.sims.size(); i++) { int c = (int)i; const Sim& s = G.sims[i];
            if (c == t || c == G.mc || s.household || !alive(c) || (s.stage >= ST_YA) != (G.sims[t].stage >= ST_YA) || s.stage < ST_TEEN) continue;
            if (partnerOf(c) >= 0) continue;
            bool likes = s.interest == 2 || (s.interest == 0) == G.sims[t].look.guy; if (!likes) continue;
            pool.push_back(c); }
        if (pool.empty()) return;
        G.rival = (int16_t)pool[irand((int)pool.size())]; G.rivalFor = (int16_t)t; G.rivalScore = 10;
        rel(G.rival, t).met = true;
        toast("RIVAL ALERT! " + G.sims[G.rival].name + " is falling for " + G.sims[t].name + " too. Win their heart first!");
        addMoodlet(me, "Jealous", EM_TENSE, 2, 720);
        return;
    }
    const int r = G.rival, t = G.rivalFor;
    const Rel* mine = findRel(G.mc, t); int myRo = mine ? mine->ro : 0;
    if (mine && mine->status >= RS_ENGAGED) {
        toast(G.sims[r].name + " gives up on " + G.sims[t].name + ". You won! (They brought you a congrats cupcake. Awkward.)");
        addMoodlet(me, "Won the Rivalry", EM_CONFIDENT, 2, 720); G.rival = -1; return;
    }
    G.rivalScore = (uint8_t)std::min(100, G.rivalScore + 5 + irand(7));
    changeRel(r, t, 3, 6);
    if (G.rivalScore >= 100) {
        if (myRo >= 75) { toast(G.sims[t].name + " turned " + G.sims[r].name + " down - they only have eyes for you!"); addMoodlet(me, "Chosen One", EM_FLIRTY, 2, 720); G.rival = -1; return; }
        if (mine && mine->status == RS_DATING) { rel(G.mc, t).status = RS_NONE; changeRel(G.mc, t, -20, -40); }
        rel(r, t).status = RS_ENGAGED; lifeMarry(r, t);
        toast("Wedding bells... " + G.sims[t].name + " married " + G.sims[r].name + "! Your rival won this time.");
        addMoodlet(me, "Lost to a Rival", EM_SAD, 3, 2880);
        addMemoryOnce(MEM_LOVE, G.mc, t, G.sims[t].name + " married " + G.sims[r].name + " instead of me.");
        G.rival = -1; return;
    }
    if (G.rivalScore >= 60 && irand(3) == 0) {
        char b[160]; snprintf(b, sizeof b, "Rival update: %s is %d%% of the way to winning %s (your romance: %d). Make your move!", G.sims[r].name.c_str(), G.rivalScore, G.sims[t].name.c_str(), myRo);
        toast(b);
    }
}

// ---------------------------------------------------------------- story progression extras
void worldSimDaily() {
    if (G.movedAway.size() < G.sims.size()) G.movedAway.resize(G.sims.size(), 0);
    if (G.storyProg) {
        for (size_t i = 0; i < G.sims.size(); i++) {
            Sim& s = G.sims[i]; int k = (int)i;
            if (s.household || k == G.mc || !alive(k)) continue;
            if (s.stage >= ST_YA && s.career == CR_NONE && irand(100) < 8) { s.career = (uint8_t)(1 + irand(CR_COUNT - 2)); s.careerLevel = 0; }
            else if (s.career != CR_NONE && s.careerLevel < 4 && irand(100) < 4) s.careerLevel++;
        }
        const int target = POPS[G.popIdx % 5], have = townCount();
        if (have > target + 2) {   // someone moves away (never close friends, family or partners)
            std::vector<int> pool;
            for (size_t i = 0; i < G.sims.size(); i++) { int k = (int)i; if (G.sims[i].household || k == G.mc || !alive(k) || k == G.rival || k == G.rivalFor) continue;
                if (G.sims[i].bio.rfind("[Cast]", 0) == 0) continue; const Rel* r = findRel(G.mc, k); if (r && (r->fr >= 30 || r->ro >= 20 || r->status != RS_NONE)) continue;
                if (partnerOf(k) >= 0) continue; pool.push_back(k); }
            if (!pool.empty()) { int k = pool[irand((int)pool.size())]; G.movedAway[k] = 1; const Rel* r = findRel(G.mc, k); if (r && r->met) toast(G.sims[k].name + " moved away to a new city. Bye!"); }
        } else if (have < target - 2) {
            int a = lifeAddTownie(irand(3) ? ST_YA : ST_ADULT, -1);
            if (a >= 0) { if (G.movedAway.size() < G.sims.size()) G.movedAway.resize(G.sims.size(), 0);
                if (irand(3) == 0) { int b = lifeAddTownie(G.sims[a].stage, G.sims[a].look.guy ? 0 : 1); if (b >= 0) { if (G.movedAway.size() < G.sims.size()) G.movedAway.resize(G.sims.size(), 0); rel(a, b).status = RS_ENGAGED; lifeMarry(a, b); } }
                toast("New neighbors! " + G.sims[a].name + " just moved to town."); }
        }
    }
    rivalDaily();
    hotelDaily();
}

// ---------------------------------------------------------------- World Settings screen
namespace {
Btn rowW(int k) { return Btn{8, 30 + k * 34.f, 304, 30}; }
const Btn W_BACK{4, 208, 90, 28};
}
void worldOpen() {}
bool worldUpdate(const Input& in) {
    if (in.down & KEY_B) return false;
    if (!in.touch) return true;
    const touchPosition& tp = in.tp;
    if (W_BACK.in(tp)) { sfxTap(); return false; }
    if (rowW(0).in(tp)) { G.storyProg = !G.storyProg; sfxTap(); }
    if (rowW(1).in(tp)) { G.aging = !G.aging; sfxTap(); }
    if (rowW(2).in(tp)) { G.speedIdx = (uint8_t)((G.speedIdx + 1) % 5); sfxTap(); }
    if (rowW(3).in(tp)) { G.popIdx = (uint8_t)((G.popIdx + 1) % 5); sfxTap(); }
    if (rowW(4).in(tp)) { G.lifespan = (uint8_t)((G.lifespan + 1) % 3); sfxTap(); }
    return true;
}
void worldDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(220, 240, 255), C(255, 235, 245));
    textOutline("WORLD SETTINGS", 200, 20, 0.8f, C(255, 255, 255), C(255, 70, 150), 1);
    char b[96];
    snprintf(b, sizeof b, "Townies in town: %d  (target %d)", townCount(), POPS[G.popIdx % 5]); text(b, 200, 70, 0.45f, C(UI_INK), 1);
    int gone = 0; for (uint8_t m : G.movedAway) gone += m; snprintf(b, sizeof b, "Moved away so far: %d", gone); text(b, 200, 92, 0.4f, C(120, 100, 130), 1);
    if (ok(G.rival) && ok(G.rivalFor)) { snprintf(b, sizeof b, "Rival: %s wants %s (%d%%)", G.sims[G.rival].name.c_str(), G.sims[G.rivalFor].name.c_str(), G.rivalScore); text(b, 200, 120, 0.4f, C(220, 60, 100), 1); }
    text("Story progression lets townies date, marry, have kids,", 200, 160, 0.36f, C(120, 100, 130), 1);
    text("get jobs, move away and move in without you.", 200, 174, 0.36f, C(120, 100, 130), 1);
    (void)t;
}
void worldDrawBottom(float t) {
    rect(0, 0, BOT_W, BOT_H, C(250, 246, 255));
    text("Tap a setting to change it", 160, 8, 0.4f, C(UI_INK), 1);
    char b[80];
    snprintf(b, sizeof b, "Story Progression: %s", G.storyProg ? "ON" : "OFF"); button(rowW(0), b, G.storyProg, 0, 0.4f);
    snprintf(b, sizeof b, "Aging: %s", G.aging ? "ON" : "OFF"); button(rowW(1), b, G.aging, 0, 0.4f);
    snprintf(b, sizeof b, "Game Speed: %s", SPEED_N[G.speedIdx % 5]); button(rowW(2), b, false, 2, 0.4f);
    snprintf(b, sizeof b, "Town Population: %d townies", POPS[G.popIdx % 5]); button(rowW(3), b, false, 2, 0.4f);
    snprintf(b, sizeof b, "Lifespan: %s (elders: %d days)", LIFE_N[G.lifespan % 3], LIFE_DAYS[G.lifespan % 3]); button(rowW(4), b, false, 2, 0.4f);
    button(W_BACK, "Back", false, 1, 0.42f);
    (void)t;
}

// ================================================================ the hotel business
namespace {
const char* THEME_N[4] = {"Classic Luxe", "Y2K Pink Dream", "Beach Bungalow", "Gothic Glam"};
struct Upg { const char* name; int cost; int stars; };
const Upg UPG[6] = {{"Rooftop Pool Makeover", 2500, 1}, {"Day Spa", 3000, 1}, {"Penthouse Suites", 5000, 2}, {"Gourmet Kitchen", 2000, 1}, {"Ballroom", 4000, 1}, {"Glam Lobby", 1500, 1}};
const char* STAFF_N[4] = {"Front Desk", "Housekeeping", "Kitchen", "Spa & Pool"};
int tabH = 0;   // 0 Stay, 1 Manage
int nights = 2;
int stars() { int s = 1; for (int k = 0; k < 6; k++) if (G.hotelUpg & (1 << k)) s += UPG[k].stars; int staffOk = 0; for (int k = 0; k < 4; k++) staffOk += G.hotelStaff[k] > 0; s += staffOk / 2; return std::min(5, s); }
int fairRate() { return 60 + stars() * 45; }
Btn hB(int k) { return Btn{6 + (k % 2) * 156.f, 30 + (k / 2) * 34.f, 152, 30}; }
const Btn H_TAB0{6, 2, 150, 24}, H_TAB1{164, 2, 150, 24};
}
bool hotelStaying() { return G.hotelStayEnd != 0xFFFF && G.day < G.hotelStayEnd; }
void hotelDaily() {
    if (G.hotelStayEnd != 0xFFFF && G.day >= G.hotelStayEnd) { G.hotelStayEnd = 0xFFFF; toast("Checked out of the Grand Starlight. Your bed at home feels SO small now."); }
    if (!G.hotelOwned) return;
    // guests come based on stars, price and staff; staff cost wages
    int rate = std::max(20, (int)G.hotelRate), fair = fairRate();
    float demand = 0.55f + stars() * 0.08f - (rate - fair) / (float)(fair * 2);
    demand = std::max(0.05f, std::min(0.98f, demand + (irand(21) - 10) / 100.f));
    const int rooms = 20 + ((G.hotelUpg & 4) ? 6 : 0);
    int guests = (int)(rooms * demand);
    int staffTotal = 0; for (int k = 0; k < 4; k++) staffTotal += G.hotelStaff[k];
    if (G.hotelStaff[1] == 0) guests = guests * 2 / 3;   // no housekeeping, rooms aren't ready
    int income = guests * rate, wages = staffTotal * 70, upkeep = 120;
    int net = income - wages - upkeep;
    G.money = std::max<int32_t>(0, G.money + net);
    G.hotelGuests = (uint8_t)guests; G.hotelLastNet = net;
    int score = stars() * 18 + (G.hotelStaff[0] ? 6 : -10) + (G.hotelStaff[2] ? 4 : 0) - std::max(0, (rate - fair) / 10);
    G.hotelReview = (uint8_t)std::max(1, std::min(5, score / 20));
    if (irand(4) == 0) { static const char* R[] = {"\"The pillows were clouds.\"", "\"Staff remembered my name!\"", "\"Pricey but worth it.\"", "\"The lobby is SO pretty.\"", "\"Room wasn't ready on time...\"", "\"Pool was a 10/10.\""}; toast(std::string("Hotel review: ") + R[irand(6)]); }
}
void hotelAppOpen() { tabH = 0; }
void hotelAppUpdate(const Input& in) {
    if (!in.touch) return;
    const touchPosition& tp = in.tp;
    if (H_TAB0.in(tp)) { tabH = 0; sfxTap(); return; }
    if (H_TAB1.in(tp)) { tabH = 1; sfxTap(); return; }
    Sim& me = MC();
    const int rate = G.hotelOwned ? 0 : std::max(20, (int)G.hotelRate);
    if (tabH == 0) {
        if (hB(0).in(tp)) { nights = nights % 5 + 1; sfxTap(); }
        if (hB(1).in(tp)) {
            if (hotelStaying()) { toast("You're already checked in!"); return; }
            if (me.stage < ST_TEEN) { toast("You need a grown-up to book a room."); sfxNo(); return; }
            int cost = rate * nights; if (G.money < cost) { toast("Not enough money!"); sfxNo(); return; }
            G.money -= cost; G.hotelStayEnd = (uint16_t)(G.day + nights);
            addMoodlet(me, "Hotel Life", EM_HAPPY, 2, (uint16_t)std::min(1440 * nights, 7200));
            toast(G.hotelOwned ? "Booked your own penthouse. Perks of being the owner!" : "Booked! Head to the Grand Starlight front desk any time to sleep in your suite."); sfxYay();
        }
        static const struct { const char* n; int c; float food; } RS[4] = {{"Breakfast in Bed", 25, 60}, {"Strawberries & Cheesecake", 30, 40}, {"Midnight Burger & Fries", 22, 70}, {"Champagne-Free Bubbly Mocktail", 15, 15}};
        for (int k = 0; k < 4; k++) if (hB(2 + k).in(tp)) {
            if (!hotelStaying()) { toast("Room service is for hotel guests - book a stay first!"); sfxNo(); return; }
            if (G.money < RS[k].c) { toast("Not enough money!"); sfxNo(); return; }
            G.money -= RS[k].c; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + RS[k].food); addMoodlet(me, "Room Service Royalty", EM_CONFIDENT, 1, 240);
            toast(std::string(RS[k].n) + " delivered on a silver tray. *chef's kiss*"); sfxYay();
        }
        return;
    }
    // manage
    if (!G.hotelOwned) {
        if (hB(0).in(tp)) { if (!me.isAdult()) { toast("Adults only!"); return; } if (G.money < 12000) { toast("The Grand Starlight costs $12,000."); sfxNo(); return; }
            G.money -= 12000; G.hotelOwned = true; G.hotelRate = (uint16_t)fairRate(); toast("You OWN the Grand Starlight Hotel now! Hire staff and set your rates."); sfxYay(); addMemoryOnce(MEM_CAREER, G.mc, -1, "Bought the Grand Starlight Hotel!"); }
        return;
    }
    if (hB(0).in(tp)) { G.hotelRate = (uint16_t)std::max(20, (int)G.hotelRate - 10); sfxTap(); }
    if (hB(1).in(tp)) { G.hotelRate = (uint16_t)std::min(999, (int)G.hotelRate + 10); sfxTap(); }
    for (int k = 0; k < 4; k++) if (Btn{6 + (k % 2) * 156.f, 64 + (k / 2) * 28.f, 152, 26}.in(tp)) { G.hotelStaff[k] = (uint8_t)((G.hotelStaff[k] + 1) % 4); sfxTap(); }
    if (Btn{6, 122, 152, 26}.in(tp)) { G.hotelTheme = (uint8_t)((G.hotelTheme + 1) % 4); toast(std::string("Redecorated: ") + THEME_N[G.hotelTheme] + "!"); sfxTap(); }
    for (int k = 0; k < 6; k++) if (Btn{6 + (k % 2) * 156.f, 152 + (k / 2) * 18.f, 152, 17}.in(tp)) {
        if (G.hotelUpg & (1 << k)) { toast("Already built!"); return; }
        if (G.money < UPG[k].cost) { toast("Not enough money!"); sfxNo(); return; }
        G.money -= UPG[k].cost; G.hotelUpg |= (uint8_t)(1 << k); toast(std::string(UPG[k].name) + " is open! Stars: " + std::to_string(stars())); sfxYay();
    }
}
void hotelAppDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(255, 236, 214), C(250, 214, 230));
    textOutline("GRAND STARLIGHT HOTEL", 200, 14, 0.7f, C(255, 255, 255), C(200, 120, 60), 1);
    char b[128];
    for (int k = 0; k < 5; k++) { float x = 152 + k * 24.f; heart(x, 50, 8, k < stars() ? C(255, 200, 60) : C(220, 210, 200)); }
    if (hotelStaying()) { snprintf(b, sizeof b, "You're checked in until day %d. Room service is on!", G.hotelStayEnd + 1); text(b, 200, 70, 0.4f, C(UI_INK), 1); }
    if (G.hotelOwned) {
        snprintf(b, sizeof b, "OWNER  -  theme: %s", THEME_N[G.hotelTheme % 4]); text(b, 200, 92, 0.42f, C(200, 80, 120), 1);
        snprintf(b, sizeof b, "Nightly rate $%d (fair price ~$%d)", G.hotelRate, fairRate()); text(b, 200, 112, 0.4f, C(UI_INK), 1);
        snprintf(b, sizeof b, "Last night: %d guests, %s$%d", G.hotelGuests, G.hotelLastNet >= 0 ? "+" : "-", std::abs((int)G.hotelLastNet)); text(b, 200, 130, 0.4f, C(UI_INK), 1);
        snprintf(b, sizeof b, "Guest reviews: %d / 5", G.hotelReview); text(b, 200, 148, 0.4f, C(UI_INK), 1);
        text("Staff wages $70/day each. No housekeeping = fewer guests.", 200, 176, 0.32f, C(120, 100, 130), 1);
        text("Price too high = empty rooms. Upgrades raise your stars.", 200, 190, 0.32f, C(120, 100, 130), 1);
    } else {
        snprintf(b, sizeof b, "Rooms $%d / night. Stay 1 to 5 nights.", std::max(20, (int)G.hotelRate)); text(b, 200, 100, 0.42f, C(UI_INK), 1);
        text("Want to run it yourself? Manage tab: buy the hotel ($12,000).", 200, 130, 0.36f, C(120, 100, 130), 1);
    }
    (void)t;
}
void hotelAppDrawBottom(float t) {
    button(H_TAB0, "Stay & Room Service", tabH == 0, 0, 0.36f); button(H_TAB1, G.hotelOwned ? "Manage Hotel" : "Own This Hotel", tabH == 1, 0, 0.36f);
    char b[80];
    if (tabH == 0) {
        const int rate = G.hotelOwned ? 0 : std::max(20, (int)G.hotelRate);
        snprintf(b, sizeof b, "Nights: %d", nights); button(hB(0), b, false, 0, 0.4f);
        snprintf(b, sizeof b, hotelStaying() ? "Checked In" : "Book ($%d)", rate * nights); button(hB(1), b, hotelStaying(), 2, 0.4f);
        static const char* RN[4] = {"Breakfast in Bed $25", "Strawberries & Cake $30", "Midnight Burger $22", "Bubbly Mocktail $15"};
        for (int k = 0; k < 4; k++) button(hB(2 + k), RN[k], false, hotelStaying() ? 3 : 1, 0.32f);
        text("Room service works from anywhere while you're checked in", 160, 136, 0.28f, C(150, 120, 150), 1);
    } else if (!G.hotelOwned) {
        button(hB(0), "Buy Hotel ($12,000)", false, 2, 0.36f);
    } else {
        snprintf(b, sizeof b, "Rate -$10  ($%d)", G.hotelRate); button(hB(0), b, false, 0, 0.34f); button(hB(1), "Rate +$10", false, 0, 0.34f);
        for (int k = 0; k < 4; k++) { snprintf(b, sizeof b, "%s: %d staff", STAFF_N[k], G.hotelStaff[k]); button(Btn{6 + (k % 2) * 156.f, 64 + (k / 2) * 28.f, 152, 26}, b, G.hotelStaff[k] > 0, 0, 0.3f); }
        snprintf(b, sizeof b, "Theme: %s", THEME_N[G.hotelTheme % 4]); button(Btn{6, 122, 152, 26}, b, false, 3, 0.3f);
        for (int k = 0; k < 6; k++) { bool has = G.hotelUpg & (1 << k); snprintf(b, sizeof b, has ? "%s (built)" : "%s $%d", UPG[k].name, UPG[k].cost); button(Btn{6 + (k % 2) * 156.f, 152 + (k / 2) * 18.f, 152, 17}, b, has, has ? 2 : 0, 0.26f); }
    }
    (void)t;
}
