#include "family.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "goals.hpp"
#include "extras.hpp"
#include "audio.hpp"
#include <algorithm>
#include <cstdio>
#include <vector>

const char* const CHORE_NAMES[CHR_COUNT] = {"No chore", "Dishes", "Laundry", "Tidy Up", "Take Out Trash", "Walk the Pet", "Water the Garden"};
const char* const FAMEV_NAMES[FEV_COUNT] = {"Pancake Saturday", "Beach Day", "Sunday Dinner", "Family Game Night", "Movie Night"};
const char* const FAMEV_WHEN[FEV_COUNT] = {"Sat 9 AM", "Sat 1 PM", "Sun 6 PM", "Fri 7 PM", "Wed 8 PM"};

namespace {
const int FEV_DAY[FEV_COUNT] = {5, 5, 6, 4, 2};   // weekday() 0 = Monday
const int FEV_HOUR[FEV_COUNT] = {9, 13, 18, 19, 20};
NpcLife& K(int s) { if (G.life.size() < G.sims.size()) G.life.resize(G.sims.size()); return G.life[s]; }
bool valid(int s) { return s >= 0 && s < (int)G.sims.size(); }
int hourNow() { return (int)(G.minute / 60); }
std::vector<int> household(bool includeMe) { std::vector<int> v; for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].household && (includeMe || (int)i != G.mc)) v.push_back((int)i); return v; }
bool hasGrownUp() { for (int s : household(false)) if (G.sims[s].isAdult()) return true; return false; }
bool pastCurfew(int h) { return h >= G.curfew || h < 5; }
std::string curfewStr() { int h = G.curfew; char b[16]; snprintf(b, sizeof b, "%d %s", h > 12 ? h - 12 : h, h >= 12 ? "PM" : "AM"); return b; }
void removeFromHome(int s) { for (int r = 0; r < R_COUNT; r++) { auto& v = G.rooms[G.house][r].actors; v.erase(std::remove_if(v.begin(), v.end(), [&](const Actor& a) { return a.kind == 0 && a.id == s; }), v.end()); } }
void putHome(int s, int room) {
    for (int r = 0; r < R_COUNT; r++) for (const Actor& a : G.rooms[G.house][r].actors) if (a.kind == 0 && a.id == s) return;
    Actor a; a.kind = 0; a.id = (uint16_t)s; a.x = 300; a.tx = 240; a.y = 218;
    G.rooms[G.house][room].actors.push_back(a);
}

// ---- catching a teen who snuck out
int g_caught = -1, g_morningCatch = -1;
std::string caughtFn(int choice, int) {
    int t = g_caught;
    if (!valid(t)) return "";
    NpcLife& k = K(t);
    G.sneaker = -1;
    putHome(t, R_LIVING);
    if (choice == 0) { k.grounded = 3; k.strict = (int8_t)std::min(100, k.strict + 8); k.disc = (uint8_t)std::min(100, k.disc + 10); changeRel(G.mc, t, -8, 0); addMoodlet(G.sims[t], "Grounded", EM_ANGRY, 2, 1440); return G.sims[t].name + " is grounded for 3 days. Door slam. Dramatic sigh. Classic."; }
    if (choice == 1) { k.warm = (int8_t)std::min(100, k.warm + 6); changeRel(G.mc, t, 5, 0); return "\"...ok. omw home. sorry.\" " + G.sims[t].name + " is back in 20 minutes. You talk it out over cereal."; }
    k.warm = (int8_t)std::min(100, k.warm + 4); k.disc = (uint8_t)std::max(0, k.disc - 6); changeRel(G.mc, t, 7, 0);
    return "You leave the window open and a granola bar on the sill. " + G.sims[t].name + " owes you. Big time.";
}
void caught(int t, bool morning) {
    if (momentActive()) return;
    g_caught = t;
    std::string txt = morning ? "The neighbor says she saw " + G.sims[t].name + " climbing in the window at 2 AM. Wearing YOUR jacket."
                              : G.sims[t].name + "'s window is wide open and their bed is stuffed with pillows shaped like a person. It's " + clockStr(G.minute) + ". Curfew was " + curfewStr() + ".";
    momentCustom(morning ? "Busted (Kind Of)" : "Sneaking Out!", txt, "Ground them (3 days)", "Text: come home, no questions", "Let it slide", caughtFn, 0, t);
}

// ---- you sneaking out
int g_wantRoom = -1;
bool g_bypass = false;
std::string sneakFn(int choice, int) {
    if (choice != 0 || g_wantRoom < 0) return "You stay in. Scrolling STATUS under the covers instead.";
    G.mcSneaking = true;
    addMoodlet(MC(), "Sneaking Out", EM_PLAYFUL, 2, 240);
    g_bypass = true;
    travelTo(g_wantRoom);
    return "";
}
void grounded(const std::string& why) {
    G.mcGrounded = 2; G.mcSneaking = false;
    addMoodlet(MC(), "Grounded", EM_SAD, 2, 2880);
    for (int s : household(false)) if (G.sims[s].isAdult()) { changeRel(G.mc, s, -6, 0); say(s, "You are SO grounded."); break; }
    dlgSay(-1, why + " You're grounded for 2 days: school and home only.");
    sfxNo();
}

// ---- family traditions
void familyEvent(int e) {
    auto fam = household(true);
    if (fam.size() < 2) return;
    if (!roomIsHome(G.room)) {
        for (int s : fam) if (s != G.mc) changeRel(G.mc, s, -2, 0);
        toast(std::string("You missed ") + FAMEV_NAMES[e] + "... the family saved you a plate.");
        return;
    }
    static const char* const TXT[FEV_COUNT] = {
        "Pancakes shaped like bunnies, hearts and one mysterious blob. Syrup EVERYWHERE.",
        "Everybody piles into the car with way too many snacks. Sandcastles, sunscreen fights, a perfect day.",
        "Sunday dinner: everybody at the table, phones in the basket, laughing about the week.",
        "Family Game Night! Someone flips the board. Someone accuses someone else of cheating. Best night ever.",
        "Blanket fort, popcorn and a movie everyone's seen five times. Half the family falls asleep."};
    for (size_t i = 0; i < fam.size(); i++) {
        Sim& s = G.sims[fam[i]];
        s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 30);
        s.needs[N_SOCIAL] = std::min(100.f, s.needs[N_SOCIAL] + 30);
        if (e == FEV_PANCAKES || e == FEV_DINNER) s.needs[N_HUNGER] = std::min(100.f, s.needs[N_HUNGER] + 45);
        addMoodlet(s, "Family Time", EM_HAPPY, 2, 360);
        for (size_t j = i + 1; j < fam.size(); j++) changeRel(fam[i], fam[j], 4, 0);
        if (s.isLittle() || s.stage <= ST_TEEN) { NpcLife& k = K(fam[i]); k.warm = (int8_t)std::min(100, k.warm + 3); k.social = (uint8_t)std::min(100, k.social + 3); }
    }
    for (int s : fam) if (s != G.mc) putHome(s, G.room);
    addMemoryOnce(MEM_FAMILY, G.mc, -1, std::string(FAMEV_NAMES[e]) + " with the family");
    rewardPopup(FAMEV_NAMES[e], TXT[e]);
    sfxYay();
}
}   // namespace

void familyHourly(int hour) {
    auto fam = household(false);
    // family traditions on their day
    for (int e = 0; e < FEV_COUNT; e++) if ((G.famEvents >> e & 1) && weekday(G.day) == FEV_DAY[e] && hour == FEV_HOUR[e]) familyEvent(e);
    // the chore chart: everyone does their chore at 5 PM (most of the time)
    if (hour == 17) {
        std::string done, skipped;
        for (int s : fam) {
            NpcLife& k = K(s);
            Sim& o = G.sims[s];
            if (!k.chore || o.stage < ST_CHILD || s == G.sneaker) continue;
            int skip = 10 + (o.hasTrait(TR_LAZY) ? 30 : 0) - k.disc / 4 - (o.hasTrait(TR_NEAT) ? 15 : 0);
            if (irand(100) < skip) { skipped += (skipped.empty() ? "" : ", ") + o.name; changeRel(G.mc, s, -1, 0); continue; }
            k.choresWeek = (uint8_t)std::min(14, k.choresWeek + 1);
            k.disc = (uint8_t)std::min(100, k.disc + 2);
            o.needs[N_FUN] = std::max(0.f, o.needs[N_FUN] - 6);
            done += (done.empty() ? "" : ", ") + o.name + " (" + CHORE_NAMES[k.chore] + ")";
        }
        if (!done.empty()) toast("Chores done: " + done + (skipped.empty() ? "" : ". Skipped: " + skipped + "!"));
        else if (!skipped.empty()) toast("Chore chart ignored by: " + skipped + "!");
    }
    // allowance day: Saturday morning
    if (hour == 10 && weekday(G.day) == 5 && G.allowance) {
        static const int AMT[4] = {0, 5, 10, 20};
        int paid = 0;
        for (int s : fam) {
            Sim& o = G.sims[s];
            if (o.stage < ST_CHILD || o.stage > ST_TEEN) continue;
            NpcLife& k = K(s);
            bool earned = k.chore == CHR_NONE || k.choresWeek >= 3;
            if (earned && G.money >= AMT[G.allowance]) { G.money -= AMT[G.allowance]; paid += AMT[G.allowance]; changeRel(G.mc, s, 4, 0); addMoodlet(o, "Allowance Day", EM_HAPPY, 1, 300); }
            else if (!earned) say(s, "No chores, no allowance? ...Fair.");
            k.choresWeek = 0;
        }
        if (paid) toast("Allowance day! You paid out $" + std::to_string(paid) + ".");
    }
    // a household teen sneaking out after curfew
    if (hour == (G.curfew + 1) % 24 && G.sneaker < 0) {
        for (int s : fam) {
            Sim& o = G.sims[s];
            if (o.stage < ST_PRETEEN || o.stage > ST_TEEN || K(s).grounded) continue;
            const Rel* r = findRel(G.mc, s);
            int ch = 6 + (o.hasTrait(TR_OUTGOING) ? 8 : 0) + (o.hasTrait(TR_HOTHEAD) ? 8 : 0) + (o.hasTrait(TR_SNEAKY) ? 12 : 0) + ((r && r->fr < 20) ? 10 : 0) - K(s).disc / 8 - (G.curfew >= 23 ? 4 : 0);
            if (irand(100) < ch) { G.sneaker = (int16_t)s; removeFromHome(s); break; }
        }
    }
    if (G.sneaker >= 0) {
        int t = G.sneaker;
        if (hour == 2) {   // back in through the window
            putHome(t, R_BEDROOM);
            G.sneaker = -1;
            if (irand(100) < 40) g_morningCatch = t;
        } else if (roomIsHome(G.room) && !(ACT.active() && ACT.kind == AK_SLEEP) && irand(100) < 35) caught(t, false);
    }
    if (hour == 8 && g_morningCatch >= 0) { int t = g_morningCatch; g_morningCatch = -1; if (roomIsHome(G.room)) caught(t, true); }
    // you: out past curfew
    if (G.mcSneaking && hour == 6) grounded("6 AM. Your bed is empty, the window's open, and someone noticed.");
}

void familyDaily() {
    for (int s : household(false)) { NpcLife& k = K(s); if (k.grounded) k.grounded--; }
    if (G.mcGrounded) { G.mcGrounded--; if (!G.mcGrounded) toast("You're not grounded anymore! Freedom!"); }
}

bool familyTravelGate(int room) {
    if (g_bypass) { g_bypass = false; return true; }
    if (roomIsHome(room) || G.sims.empty()) return true;
    Sim& me = MC();
    if (me.stage > ST_TEEN || !hasGrownUp()) return true;   // only kids & teens living with grown-ups
    if (G.mcGrounded && room != R_SCHOOL && room != R_YARD) { toast("You're grounded! Only school for now."); sfxNo(); return false; }
    int h = hourNow();
    if (roomIsHome(G.room) && pastCurfew(h) && !G.mcSneaking) {
        if (momentActive()) return false;
        g_wantRoom = room;
        momentCustom("Sneak Out?", "It's " + clockStr(G.minute) + ". Curfew was " + curfewStr() + ". Everyone thinks you're asleep...", "Climb out the window", "Stay in", nullptr, sneakFn, 0, G.mc);
        return false;
    }
    return true;
}

void familyOnArrive(int room) {
    if (!G.mcSneaking || !roomIsHome(room)) return;
    int catchP = 30 - (MC().hasTrait(TR_SNEAKY) ? 15 : 0) + (hourNow() >= 4 && hourNow() < 6 ? 15 : 0);
    if (irand(100) < catchP) grounded("The lights flick on. A parent is sitting in the dark, arms crossed. \"Where were YOU?\"");
    else { G.mcSneaking = false; toast("You climb back in the window. Nobody noticed. Legend."); addMoodlet(MC(), "Got Away With It", EM_CONFIDENT, 1, 240); }
}

std::string familyStatusLine() {
    std::string o = "Curfew " + curfewStr();
    if (G.sneaker >= 0 && valid(G.sneaker)) o += "  -  " + G.sims[G.sneaker].name + "'s bed is empty...";
    if (G.mcGrounded) o += "  -  You're grounded";
    return o;
}
